#pragma once
// konpak.hpp -- KonPak archive format
// Shared by the engine reader, CLI tool, and KonPaktor GUI.
//
// Format: KPAK header + encrypted index + encrypted compressed data blobs
// Compress first (zlib deflate), then encrypt (AES-256-CBC, PBKDF2 key).
//
// Dependencies:
//   - miniz.h  (single-header zlib, bundled)
//   - On Linux: -lcrypto (OpenSSL)
//   - On Windows: -lbcrypt (built-in)

#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <functional>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif

// -----------------------------------------------------------------------
// zlib for compression
// -----------------------------------------------------------------------
#include "miniz.h"

// -----------------------------------------------------------------------
// AES-256-CBC + PBKDF2 via platform crypto
// -----------------------------------------------------------------------
#ifdef _WIN32
  #include <windows.h>
  #include <bcrypt.h>
  #pragma comment(lib, "bcrypt.lib")
#else
  #include <openssl/evp.h>
  #include <openssl/rand.h>
  #include <openssl/sha.h>
#endif

namespace KonPak {

// -----------------------------------------------------------------------
// Constants
// -----------------------------------------------------------------------
static constexpr uint8_t  MAGIC[4]   = { 'K', 'P', 'A', 'K' };
static constexpr uint8_t  VERSION    = 0x01;
static constexpr uint8_t  FLAG_ENC   = 0x01;
static constexpr uint8_t  FLAG_COMP  = 0x02;
static constexpr int      SALT_SIZE  = 16;
static constexpr int      IV_SIZE    = 16;
static constexpr int      KEY_SIZE   = 32;  // AES-256
static constexpr int      PBKDF2_ITER = 100000;
static constexpr int      BLOCK_SIZE = 16;  // AES block

// -----------------------------------------------------------------------
// Compile-time key support
// Define KON_PACK_KEY in your CMakeLists to bake a password into the binary:
//   target_compile_definitions(MyGame PRIVATE KON_PACK_KEY="mygamekey")
// Then use Pack::openWithBuiltinKey() instead of setting password manually.
// -----------------------------------------------------------------------
#ifdef KON_PACK_KEY
  static constexpr const char* BUILTIN_KEY = KON_PACK_KEY;
#else
  static constexpr const char* BUILTIN_KEY = nullptr;
#endif

// -----------------------------------------------------------------------
// Entry — one file inside the pack
// -----------------------------------------------------------------------
struct Entry {
    std::string path;        // relative path, e.g. "sprites/player.png"
    uint64_t    offset  = 0; // byte offset in the DATA section
    uint64_t    sizeRaw = 0; // original uncompressed size
    uint64_t    sizePacked = 0; // size after compress+encrypt
    std::vector<uint8_t> data; // in-memory data (raw, after decrypt+decompress)
};

// -----------------------------------------------------------------------
// Crypto helpers
// -----------------------------------------------------------------------

// Generate cryptographically random bytes
inline void randomBytes(uint8_t* out, size_t len) {
#ifdef _WIN32
    BCryptGenRandom(nullptr, out, (ULONG)len, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
#else
    RAND_bytes(out, (int)len);
#endif
}

// PBKDF2-SHA256: password + salt -> 32-byte AES key
inline void deriveKey(const std::string& password,
                      const uint8_t* salt, int saltLen,
                      uint8_t* keyOut) {
#ifdef _WIN32
    // BCrypt PBKDF2
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM,
                                nullptr, BCRYPT_ALG_HANDLE_HMAC_FLAG);
    BCryptDeriveKeyPBKDF2(hAlg,
        (PUCHAR)password.data(), (ULONG)password.size(),
        (PUCHAR)salt, saltLen,
        PBKDF2_ITER, keyOut, KEY_SIZE, 0);
    BCryptCloseAlgorithmProvider(hAlg, 0);
#else
    PKCS5_PBKDF2_HMAC(password.c_str(), (int)password.size(),
                      salt, saltLen,
                      PBKDF2_ITER, EVP_sha256(),
                      KEY_SIZE, keyOut);
#endif
}

// AES-256-CBC encrypt. Returns ciphertext (includes PKCS7 padding).
inline std::vector<uint8_t> aesEncrypt(const uint8_t* data, size_t len,
                                        const uint8_t* key, const uint8_t* iv) {
    std::vector<uint8_t> out(len + BLOCK_SIZE);
    int outLen1 = 0, outLen2 = 0;
#ifdef _WIN32
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    BCRYPT_KEY_HANDLE hKey = nullptr;
    BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, nullptr, 0);
    BCryptSetProperty(hAlg, BCRYPT_CHAINING_MODE,
                      (PUCHAR)BCRYPT_CHAIN_MODE_CBC,
                      sizeof(BCRYPT_CHAIN_MODE_CBC), 0);
    BCryptGenerateSymmetricKey(hAlg, &hKey, nullptr, 0,
                               (PUCHAR)key, KEY_SIZE, 0);
    ULONG result = 0;
    std::vector<uint8_t> ivCopy(iv, iv + IV_SIZE);
    BCryptEncrypt(hKey, (PUCHAR)data, (ULONG)len,
                  nullptr, ivCopy.data(), IV_SIZE,
                  out.data(), (ULONG)out.size(), &result,
                  BCRYPT_BLOCK_PADDING);
    out.resize(result);
    BCryptDestroyKey(hKey);
    BCryptCloseAlgorithmProvider(hAlg, 0);
#else
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv);
    EVP_EncryptUpdate(ctx, out.data(), &outLen1, data, (int)len);
    EVP_EncryptFinal_ex(ctx, out.data() + outLen1, &outLen2);
    EVP_CIPHER_CTX_free(ctx);
    out.resize(outLen1 + outLen2);
