#include "../include/lexer.hpp"
#include "../include/parser.hpp"
#include "../include/typechecker.hpp"
#include "../include/codegen.hpp"
#include "../include/irgen.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <climits>
#include <cstdlib>

#ifndef _WIN32
#include <unistd.h>
#endif

namespace fs = std::filesystem;

// -----------------------------------------------------------------------
// Find the directory containing the konscript binary itself.
// Used to locate the bundled toolchain from any working directory.
// -----------------------------------------------------------------------
static std::string selfDir() {
#ifdef _WIN32
    char buf[MAX_PATH];
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    return fs::path(buf).parent_path().string();
#else
    char buf[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len > 0) {
        buf[len] = '\0';
        return fs::path(buf).parent_path().string();
    }
    return ".";
#endif
}

// -----------------------------------------------------------------------
// Find the toolchain directory.
// Search order:
//   0. Path baked in at compile time by build.sh  (no env var needed)
//   1. KONSCRIPT_TOOLCHAIN env var
//   2. <binary_dir>/toolchain/               (installed layout)
//   3. <binary_dir>/../tools/KonScript/toolchain/  (repo dev layout)
//   4. ./toolchain/                          (CWD fallback)
// -----------------------------------------------------------------------
static std::string findToolchainDir() {
    // 0. Baked in at compile time — works from any directory, no env var needed
#ifdef KONSCRIPT_TOOLCHAIN_BUILTIN
    {
        std::string builtin = KONSCRIPT_TOOLCHAIN_BUILTIN;
        if (fs::exists(builtin + "/llvm/bin/llc"))
            return builtin;
    }
#endif

    // 1. Env var override (useful for CI or switching toolchains)
    const char* env = std::getenv("KONSCRIPT_TOOLCHAIN");
    if (env && fs::exists(std::string(env) + "/llvm/bin/llc"))
        return env;

    std::string self = selfDir();

    std::string next = self + "/toolchain";
    if (fs::exists(next + "/llvm/bin/llc")) return next;

    std::string repo = self + "/../tools/KonScript/toolchain";
    if (fs::exists(repo + "/llvm/bin/llc"))
        return fs::canonical(repo).string();

    if (fs::exists("toolchain/llvm/bin/llc"))
        return fs::canonical("toolchain").string();

    return "";
}

// -----------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------
static void usage() {
    std::cout <<
        "KonScript v0.3.0\n\n"
        "USAGE\n"
        "  konscript <file.ks>                    build native binary  (default)\n"
        "  konscript <file.ks> -o <name>          build with custom output name\n"
        "  konscript --target <platform> <file.ks> cross-compile\n"
        "      platforms: linux64  windows64  wasm32\n"
        "                 or any LLVM triple e.g. aarch64-linux-gnu\n\n"
        "OTHER MODES\n"
        "  konscript --cpp  <file.ks>             transpile → .cpp\n"
        "  konscript --llvm <file.ks>             emit → .ll (LLVM IR only)\n"
        "  konscript --ir   <file.ks>             print IR to stdout\n\n"
        "DEBUG\n"
        "  konscript --lex   <file.ks>            dump tokens\n"
        "  konscript --parse <file.ks>            dump AST\n"
        "  konscript --check <file.ks>            typecheck only\n\n"
        "ENVIRONMENT\n"
        "  KONSCRIPT_TOOLCHAIN=<dir>              override toolchain directory\n\n"
        "EXAMPLES\n"
        "  konscript hello.ks                     → ./hello\n"
        "  konscript game.ks -o mygame            → ./mygame\n"
        "  konscript --target windows64 game.ks   → ./game.exe\n"
        "  konscript --cpp game.ks                → game.cpp\n"
        "  konscript --llvm game.ks               → game.ll\n";
}

static std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "konscript: cannot open '" << path << "'\n";
        std::exit(1);
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static bool hasEngineInclude(const std::vector<KonScript::Token>& tokens) {
    for (size_t i = 0; i + 1 < tokens.size(); i++)
        if (tokens[i].type == KonScript::TokenType::Include &&
            tokens[i+1].value == "<engine>")
            return true;
    return false;
}

static KonScript::IRGen::Target resolveTarget(const std::string& name) {
    if (name == "linux64"   || name == "linux")   return KonScript::IRGen::Target::linux64();
    if (name == "windows64" || name == "windows") return KonScript::IRGen::Target::windows64();
    if (name == "wasm32"    || name == "wasm")    return KonScript::IRGen::Target::wasm32();
    KonScript::IRGen::Target t = KonScript::IRGen::Target::linux64();
    t.triple = name;
    return t;
}

