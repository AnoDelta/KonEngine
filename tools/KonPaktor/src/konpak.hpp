#pragma once
// konpak.hpp -- KonPak archive format v2
// Single clean header. No duplicate definitions.
//
// Memory model: load() reads only the index (tiny).
// getData() decrypts one file on demand and caches it.
// pack() static method streams files one at a time — no full-archive RAM spike.
//
// Compression: uses miniz (bundled) when KONPAK_USE_MINIZ is defined,
// otherwise falls back to system zlib.

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <functional>
#include <thread>
#include <algorithm>
#include <optional>
#include <sys/stat.h>

// -----------------------------------------------------------------------
// Compression backend — miniz (bundled) or system zlib
// -----------------------------------------------------------------------
// Compression backend
#if defined(KONPAK_USE_MINIZ)
#  include "miniz.h"
   // miniz.c amalgamated defines these zlib-compat names:
   // If the include worked, compress2/Z_OK etc. are available.
#  define KONPAK_HAS_COMPRESS
#elif !defined(_WIN32) || !defined(__MINGW32__)
#  include <zlib.h>
#  define KONPAK_HAS_COMPRESS
#endif
// On MinGW cross-compile without zlib/miniz, compression is skipped (store-only)

// -----------------------------------------------------------------------
// Platform crypto
// -----------------------------------------------------------------------
#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOGDI
#    define NOGDI
#  endif
#  ifndef NOUSER
#    define NOUSER
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#  undef DrawText
#  undef Rectangle
#  undef PlaySound
#  undef LoadBitmap
#  undef CreateFont
#  undef CopyFile
#  undef DeleteFile
#  undef MoveFile
#  include <bcrypt.h>
#  pragma comment(lib, "bcrypt.lib")
#  include <direct.h>
#else
#  include <openssl/evp.h>
#  include <openssl/rand.h>
#  include <openssl/sha.h>
#endif

namespace KonPak {

// -----------------------------------------------------------------------
// Constants
// -----------------------------------------------------------------------
static constexpr uint8_t MAGIC[4]    = { 'K', 'P', 'A', 'K' };
static constexpr uint8_t VERSION     = 0x02;
static constexpr uint8_t FLAG_ENC    = 0x01;
static constexpr uint8_t FLAG_COMP   = 0x02;
static constexpr int     SALT_SIZE   = 16;
static constexpr int     IV_SIZE     = 16;
static constexpr int     KEY_SIZE    = 32;
static constexpr int     PBKDF2_ITER = 100000;
static constexpr int     BLOCK_SIZE  = 16;

#ifdef KON_PACK_KEY
  static constexpr const char* BUILTIN_KEY = KON_PACK_KEY;
#else
  static constexpr const char* BUILTIN_KEY = nullptr;
#endif

// -----------------------------------------------------------------------
// Index entry — metadata only, no data buffer
// -----------------------------------------------------------------------
struct IndexEntry {
    std::string path;
    uint64_t    sizeRaw    = 0;
    uint64_t    sizePacked = 0;
    uint64_t    offset     = 0;
};

// -----------------------------------------------------------------------
// Crypto helpers
// -----------------------------------------------------------------------
inline void randomBytes(uint8_t* out, size_t len) {
#ifdef _WIN32
    BCryptGenRandom(nullptr, out, (ULONG)len, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
#else
    RAND_bytes(out, (int)len);
#endif
}

inline void deriveKey(const std::string& pw,
                      const uint8_t* salt, int saltLen, uint8_t* keyOut) {
#ifdef _WIN32
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM,
                                nullptr, BCRYPT_ALG_HANDLE_HMAC_FLAG);
    BCryptDeriveKeyPBKDF2(hAlg, (PUCHAR)pw.data(), (ULONG)pw.size(),
                          (PUCHAR)salt, saltLen, PBKDF2_ITER, keyOut, KEY_SIZE, 0);
    BCryptCloseAlgorithmProvider(hAlg, 0);
#else
    PKCS5_PBKDF2_HMAC(pw.c_str(), (int)pw.size(), salt, saltLen,
                      PBKDF2_ITER, EVP_sha256(), KEY_SIZE, keyOut);
#endif
}

inline std::vector<uint8_t> aesEncrypt(const uint8_t* data, size_t len,
                                        const uint8_t* key, const uint8_t* iv) {
    std::vector<uint8_t> out(len + BLOCK_SIZE);
#ifdef _WIN32
    BCRYPT_ALG_HANDLE hAlg=nullptr; BCRYPT_KEY_HANDLE hKey=nullptr;
    BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, nullptr, 0);
    BCryptSetProperty(hAlg, BCRYPT_CHAINING_MODE, (PUCHAR)BCRYPT_CHAIN_MODE_CBC,
                      sizeof(BCRYPT_CHAIN_MODE_CBC), 0);
    BCryptGenerateSymmetricKey(hAlg, &hKey, nullptr, 0, (PUCHAR)key, KEY_SIZE, 0);
    ULONG result=0;
    std::vector<uint8_t> ivCopy(iv, iv+IV_SIZE);
    BCryptEncrypt(hKey, (PUCHAR)data, (ULONG)len, nullptr,
                  ivCopy.data(), IV_SIZE, out.data(), (ULONG)out.size(),
                  &result, BCRYPT_BLOCK_PADDING);
    out.resize(result);
    BCryptDestroyKey(hKey); BCryptCloseAlgorithmProvider(hAlg, 0);
#else
    int l1=0, l2=0;
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv);
    EVP_EncryptUpdate(ctx, out.data(), &l1, data, (int)len);
    EVP_EncryptFinal_ex(ctx, out.data()+l1, &l2);
    EVP_CIPHER_CTX_free(ctx);
    out.resize(l1+l2);