#endif
    return out;
}

// AES-256-CBC decrypt.
inline std::vector<uint8_t> aesDecrypt(const uint8_t* data, size_t len,
                                        const uint8_t* key, const uint8_t* iv) {
    std::vector<uint8_t> out(len);
    int outLen1 = 0, outLen2 = 0;
#ifdef _WIN32
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    BCRYPT_KEY_HANDLE hKey = nullptr;
    BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, nullptr, 0);
    BCryptSetProperty(hAlg, BCRYPT_CHAINING_MODE,
                      (PUCHAR)BCRYPT_CHAIN_MODE_CBC,
                      sizeof(BCRYPT_CHAIN_MODE_CBC), 0);
    BCryptGenerateSymmetricKey(hAlg, &hKey, nullptr, 0,
                               (PUCHAR)key, KEY_SIZE, 0);
    ULONG result = 0;
    std::vector<uint8_t> ivCopy(iv, iv + IV_SIZE);
    BCryptDecrypt(hKey, (PUCHAR)data, (ULONG)len,
                  nullptr, ivCopy.data(), IV_SIZE,
                  out.data(), (ULONG)out.size(), &result,
                  BCRYPT_BLOCK_PADDING);
    out.resize(result);
    BCryptDestroyKey(hKey);
    BCryptCloseAlgorithmProvider(hAlg, 0);
#else
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv);
    EVP_DecryptUpdate(ctx, out.data(), &outLen1, data, (int)len);
    EVP_DecryptFinal_ex(ctx, out.data() + outLen1, &outLen2);
    EVP_CIPHER_CTX_free(ctx);
    out.resize(outLen1 + outLen2);
#endif
    return out;
}

// -----------------------------------------------------------------------
// Compress / decompress (zlib deflate via miniz)
// -----------------------------------------------------------------------
inline std::vector<uint8_t> compress(const uint8_t* data, size_t len) {
    uLongf outLen = compressBound((uLongf)len);
    std::vector<uint8_t> out(outLen);
    if (::compress2(out.data(), &outLen,
                    data, (uLongf)len,
                    Z_BEST_COMPRESSION) != Z_OK)
        throw std::runtime_error("KonPak: compression failed");
    out.resize(outLen);
    return out;
}