// -----------------------------------------------------------------------
// AST printer
// -----------------------------------------------------------------------
static void printStmt(const KonScript::Stmt* s, int indent = 0) {
    std::string pad(indent * 2, ' ');
    if (!s) return;
    switch (s->kind) {
    case KonScript::Stmt::Kind::NodeDecl: {
        auto* n = static_cast<const KonScript::NodeDecl*>(s);
        std::cout << pad << "node " << n->name << " : " << n->base << "\n";
        for (auto& f : n->fields)
            std::cout << pad << "  field " << f.name << ": " << f.type.base
                      << (f.mut ? " (mut)" : "") << "\n";
        for (auto& m : n->methods)
            std::cout << pad << "  func " << m->name
                      << "(" << m->params.size() << " params)"
                      << (m->returnType ? " -> " + m->returnType->base : "")
                      << "\n";
        break;
    }
    case KonScript::Stmt::Kind::FuncDecl: {
        auto* f = static_cast<const KonScript::FuncDecl*>(s);
        std::cout << pad << "func " << f->name
                  << "(" << f->params.size() << " params)"
                  << (f->returnType ? " -> " + f->returnType->base : "")
                  << "\n";
        break;
    }
    case KonScript::Stmt::Kind::EnumDecl: {
        auto* e = static_cast<const KonScript::EnumDecl*>(s);
        std::cout << pad << "enum " << e->name << " (" << e->variants.size() << " variants)\n";
        for (auto& v : e->variants)
            std::cout << pad << "  " << v.name
                      << (v.payload ? "(" + v.payload->base + ")" : "") << "\n";
        break;
    }
    case KonScript::Stmt::Kind::StructDecl: {
        auto* st = static_cast<const KonScript::StructDecl*>(s);
        std::cout << pad << "struct " << st->name << " (" << st->fields.size() << " fields)\n";
        break;
    }
    case KonScript::Stmt::Kind::Include: {
        auto* i = static_cast<const KonScript::IncludeStmt*>(s);
        std::cout << pad << "#include "
                  << (i->isSystem ? "<" + i->path + ">" : "\"" + i->path + "\"") << "\n";
        break;
    }
    default:
        std::cout << pad << "stmt(kind=" << (int)s->kind << ")\n";
        break;
    }
}

// -----------------------------------------------------------------------
// Shared: run IRGen and write .ll, return path or ""
// -----------------------------------------------------------------------
static std::string runIRGen(
    const KonScript::Program& prog,
    const std::string& targetName,
    const std::string& outDir,
    const std::string& stem)
{
    KonScript::IRGen irgen;
    irgen.setTarget(resolveTarget(targetName));
    std::string ir = irgen.generate(prog);
    if (irgen.hasErrors()) {
        for (auto& e : irgen.errors())
            std::cerr << prog.filename << ":" << e.line << ": irgen: " << e.message << "\n";
        return "";
    }
    std::string llFile = outDir + "/" + stem + ".ll";
    std::ofstream f(llFile);
    if (!f.is_open()) { std::cerr << "konscript: cannot write " << llFile << "\n"; return ""; }
    f << ir;
    return llFile;
}