#endif
    return out;
}

inline std::vector<uint8_t> aesDecrypt(const uint8_t* data, size_t len,
                                        const uint8_t* key, const uint8_t* iv) {
    std::vector<uint8_t> out(len);
#ifdef _WIN32
    BCRYPT_ALG_HANDLE hAlg=nullptr; BCRYPT_KEY_HANDLE hKey=nullptr;
    BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, nullptr, 0);
    BCryptSetProperty(hAlg, BCRYPT_CHAINING_MODE, (PUCHAR)BCRYPT_CHAIN_MODE_CBC,
                      sizeof(BCRYPT_CHAIN_MODE_CBC), 0);
    BCryptGenerateSymmetricKey(hAlg, &hKey, nullptr, 0, (PUCHAR)key, KEY_SIZE, 0);
    ULONG result=0;
    std::vector<uint8_t> ivCopy(iv, iv+IV_SIZE);
    BCryptDecrypt(hKey, (PUCHAR)data, (ULONG)len, nullptr,
                  ivCopy.data(), IV_SIZE, out.data(), (ULONG)out.size(),
                  &result, BCRYPT_BLOCK_PADDING);
    out.resize(result);
    BCryptDestroyKey(hKey); BCryptCloseAlgorithmProvider(hAlg, 0);
#else
    int l1=0, l2=0;
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv);
    EVP_DecryptUpdate(ctx, out.data(), &l1, data, (int)len);
    EVP_DecryptFinal_ex(ctx, out.data()+l1, &l2);
    EVP_CIPHER_CTX_free(ctx);
    out.resize(l1+l2);
#endif
    return out;
}

inline std::vector<uint8_t> kompressData(const uint8_t* data, size_t len) {
#ifdef KONPAK_HAS_COMPRESS
    uLongf outLen = compressBound((uLongf)len);
    std::vector<uint8_t> out(outLen);
    if (::compress2(out.data(), &outLen, data, (uLongf)len, Z_BEST_COMPRESSION) != Z_OK)
        throw std::runtime_error("KonPak: compression failed");
    out.resize(outLen);
    return out;
#else
    // No compression library available — store as-is
    return std::vector<uint8_t>(data, data + len);
#endif
}

inline std::vector<uint8_t> decompressData(const uint8_t* data, size_t len,
                                             uint64_t originalSize) {
#ifdef KONPAK_HAS_COMPRESS
    std::vector<uint8_t> out(originalSize);
    uLongf outLen = (uLongf)originalSize;
    if (::uncompress(out.data(), &outLen, data, (uLongf)len) != Z_OK)
        throw std::runtime_error("KonPak: decompression failed");
    return out;
#else
    // No compression — data was stored as-is
    return std::vector<uint8_t>(data, data + len);
#endif
}

// -----------------------------------------------------------------------
// Pack
// -----------------------------------------------------------------------
struct Pack {
    std::string password;

    // Index — loaded on open(), tiny (paths + offsets only)
    std::vector<IndexEntry> index;