inline std::vector<uint8_t> decompress(const uint8_t* data, size_t len,
                                        uint64_t originalSize) {
    std::vector<uint8_t> out(originalSize);
    uLongf outLen = (uLongf)originalSize;
    if (::uncompress(out.data(), &outLen,
                     data, (uLongf)len) != Z_OK)
        throw std::runtime_error("KonPak: decompression failed");
    return out;
}

// -----------------------------------------------------------------------
// Pack — the main archive object
// -----------------------------------------------------------------------
struct Pack {
    std::string password;
    std::vector<Entry> entries;

    // ---- Find an entry by path ----
    Entry* find(const std::string& path) {
        for (auto& e : entries)
            if (e.path == path) return &e;
        return nullptr;
    }

    const Entry* find(const std::string& path) const {
        for (auto& e : entries)
            if (e.path == path) return &e;
        return nullptr;
    }

    // ---- List all paths ----
    std::vector<std::string> list() const {
        std::vector<std::string> paths;
        for (auto& e : entries) paths.push_back(e.path);
        return paths;
    }

    // ---- Add or replace a file from disk ----
    void addFile(const std::string& diskPath, const std::string& packPath) {
        std::ifstream f(diskPath, std::ios::binary);
        if (!f.is_open())
            throw std::runtime_error("KonPak: cannot open: " + diskPath);
        std::vector<uint8_t> data(
            (std::istreambuf_iterator<char>(f)),
            std::istreambuf_iterator<char>());
        addData(packPath, data);
    }

    // ---- Add or replace from memory ----
    void addData(const std::string& packPath,
                 const std::vector<uint8_t>& data) {
        Entry* existing = find(packPath);
        if (existing) {
            existing->data    = data;
            existing->sizeRaw = data.size();
        } else {
            Entry e;
            e.path    = packPath;
            e.sizeRaw = data.size();
            e.data    = data;
            entries.push_back(std::move(e));
        }
    }

    // ---- Remove a file ----
    bool remove(const std::string& packPath) {
        auto it = std::find_if(entries.begin(), entries.end(),
            [&](const Entry& e) { return e.path == packPath; });
        if (it == entries.end()) return false;
        entries.erase(it);
        return true;
    }

    // ---- Extract one file to disk ----
    void extractFile(const std::string& packPath,
                     const std::string& diskPath) const {
        const Entry* e = find(packPath);
        if (!e) throw std::runtime_error("KonPak: not found: " + packPath);
        std::ofstream f(diskPath, std::ios::binary);
        f.write(reinterpret_cast<const char*>(e->data.data()), e->data.size());
    }

    // ---- Get raw bytes of a file (for engine use) ----
    const std::vector<uint8_t>& getData(const std::string& packPath) const {
        const Entry* e = find(packPath);
        if (!e) throw std::runtime_error("KonPak: not found: " + packPath);
        return e->data;
    }