// -----------------------------------------------------------------------
// main
// -----------------------------------------------------------------------
int main(int argc, char** argv) {
    if (argc < 2) { usage(); return 0; }

    std::string first = argv[1];
    if (first == "--help" || first == "-h") { usage(); return 0; }

    // ── Parse flags ───────────────────────────────────────────────────────
    bool lexOnly   = false;
    bool parseOnly = false;
    bool checkOnly = false;
    bool cppMode   = false;
    bool llvmMode  = false;
    bool irDump    = false;

    std::string path;
    std::string outPath;
    std::string targetName = "linux64";

    std::vector<std::string> args(argv + 1, argv + argc);
    for (size_t i = 0; i < args.size(); i++) {
        if      (args[i] == "--lex")   { lexOnly   = true; }
        else if (args[i] == "--parse") { parseOnly = true; }
        else if (args[i] == "--check") { checkOnly = true; }
        else if (args[i] == "--cpp")   { cppMode   = true; }
        else if (args[i] == "--llvm")  { llvmMode  = true; }
        else if (args[i] == "--ir")    { irDump = true; llvmMode = true; }
        else if (args[i] == "--target" && i + 1 < args.size()) { targetName = args[++i]; }
        else if (args[i] == "-o"       && i + 1 < args.size()) { outPath    = args[++i]; }
        else if (args[i][0] != '-') { if (path.empty()) path = args[i]; }
    }

    if (path.empty()) { std::cerr << "konscript: no input file\n"; usage(); return 1; }

    std::string src = readFile(path);

    // ── Lex ───────────────────────────────────────────────────────────────
    KonScript::Lexer lexer(src, path);
    auto tokens = lexer.tokenize();
    if (lexer.hasErrors()) {
        for (auto& e : lexer.errors()) std::cerr << e.message << "\n";
        return 1;
    }
    if (lexOnly) {
        for (auto& t : tokens) {
            if (t.type == KonScript::TokenType::Eof) break;
            std::cout << t.line << ":" << t.col << "\t" << t.value << "\n";
        }
        return 0;
    }

    bool engineTarget = hasEngineInclude(tokens);

    // ── Parse ─────────────────────────────────────────────────────────────
    KonScript::Parser parser(std::move(tokens), path);
    auto prog = parser.parse();
    if (parser.hasErrors()) {
        for (auto& e : parser.errors()) std::cerr << e << "\n";
        return 1;
    }
    if (parseOnly) {
        std::cout << prog.stmts.size() << " top-level declarations:\n";
        for (auto& s : prog.stmts) printStmt(s.get(), 1);
        return 0;
    }

    // ── Typecheck ─────────────────────────────────────────────────────────
    KonScript::TypeChecker checker;
    checker.check(prog);
    if (checker.hasErrors()) {
        for (auto& e : checker.errors())
            std::cerr << path << ":" << e.line << ":" << e.col
                      << ": error: " << e.message << "\n";
        return 1;
    }
    if (checkOnly) { std::cout << path << ": OK\n"; return 0; }

    // ── --ir: print IR to stdout ───────────────────────────────────────────
    if (irDump) {
        KonScript::IRGen irgen;
        irgen.setTarget(resolveTarget(targetName));
        std::string ir = irgen.generate(prog);
        if (irgen.hasErrors()) {
            for (auto& e : irgen.errors())
                std::cerr << path << ":" << e.line << ": irgen: " << e.message << "\n";
            return 1;
        }
        std::cout << ir;
        return 0;
    }

    // ── --llvm: emit .ll only ──────────────────────────────────────────────
    if (llvmMode) {
        KonScript::IRGen irgen;
        irgen.setTarget(resolveTarget(targetName));
        std::string ir = irgen.generate(prog);
        if (irgen.hasErrors()) {
            for (auto& e : irgen.errors())
                std::cerr << path << ":" << e.line << ": irgen: " << e.message << "\n";
            return 1;
        }
        if (outPath.empty()) {
            outPath = path;
            auto dot = outPath.rfind('.');
            if (dot != std::string::npos) outPath = outPath.substr(0, dot);
            outPath += ".ll";
        }
        std::ofstream out(outPath);
        if (!out.is_open()) { std::cerr << "konscript: cannot write '" << outPath << "'\n"; return 1; }
        out << ir;
        std::cout << "[ir/" << targetName << "] " << path << " -> " << outPath << "\n";
        return 0;
    }

    // ── --cpp: C++ transpiler ──────────────────────────────────────────────
    if (cppMode) {
        KonScript::Codegen cg;
        cg.setTarget(engineTarget
            ? KonScript::Codegen::Target::Engine
            : KonScript::Codegen::Target::Standalone);
        std::string cpp = cg.generate(prog);
        if (cg.hasErrors()) {
            for (auto& e : cg.errors()) std::cerr << "codegen: " << e.message << "\n";
            return 1;
        }
        if (outPath.empty()) {
            outPath = path;
            auto dot = outPath.rfind('.');
            if (dot != std::string::npos) outPath = outPath.substr(0, dot);
            outPath += ".cpp";
        }
        std::ofstream out(outPath);
        if (!out.is_open()) { std::cerr << "konscript: cannot write '" << outPath << "'\n"; return 1; }
        out << cpp;
        std::cout << (engineTarget ? "[engine] " : "[standalone] ")
                  << path << " -> " << outPath << "\n";
        return 0;
    }

    // ── DEFAULT: full build → native binary ───────────────────────────────
    //
    //   konscript hello.ks        →  ./hello
    //   konscript --target windows64 game.ks  →  ./game.exe
    //
    std::string toolchainDir = findToolchainDir();

    // Output name from stem
    std::string stem = fs::path(path).stem().string();
    if (outPath.empty()) {
        outPath = stem;
        if (targetName == "windows64" || targetName == "windows") outPath += ".exe";
        else if (targetName == "wasm32" || targetName == "wasm")  outPath += ".wasm";
    }

    // Temp dir for .ll and .o
    std::string buildDir = (fs::temp_directory_path() / ("ks-" + stem)).string();
    fs::create_directories(buildDir);

    // IRGen → .ll
    std::string llFile = runIRGen(prog, targetName, buildDir, stem);
    if (llFile.empty()) return 1;

    std::string objFile = buildDir + "/" + stem + ".o";

    // Detect host OS for tool extensions
#ifdef _WIN32
    bool onWindows = true;
#else
    bool onWindows = false;
#endif
    std::string exe = onWindows ? ".exe" : "";

    // llc → .o
    std::string llc = toolchainDir.empty() ? "llc"
                    : toolchainDir + "/llvm/bin/llc" + exe;
    std::string llcCmd = "\"" + llc + "\""
        + " -filetype=obj"
        + " --mtriple=" + resolveTarget(targetName).triple
        + " \"" + llFile + "\""
        + " -o \"" + objFile + "\"";

    if (std::system(llcCmd.c_str()) != 0) {
        std::cerr << "konscript: llc failed\n"
                  << "  Toolchain: " << (toolchainDir.empty() ? "(system PATH)" : toolchainDir) << "\n"
                  << "  Run bundle-toolchain.sh to set up the bundled toolchain.\n";
        return 1;
    }

    // lld → binary
    bool isWindows = (targetName == "windows64" || targetName == "windows");
    bool isWasm    = (targetName == "wasm32"    || targetName == "wasm");

    std::string lld;
    if (!toolchainDir.empty()) {
        if (isWindows)     lld = toolchainDir + "/llvm/bin/lld-link" + exe;
        else if (isWasm)   lld = toolchainDir + "/llvm/bin/wasm-ld"  + exe;
        else               lld = toolchainDir + "/llvm/bin/ld.lld"   + exe;
    } else {
        lld = isWindows ? "lld-link" : "ld.lld";
    }

    std::string linkCmd;

    if (isWindows) {
        // Windows target: use lld-link (COFF/PE linker)
        // If we have a bundled sysroot use MinGW libs, otherwise bare link
        std::string sr = toolchainDir.empty() ? "" : toolchainDir + "/sysroot/windows64/lib";
        linkCmd = "\"" + lld + "\""
            + " /OUT:\"" + outPath + "\""
            + " /SUBSYSTEM:CONSOLE"
            + " \"" + objFile + "\"";
        if (!sr.empty() && fs::exists(sr + "/libmingwex.a")) {
            linkCmd += " \"" + sr + "/libmingwex.a\""
                     + " \"" + sr + "/libmsvcrt.a\""
                     + " \"" + sr + "/libkernel32.a\"";
        }
    } else if (isWasm) {
        linkCmd = "\"" + lld + "\""
            + " --export-all --no-entry"
            + " \"" + objFile + "\""
            + " -o \"" + outPath + "\"";
    } else if (!toolchainDir.empty()) {
        // Linux target with bundled musl — fully self-contained
        std::string sr = toolchainDir + "/sysroot/linux64/lib";
        linkCmd = "\"" + lld + "\""
            + " -static"
            + " \"" + sr + "/crt1.o\""
            + " \"" + sr + "/crti.o\""
            + " \"" + objFile + "\""
            + " \"" + sr + "/libc.a\""
            + " \"" + sr + "/libm.a\""
            + " \"" + sr + "/crtn.o\""
            + " -o \"" + outPath + "\"";
    } else {
        // No bundled toolchain — fall back to system linker
        linkCmd = "\"" + lld + "\""
            + " \"" + objFile + "\""
            + " -lm -lc"
            + " -o \"" + outPath + "\"";
    }

    if (std::system(linkCmd.c_str()) != 0) {
        std::cerr << "konscript: link failed\n";
#ifdef _WIN32
        std::cerr << "  Run: powershell -ExecutionPolicy Bypass -File bundle-toolchain.ps1\n";
#else
        std::cerr << "  Run bundle-toolchain.sh to set up the bundled toolchain.\n";
#endif
        return 1;
    }

    std::cout << "[build/" << targetName << "] " << path << " -> " << outPath << "\n";
    return 0;
}