    // Compat: entries mirrors index metadata for old GUI/CLI code.
    // data field is intentionally empty — call getData() to load a file.
    struct Entry {
        std::string          path;
        uint64_t             sizeRaw    = 0;
        uint64_t             sizePacked = 0;
        uint64_t             offset     = 0;
        std::vector<uint8_t> data;       // empty unless explicitly loaded
    };
    mutable std::vector<Entry> entries;

    // ---- Index lookup ----
    const IndexEntry* findIndex(const std::string& path) const {
        for (auto& e : index) if (e.path == path) return &e;
        return nullptr;
    }

    // ---- Old API: find entry by path ----
    Entry* find(const std::string& path) {
        for (auto& e : entries) if (e.path == path) return &e;
        return nullptr;
    }
    const Entry* find(const std::string& path) const {
        for (auto& e : entries) if (e.path == path) return &e;
        return nullptr;
    }

    std::vector<std::string> list() const {
        std::vector<std::string> out;
        for (auto& e : index) out.push_back(e.path);
        return out;
    }

    // ---- Open: read header + index only. O(index) not O(data). ----
    void load(const std::string& inPath) {
        if (password.empty())
            throw std::runtime_error("KonPak: password not set");

        m_filePath = inPath;
        std::ifstream in(inPath, std::ios::binary);
        if (!in) throw std::runtime_error("KonPak: cannot open: " + inPath);

        uint8_t magic[4];
        in.read(reinterpret_cast<char*>(magic), 4);
        if (memcmp(magic, MAGIC, 4) != 0)
            throw std::runtime_error("KonPak: not a .konpak file");

        uint8_t version, flags;
        in.read(reinterpret_cast<char*>(&version), 1);
        in.read(reinterpret_cast<char*>(&flags), 1);
        (void)version; (void)flags;

        in.read(reinterpret_cast<char*>(m_salt), SALT_SIZE);
        in.read(reinterpret_cast<char*>(m_iv),   IV_SIZE);
        deriveKey(password, m_salt, SALT_SIZE, m_key);

        uint32_t idxSize = 0;
        in.read(reinterpret_cast<char*>(&idxSize), 4);
        std::vector<uint8_t> encIdx(idxSize);
        in.read(reinterpret_cast<char*>(encIdx.data()), idxSize);
        auto idxBytes = aesDecrypt(encIdx.data(), encIdx.size(), m_key, m_iv);

        size_t pos = 0;
        auto ru16 = [&]{ uint16_t v=idxBytes[pos]|(uint16_t(idxBytes[pos+1])<<8); pos+=2; return v; };
        auto ru32 = [&]{ uint32_t v=0; for(int i=0;i<4;i++) v|=uint32_t(idxBytes[pos+i])<<(i*8); pos+=4; return v; };
        auto ru64 = [&]{ uint64_t v=0; for(int i=0;i<8;i++) v|=uint64_t(idxBytes[pos+i])<<(i*8); pos+=8; return v; };

        uint32_t count = ru32();
        index.clear();   index.reserve(count);
        entries.clear(); entries.reserve(count);

        for (uint32_t i = 0; i < count; i++) {
            IndexEntry ie;
            uint16_t plen = ru16();
            ie.path.resize(plen);
            for (char& c : ie.path) c = char(idxBytes[pos++]);
            ie.sizeRaw    = ru64();
            ie.sizePacked = ru64();
            ie.offset     = ru64();
            index.push_back(ie);

            Entry e;
            e.path = ie.path; e.sizeRaw = ie.sizeRaw;
            e.sizePacked = ie.sizePacked; e.offset = ie.offset;
            entries.push_back(std::move(e));
        }

        m_dataStart = uint64_t(in.tellg());
        m_isOpen    = true;
        m_cache.clear();
    }

    void openWithBuiltinKey(const std::string& path) {
        if (!BUILTIN_KEY) throw std::runtime_error("KonPak: no builtin key");
        password = BUILTIN_KEY;
        load(path);
    }

