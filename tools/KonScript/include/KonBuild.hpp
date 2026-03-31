#pragma once
// ---------------------------------------------------------------------------
// KonBuild — self-contained KonScript project build orchestrator
//
// Requires NO external toolchain on the end user's machine.
// Everything lives inside the editor's install directory:
//
//   <editor>/
//     bin/KonEditor
//     toolchain/
//       llvm/
//         bin/llc          ← compiles .ll → .o
//         bin/ld.lld       ← links ELF (Linux)
//         bin/lld-link     ← links COFF (Windows cross-compile)
//         bin/wasm-ld      ← links WASM
//       sysroot/
//         linux64/
//           lib/crt1.o     ← musl C runtime startup
//           lib/crti.o
//           lib/crtn.o
//           lib/libc.a     ← musl libc (static)
//           lib/libm.a
//         windows64/
//           (MinGW/LLVM lib stubs for cross-linking)
//     templates/
//       KonEngine-linux64.a
//       KonEngine-windows64.lib
//       KonEngine-wasm32.a
//
// Build this layout with:  tools/KonScript/bundle-toolchain.sh
//
// ---------------------------------------------------------------------------
#include "irgen.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "typechecker.hpp"

#include <string>
#include <vector>
#include <functional>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstdio>

#ifdef _WIN32
  #include <windows.h>
#else
  #include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace KonScript {

class KonBuild {
public:
    // -----------------------------------------------------------------------
    // Target platform
    // -----------------------------------------------------------------------
    enum class Platform {
        Linux64,
        Windows64,   // cross-compile from Linux via LLVM
        WASM32,
        Custom,
    };

    // -----------------------------------------------------------------------
    // Build result
    // -----------------------------------------------------------------------
    struct Result {
        bool        success = false;
        std::string binary;
        std::vector<std::string> errors;
        std::vector<std::string> log;
    };

    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------
    void setProject(const std::string& dir)       { m_projectDir  = dir;  }
    void setOutputDir(const std::string& dir)     { m_outputDir   = dir;  }
    void setOutputName(const std::string& name)   { m_outputName  = name; }
    void setTarget(Platform p)                    { m_platform    = p;    }
    void setCustomTarget(const IRGen::Target& t)  { m_custom = t; m_platform = Platform::Custom; }
    void setOptimize(bool opt)                    { m_optimize    = opt;  }

    // Path to the <editor>/toolchain directory (set by KonEditor at startup)
    void setToolchainDir(const std::string& dir)  { m_toolchainDir = dir; }

    // Path to the <editor>/templates directory (export template .a/.lib files)
    void setTemplatesDir(const std::string& dir)  { m_templatesDir = dir; }

    // Stream each log line to the Build panel UI
    void setLogCallback(std::function<void(const std::string&)> cb) {
        m_logCb = std::move(cb);
    }

    // -----------------------------------------------------------------------
    // Validate the toolchain directory before attempting a build.
    // Returns empty string if OK, or an error message if something is missing.
    // -----------------------------------------------------------------------
    std::string validate() const {
        if (m_toolchainDir.empty())
            return "Toolchain directory not set. Run bundle-toolchain.sh first.";

        std::string llc = llcPath();
        if (!fs::exists(llc))
            return "llc not found at: " + llc
                 + "\nRun tools/KonScript/bundle-toolchain.sh to set up the toolchain.";

        std::string lld = lldPath();
        if (!fs::exists(lld))
            return "lld not found at: " + lld
                 + "\nRun tools/KonScript/bundle-toolchain.sh to set up the toolchain.";

        if (m_platform != Platform::Windows64 && m_platform != Platform::WASM32) {
            std::string crt = crtPath("crt1.o");
            if (!fs::exists(crt))
                return "musl sysroot not found at: " + crt
                     + "\nRun tools/KonScript/bundle-toolchain.sh to build the sysroot.";
        }

        return ""; // all good
    }