    // ---- Save to .konpak file ----
    void save(const std::string& outPath) const {
        if (password.empty())
            throw std::runtime_error("KonPak: password not set");

        // Generate salt and IV
        uint8_t salt[SALT_SIZE], iv[IV_SIZE];
        randomBytes(salt, SALT_SIZE);
        randomBytes(iv, IV_SIZE);

        // Derive key
        uint8_t key[KEY_SIZE];
        deriveKey(password, salt, SALT_SIZE, key);

        // Build index + data blobs
        // Index format (before encryption):
        //   [uint32] entry count
        //   per entry:
        //     [uint16] path len + path bytes
        //     [uint64] sizeRaw
        //     [uint64] sizePacked (filled after we build data)

        // First pass: compress+encrypt all data, record sizes
        struct PackedEntry {
            std::string path;
            uint64_t    sizeRaw;
            std::vector<uint8_t> blob; // compressed+encrypted
        };

        std::vector<PackedEntry> packed;
        packed.reserve(entries.size());
        uint64_t dataOffset = 0;

        for (auto& e : entries) {
            PackedEntry pe;
            pe.path    = e.path;
            pe.sizeRaw = e.sizeRaw;
            // compress then encrypt
            auto compressed = compress(e.data.data(), e.data.size());
            pe.blob         = aesEncrypt(compressed.data(), compressed.size(),
                                         key, iv);
            packed.push_back(std::move(pe));
        }

        // Build index bytes
        std::vector<uint8_t> indexBytes;
        auto writeU16 = [&](uint16_t v) {
            indexBytes.push_back(v & 0xFF);
            indexBytes.push_back((v >> 8) & 0xFF);
        };
        auto writeU64 = [&](uint64_t v) {
            for (int i = 0; i < 8; i++)
                indexBytes.push_back((v >> (i*8)) & 0xFF);
        };
        auto writeU32 = [&](uint32_t v) {
            for (int i = 0; i < 4; i++)
                indexBytes.push_back((v >> (i*8)) & 0xFF);
        };

        writeU32((uint32_t)packed.size());
        uint64_t offset = 0;
        for (auto& pe : packed) {
            writeU16((uint16_t)pe.path.size());
            for (char c : pe.path) indexBytes.push_back((uint8_t)c);
            writeU64(pe.sizeRaw);
            writeU64((uint64_t)pe.blob.size());
            writeU64(offset);
            offset += pe.blob.size();
        }

        // Encrypt the index
        auto encIndex = aesEncrypt(indexBytes.data(), indexBytes.size(),
                                    key, iv);

        // Write file
        std::ofstream out(outPath, std::ios::binary);
        if (!out.is_open())
            throw std::runtime_error("KonPak: cannot write: " + outPath);

        // Header
        out.write(reinterpret_cast<const char*>(MAGIC), 4);
        out.write(reinterpret_cast<const char*>(&VERSION), 1);
        uint8_t flags = FLAG_ENC | FLAG_COMP;
        out.write(reinterpret_cast<const char*>(&flags), 1);
        out.write(reinterpret_cast<const char*>(salt), SALT_SIZE);
        out.write(reinterpret_cast<const char*>(iv), IV_SIZE);

        // Index size + encrypted index
        uint32_t idxSize = (uint32_t)encIndex.size();
        out.write(reinterpret_cast<const char*>(&idxSize), 4);
        out.write(reinterpret_cast<const char*>(encIndex.data()), encIndex.size());

        // Data blobs
        for (auto& pe : packed)
            out.write(reinterpret_cast<const char*>(pe.blob.data()),
                      pe.blob.size());
    }