    // ---- Read one file on demand. Caches it. ----
    const std::vector<uint8_t>& getData(const std::string& path) const {
        auto it = m_cache.find(path);
        if (it != m_cache.end()) return it->second;

        const IndexEntry* ie = findIndex(path);
        if (!ie) throw std::runtime_error("KonPak: not found: " + path);

        std::ifstream in(m_filePath, std::ios::binary);
        if (!in) throw std::runtime_error("KonPak: cannot reopen: " + m_filePath);

        in.seekg(std::streamoff(m_dataStart + ie->offset));
        std::vector<uint8_t> blob(ie->sizePacked);
        in.read(reinterpret_cast<char*>(blob.data()), ie->sizePacked);

        auto dec = aesDecrypt(blob.data(), blob.size(), m_key, m_iv);
        auto raw = decompressData(dec.data(), dec.size(), ie->sizeRaw);
        m_cache[path] = std::move(raw);
        return m_cache[path];
    }

    void clearEntry(const std::string& path) { m_cache.erase(path); }
    void clearCache()                         { m_cache.clear(); }
    bool isOpen() const                       { return m_isOpen; }

    // ---- Extract one file to disk ----
    void extractFile(const std::string& packPath, const std::string& diskPath) const {
        const auto& data = getData(packPath);
        std::ofstream f(diskPath, std::ios::binary);
        if (!f) throw std::runtime_error("KonPak: cannot write: " + diskPath);
        f.write(reinterpret_cast<const char*>(data.data()), data.size());
    }

    // ---- Old API: addFile / addData / remove / save ----
    void addFile(const std::string& diskPath, const std::string& packPath) {
        m_pending[packPath] = diskPath;
        _updateEntrySizeFromDisk(packPath, diskPath);
    }

    void addData(const std::string& packPath, const std::vector<uint8_t>& data) {
        char tmp[L_tmpnam]; std::tmpnam(tmp);
        std::string tmpPath(tmp);
        { std::ofstream f(tmpPath, std::ios::binary);
          f.write(reinterpret_cast<const char*>(data.data()), data.size()); }
        m_pending[packPath] = tmpPath;
        m_tempFiles.push_back(tmpPath);
        bool found = false;
        for (auto& e : entries) if (e.path==packPath){ e.sizeRaw=data.size(); found=true; break; }
        if (!found) { Entry e; e.path=packPath; e.sizeRaw=data.size(); entries.push_back(e); }
    }

    bool remove(const std::string& packPath) {
        m_removed.insert(packPath);
        m_pending.erase(packPath);
        auto it = std::find_if(entries.begin(), entries.end(),
            [&](const Entry& e){ return e.path==packPath; });
        if (it==entries.end()) return false;
        entries.erase(it);
        return true;
    }

    void save(const std::string& outPath,
          std::function<void(int,int,const std::string&)> onProgress = nullptr) const {
        if (password.empty()) throw std::runtime_error("KonPak: password not set");

        std::vector<std::pair<std::string,std::string>> files;
        if (m_isOpen) {
            for (auto& ie : index) {
                if (m_removed.count(ie.path)) continue;
                if (m_pending.count(ie.path)) continue;
                char tmp[L_tmpnam]; std::tmpnam(tmp);
                std::string tmpPath(tmp);
                extractFile(ie.path, tmpPath);
                files.push_back({tmpPath, ie.path});
                m_tempFiles.push_back(tmpPath);
            }
        }
        for (auto& [pp, dp] : m_pending) files.push_back({dp, pp});

        pack(outPath, files, password);

        for (auto& t : m_tempFiles) ::remove(t.c_str());
        m_tempFiles.clear();
        m_pending.clear();
        m_removed.clear();
    }