    // -----------------------------------------------------------------------
    // Run the build
    // -----------------------------------------------------------------------
    Result run() {
        Result result;

        if (m_projectDir.empty()) {
            result.errors.push_back("No project directory set.");
            return result;
        }
        if (m_outputDir.empty())
            m_outputDir = m_projectDir + "/build";
        if (m_outputName.empty())
            m_outputName = fs::path(m_projectDir).filename().string();

        // Validate toolchain first
        std::string valErr = validate();
        if (!valErr.empty()) {
            result.errors.push_back(valErr);
            return result;
        }

        fs::create_directories(m_outputDir);

        IRGen::Target target = resolveTarget();
        log("=== KonBuild ===");
        log("Project   : " + m_projectDir);
        log("Target    : " + target.triple);
        log("Toolchain : " + m_toolchainDir);
        log("Output    : " + outputPath());
        log("");

        // ---- Step 1: collect .ks files ----
        std::vector<fs::path> ksFiles;
        for (auto& e : fs::recursive_directory_iterator(m_projectDir))
            if (e.is_regular_file() && e.path().extension() == ".ks")
                ksFiles.push_back(e.path());

        if (ksFiles.empty()) {
            result.errors.push_back("No .ks files found in: " + m_projectDir);
            return result;
        }
        log("Found " + std::to_string(ksFiles.size()) + " .ks file(s)\n");

        // ---- Step 2 + 3: IRGen each .ks → .ll → .o ----
        std::vector<std::string> objFiles;
        bool anyError = false;

        for (auto& ksPath : ksFiles) {
            std::string stem    = ksPath.stem().string();
            std::string llFile  = m_outputDir + "/" + stem + ".ll";
            std::string objFile = m_outputDir + "/" + stem + ".o";

            log("[compile] " + ksPath.filename().string());

            // Read source
            std::string src;
            {
                std::ifstream f(ksPath);
                if (!f.is_open()) {
                    err(result, "cannot open: " + ksPath.string());
                    anyError = true; continue;
                }
                std::ostringstream ss; ss << f.rdbuf(); src = ss.str();
            }

            // Lex
            Lexer lexer(src, ksPath.string());
            auto tokens = lexer.tokenize();
            if (lexer.hasErrors()) {
                for (auto& e2 : lexer.errors()) err(result, e2.message);
                anyError = true; continue;
            }

            // Parse
            Parser parser(std::move(tokens), ksPath.string());
            auto prog = parser.parse();
            if (parser.hasErrors()) {
                for (auto& e2 : parser.errors()) err(result, e2);
                anyError = true; continue;
            }

            // Typecheck
            TypeChecker checker;
            checker.check(prog);
            if (checker.hasErrors()) {
                for (auto& e2 : checker.errors())
                    err(result, ksPath.string() + ":" + std::to_string(e2.line)
                              + ":" + std::to_string(e2.col) + ": " + e2.message);
                anyError = true; continue;
            }

            // IRGen → .ll
            IRGen irgen;
            irgen.setTarget(target);
            std::string ir = irgen.generate(prog);
            if (irgen.hasErrors()) {
                for (auto& e2 : irgen.errors())
                    err(result, ksPath.string() + ":" + std::to_string(e2.line)
                              + ": " + e2.message);
                anyError = true; continue;
            }
            {
                std::ofstream f(llFile);
                if (!f.is_open()) { err(result, "cannot write: " + llFile); anyError=true; continue; }
                f << ir;
            }
            log("  → " + llFile);

            // llc: .ll → .o
            std::string optFlag = m_optimize ? "-O2" : "-O0";
            std::string llcCmd  = q(llcPath())
                + " -filetype=obj"
                + " --mtriple=" + target.triple
                + " " + optFlag
                + " " + q(llFile)
                + " -o " + q(objFile);
            log("  $ " + llcCmd);
            if (shell(llcCmd) != 0) {
                err(result, "llc failed for " + ksPath.filename().string());
                anyError = true; continue;
            }
            objFiles.push_back(objFile);
            log("  → " + objFile + "\n");
        }

        if (anyError) { log("\nBuild FAILED."); return result; }

        // ---- Step 4: link ----
        log("[link] " + outputPath());
        std::string linkCmd = buildLinkCmd(objFiles, target);
        log("  $ " + linkCmd);
        if (shell(linkCmd) != 0) {
            err(result, "link failed");
            log("\nBuild FAILED.");
            return result;
        }

        log("\nBuild OK  →  " + outputPath());
        result.success = true;
        result.binary  = outputPath();
        result.log     = m_logLines;
        return result;
    }

private:
    // ── Config ────────────────────────────────────────────────────────────
    std::string   m_projectDir;
    std::string   m_outputDir;
    std::string   m_outputName;
    std::string   m_toolchainDir;
    std::string   m_templatesDir;
    Platform      m_platform  = Platform::Linux64;
    IRGen::Target m_custom;
    bool          m_optimize  = false;
    std::function<void(const std::string&)> m_logCb;
    std::vector<std::string> m_logLines;

    // ── Logging ───────────────────────────────────────────────────────────
    void log(const std::string& line) {
        m_logLines.push_back(line);
        if (m_logCb) m_logCb(line);
    }
    void err(Result& r, const std::string& msg) {
        r.errors.push_back(msg);
        log("  ERROR: " + msg);
    }

    // ── Path helpers ──────────────────────────────────────────────────────
    // Bundled tool paths — everything relative to m_toolchainDir

    std::string llcPath() const {
        return m_toolchainDir + "/llvm/bin/llc";
    }