    // ---- Load from .konpak file ----
    void load(const std::string& inPath) {
        if (password.empty())
            throw std::runtime_error("KonPak: password not set");

        std::ifstream in(inPath, std::ios::binary);
        if (!in.is_open())
            throw std::runtime_error("KonPak: cannot open: " + inPath);

        // Read header
        uint8_t magic[4];
        in.read(reinterpret_cast<char*>(magic), 4);
        if (memcmp(magic, MAGIC, 4) != 0)
            throw std::runtime_error("KonPak: not a .konpak file");

        uint8_t version, flags;
        in.read(reinterpret_cast<char*>(&version), 1);
        in.read(reinterpret_cast<char*>(&flags), 1);

        uint8_t salt[SALT_SIZE], iv[IV_SIZE];
        in.read(reinterpret_cast<char*>(salt), SALT_SIZE);
        in.read(reinterpret_cast<char*>(iv), IV_SIZE);

        // Derive key
        uint8_t key[KEY_SIZE];
        deriveKey(password, salt, SALT_SIZE, key);

        // Read + decrypt index
        uint32_t idxSize = 0;
        in.read(reinterpret_cast<char*>(&idxSize), 4);
        std::vector<uint8_t> encIndex(idxSize);
        in.read(reinterpret_cast<char*>(encIndex.data()), idxSize);
        auto indexBytes = aesDecrypt(encIndex.data(), encIndex.size(), key, iv);

        // Parse index
        size_t pos = 0;
        auto readU16 = [&]() -> uint16_t {
            uint16_t v = indexBytes[pos] | (indexBytes[pos+1] << 8);
            pos += 2; return v;
        };
        auto readU32 = [&]() -> uint32_t {
            uint32_t v = 0;
            for (int i = 0; i < 4; i++) v |= (uint32_t)indexBytes[pos+i] << (i*8);
            pos += 4; return v;
        };
        auto readU64 = [&]() -> uint64_t {
            uint64_t v = 0;
            for (int i = 0; i < 8; i++) v |= (uint64_t)indexBytes[pos+i] << (i*8);
            pos += 8; return v;
        };

        uint32_t count = readU32();
        struct IndexEntry { std::string path; uint64_t sizeRaw, sizePacked, offset; };
        std::vector<IndexEntry> idx(count);
        for (auto& ie : idx) {
            uint16_t pathLen = readU16();
            ie.path.resize(pathLen);
            for (char& c : ie.path) c = (char)indexBytes[pos++];
            ie.sizeRaw    = readU64();
            ie.sizePacked = readU64();
            ie.offset     = readU64();
        }

        // Remember where data section starts
        uint64_t dataStart = (uint64_t)in.tellg();

        // Read, decrypt, decompress each entry
        entries.clear();
        for (auto& ie : idx) {
            in.seekg((std::streamoff)(dataStart + ie.offset));
            std::vector<uint8_t> blob(ie.sizePacked);
            in.read(reinterpret_cast<char*>(blob.data()), ie.sizePacked);

            auto decompData = aesDecrypt(blob.data(), blob.size(), key, iv);
            auto raw        = decompress(decompData.data(), decompData.size(),
                                         ie.sizeRaw);
            Entry e;
            e.path    = ie.path;
            e.sizeRaw = ie.sizeRaw;
            e.sizePacked = ie.sizePacked;
            e.offset  = ie.offset;
            e.data    = std::move(raw);
            entries.push_back(std::move(e));
        }
    }

    // ---- Open using the compile-time baked key (KON_PACK_KEY) ----
    // Use this in release game builds so the player is never prompted.
    void openWithBuiltinKey(const std::string& path) {
        if (BUILTIN_KEY == nullptr)
            throw std::runtime_error(
                "KonPak: no builtin key -- compile with -DKON_PACK_KEY=...");
        password = BUILTIN_KEY;
        load(path);
    }

    // ---- Extract all files to a directory at startup ----
    // Useful pattern: extract everything once on launch, then use loose files.
    void extractAllTo(const std::string& outDir) const {
        for (auto& e : entries) {
            // Reconstruct directory structure
            std::string fullPath = outDir + "/" + e.path;
            // Create parent dirs
            std::string dir = fullPath;
            size_t pos = dir.rfind('/');
            if (pos != std::string::npos) {
                dir = dir.substr(0, pos);
                // mkdir -p equivalent (portable C++17)
                std::string tmp;
                for (char c : dir) {
                    tmp += c;
                    if (c == '/') {
                        #ifdef _WIN32
                        _mkdir(tmp.c_str());
                        #else
                        mkdir(tmp.c_str(), 0755);
                        #endif
                    }
                }
                #ifdef _WIN32
                _mkdir(dir.c_str());
                #else
                mkdir(dir.c_str(), 0755);
                #endif
            }
            extractFile(e.path, fullPath);
        }
    }

    // ---- Verify password without fully loading ----
    static bool checkPassword(const std::string& filePath,
                               const std::string& password) {
        try {
            Pack p;
            p.password = password;
            p.load(filePath);
            return true;
        } catch (...) {
            return false;
        }
    }
};

} // namespace KonPak