    // ---- Streaming pack — never holds more than one file in RAM ----
    static void pack(const std::string& outPath,
                     const std::vector<std::pair<std::string,std::string>>& files,
                     const std::string& pw,
                     std::function<void(int,int,const std::string&)> progress = {}) {

        if (pw.empty()) throw std::runtime_error("KonPak: password required");

        uint8_t salt[SALT_SIZE], iv[IV_SIZE], key[KEY_SIZE];
        randomBytes(salt, SALT_SIZE);
        randomBytes(iv,   IV_SIZE);
        deriveKey(pw, salt, SALT_SIZE, key);

        struct BlobMeta { std::string path; uint64_t sizeRaw, sizePacked, offset; };
        std::vector<BlobMeta> metas;
        metas.reserve(files.size());
        uint64_t offset = 0;

        std::string tmpPath = outPath + ".kptmp";
        {
            std::ofstream tmp(tmpPath, std::ios::binary);
            if (!tmp) throw std::runtime_error("KonPak: cannot write tmp");

            for (int i = 0; i < (int)files.size(); i++) {
                if (progress) progress(i, (int)files.size(), files[i].second);

                std::ifstream f(files[i].first, std::ios::binary);
                if (!f) throw std::runtime_error("KonPak: cannot open: " + files[i].first);

                std::vector<uint8_t> raw(
                    (std::istreambuf_iterator<char>(f)),
                    std::istreambuf_iterator<char>());

                auto comp = kompressData(raw.data(), raw.size());
                auto enc  = aesEncrypt(comp.data(), comp.size(), key, iv);

                BlobMeta m; m.path=files[i].second; m.sizeRaw=raw.size();
                m.sizePacked=enc.size(); m.offset=offset;
                metas.push_back(m);
                offset += enc.size();

                tmp.write(reinterpret_cast<const char*>(enc.data()), enc.size());
            }
        }

        std::vector<uint8_t> idxBytes;
        auto wu16=[&](uint16_t v){ idxBytes.push_back(v&0xFF); idxBytes.push_back(v>>8); };
        auto wu32=[&](uint32_t v){ for(int i=0;i<4;i++) idxBytes.push_back((v>>(i*8))&0xFF); };
        auto wu64=[&](uint64_t v){ for(int i=0;i<8;i++) idxBytes.push_back((v>>(i*8))&0xFF); };
        wu32((uint32_t)metas.size());
        for (auto& m : metas) {
            wu16((uint16_t)m.path.size());
            for (char c : m.path) idxBytes.push_back((uint8_t)c);
            wu64(m.sizeRaw); wu64(m.sizePacked); wu64(m.offset);
        }
        auto encIdx = aesEncrypt(idxBytes.data(), idxBytes.size(), key, iv);

        std::ofstream out(outPath, std::ios::binary);
        if (!out) throw std::runtime_error("KonPak: cannot write: " + outPath);

        out.write(reinterpret_cast<const char*>(MAGIC), 4);
        out.write(reinterpret_cast<const char*>(&VERSION), 1);
        uint8_t flags = FLAG_ENC | FLAG_COMP;
        out.write(reinterpret_cast<const char*>(&flags), 1);
        out.write(reinterpret_cast<const char*>(salt), SALT_SIZE);
        out.write(reinterpret_cast<const char*>(iv),   IV_SIZE);
        uint32_t idxSz = (uint32_t)encIdx.size();
        out.write(reinterpret_cast<const char*>(&idxSz), 4);
        out.write(reinterpret_cast<const char*>(encIdx.data()), encIdx.size());

        // Stream blobs from tmp into final — 4MB at a time
        {
            std::ifstream tmp(tmpPath, std::ios::binary);
            static constexpr size_t CHUNK = 4*1024*1024;
            std::vector<char> buf(CHUNK);
            while (tmp.read(buf.data(), CHUNK) || tmp.gcount()>0)
                out.write(buf.data(), tmp.gcount());
        }
        ::remove(tmpPath.c_str());
    }

    static bool checkPassword(const std::string& filePath, const std::string& pw) {
        try { Pack p; p.password=pw; p.load(filePath); return true; }
        catch (...) { return false; }
    }

private:
    std::string m_filePath;
    bool        m_isOpen    = false;
    uint64_t    m_dataStart = 0;
    uint8_t     m_salt[SALT_SIZE] = {};
    uint8_t     m_iv[IV_SIZE]     = {};
    uint8_t     m_key[KEY_SIZE]   = {};

    mutable std::unordered_map<std::string, std::vector<uint8_t>> m_cache;
    mutable std::unordered_map<std::string, std::string>          m_pending;
    mutable std::unordered_set<std::string>                       m_removed;
    mutable std::vector<std::string>                              m_tempFiles;

    void _updateEntrySizeFromDisk(const std::string& packPath, const std::string& diskPath) {
        std::ifstream f(diskPath, std::ios::binary | std::ios::ate);
        uint64_t sz = f.is_open() ? (uint64_t)f.tellg() : 0;
        for (auto& e : entries) if (e.path==packPath){ e.sizeRaw=sz; return; }
        Entry e; e.path=packPath; e.sizeRaw=sz; entries.push_back(e);
    }
};

// Convenience alias so old code using KonPak::Entry still compiles
using Entry = Pack::Entry;

} // namespace KonPak