    std::string lldPath() const {
        switch (m_platform) {
        case Platform::Windows64: return m_toolchainDir + "/llvm/bin/lld-link";
        case Platform::WASM32:    return m_toolchainDir + "/llvm/bin/wasm-ld";
        default:                  return m_toolchainDir + "/llvm/bin/ld.lld";
        }
    }

    std::string sysrootDir() const {
        switch (m_platform) {
        case Platform::Windows64: return m_toolchainDir + "/sysroot/windows64";
        case Platform::WASM32:    return m_toolchainDir + "/sysroot/wasm32";
        default:                  return m_toolchainDir + "/sysroot/linux64";
        }
    }

    std::string crtPath(const std::string& file) const {
        return sysrootDir() + "/lib/" + file;
    }

    std::string exportTemplate() const {
        if (m_templatesDir.empty()) return "";
        switch (m_platform) {
        case Platform::Windows64: return m_templatesDir + "/KonEngine-windows64.lib";
        case Platform::WASM32:    return m_templatesDir + "/KonEngine-wasm32.a";
        default:                  return m_templatesDir + "/KonEngine-linux64.a";
        }
    }

    std::string outputPath() const {
        std::string name = m_outputName;
        switch (m_platform) {
        case Platform::Windows64: return m_outputDir + "/" + name + ".exe";
        case Platform::WASM32:    return m_outputDir + "/" + name + ".wasm";
        default:                  return m_outputDir + "/" + name;
        }
    }

    // Quote a path for the shell
    static std::string q(const std::string& p) { return "\"" + p + "\""; }

    // ── Target ────────────────────────────────────────────────────────────
    IRGen::Target resolveTarget() const {
        switch (m_platform) {
        case Platform::Linux64:   return IRGen::Target::linux64();
        case Platform::Windows64: return IRGen::Target::windows64();
        case Platform::WASM32:    return IRGen::Target::wasm32();
        case Platform::Custom:    return m_custom;
        default:                  return IRGen::Target::linux64();
        }
    }

    // ── Link command ──────────────────────────────────────────────────────
    std::string buildLinkCmd(
        const std::vector<std::string>& objs,
        const IRGen::Target& target) const
    {
        std::string tmpl = exportTemplate();
        bool hasEngine   = !tmpl.empty() && fs::exists(tmpl);
        std::string cmd;

        if (m_platform == Platform::Windows64) {
            // lld-link: COFF linker, same CLI as link.exe
            cmd = q(lldPath());
            cmd += " /OUT:" + q(outputPath());
            cmd += " /SUBSYSTEM:CONSOLE";
            cmd += " /NODEFAULTLIB";
            for (auto& o : objs) cmd += " " + q(o);
            if (hasEngine) cmd += " " + q(tmpl);
            // MinGW import libs for OpenGL + Windows API
            std::string wlib = sysrootDir() + "/lib";
            for (auto& lib : {"opengl32.lib","gdi32.lib","winmm.lib","user32.lib",
                               "kernel32.lib","msvcrt.lib"})
                if (fs::exists(wlib + "/" + lib))
                    cmd += " " + q(wlib + "/" + lib);

        } else if (m_platform == Platform::WASM32) {
            cmd = q(lldPath());
            cmd += " --export-all --no-entry --allow-undefined";
            for (auto& o : objs) cmd += " " + q(o);
            if (hasEngine) cmd += " " + q(tmpl);
            cmd += " -o " + q(outputPath());

        } else {
            // ELF (Linux64) — fully static binary using bundled musl.
            // -static: no dynamic linker needed, runs on any Linux kernel.
            std::string sr = sysrootDir();

            cmd = q(lldPath());
            cmd += " -static";

            // musl CRT startup
            cmd += " " + q(sr + "/lib/crt1.o");
            cmd += " " + q(sr + "/lib/crti.o");

            // Object files from user's game
            for (auto& o : objs) cmd += " " + q(o);

            // KonEngine export template (if present)
            if (hasEngine) cmd += " " + q(tmpl);

            // musl libc + libm
            cmd += " " + q(sr + "/lib/libc.a");
            cmd += " " + q(sr + "/lib/libm.a");

            cmd += " " + q(sr + "/lib/crtn.o");
            cmd += " -o " + q(outputPath());
        }

        return cmd;
    }

    // ── Shell subprocess ──────────────────────────────────────────────────
    int shell(const std::string& cmd) {
#ifdef _WIN32
        return std::system(cmd.c_str());
#else
        FILE* p = popen((cmd + " 2>&1").c_str(), "r");
        if (!p) return -1;
        char buf[512];
        while (fgets(buf, sizeof(buf), p)) {
            std::string line(buf);
            if (!line.empty() && line.back() == '\n') line.pop_back();
            log("    " + line);
        }
        return pclose(p);
#endif
    }
};

} // namespace KonScript
