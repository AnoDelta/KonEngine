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
#include <algorithm>
#include <filesystem>
#include <functional>
#include <unordered_set>
#include <climits>
#include <cstdlib>
#include <ctime>
#include <time.h>

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
        "  konscript --check <file.ks>            typecheck only\n"
        "  konscript --test [file.ks]              run built-in test suite\n\n"
        "ENVIRONMENT\n"
        "  KONSCRIPT_TOOLCHAIN=<dir>              override toolchain directory\n\n"
        "EXAMPLES\n"
        "  konscript hello.ks                     → ./hello\n"
        "  konscript game.ks -o mygame            → ./mygame\n"
        "  konscript --target windows64 game.ks   → ./game.exe\n"
        "  konscript --cpp game.ks                → game.cpp\n"
        "  konscript --llvm game.ks               → game.ll\n";
}

// -----------------------------------------------------------------------
// Progress bar helpers
// -----------------------------------------------------------------------
// ANSI color codes
#define KS_RESET   "\033[0m"
#define KS_BOLD    "\033[1m"
#define KS_GREEN   "\033[32m"
#define KS_YELLOW  "\033[33m"
#define KS_CYAN    "\033[36m"
#define KS_DIM     "\033[2m"

// Print a completed stage bar:
//   [1/4] Lexing         ████████████████████  0.3ms
static void printStageOK(int step, int total, const std::string& name,
                         double ms, bool isLast = false) {
    // Bar: 20 filled blocks
    const int W = 20;
    std::string bar(W, '\0');
    for (int i = 0; i < W; i++) bar[i] = '\xe2'; // UTF-8 for ████
    // Actually use ASCII-safe blocks
    std::string filled(W, '#');
    std::string empty(0,  '-');

    // Format step label
    char stepBuf[8];
    std::snprintf(stepBuf, sizeof(stepBuf), "[%d/%d]", step, total);

    // Pad name to 16 chars
    std::string padded = name;
    while ((int)padded.size() < 16) padded += ' ';

    // Time string
    char timeBuf[32];
    if (ms < 1000.0)
        std::snprintf(timeBuf, sizeof(timeBuf), "%.1fms", ms);
    else
        std::snprintf(timeBuf, sizeof(timeBuf), "%.2fs", ms / 1000.0);

    std::cout << KS_DIM << stepBuf << KS_RESET
              << " " << KS_BOLD << padded << KS_RESET
              << " " << KS_GREEN;
    // Print filled blocks (unicode full block ▓)
    for (int i = 0; i < W; i++) std::cout << "\xe2\x96\x93";
    std::cout << KS_RESET
              << "  " << KS_DIM << timeBuf << KS_RESET
              << "\n";
    std::cout << std::flush;
}

// Print an in-progress stage (partial bar with spinner feel)
static void printStageDoing(int step, int total, const std::string& name) {
    const int W = 20;
    char stepBuf[8];
    std::snprintf(stepBuf, sizeof(stepBuf), "[%d/%d]", step, total);
    std::string padded = name;
    while ((int)padded.size() < 16) padded += ' ';

    std::cout << KS_DIM << stepBuf << KS_RESET
              << " " << KS_BOLD << padded << KS_RESET
              << " " << KS_YELLOW;
    for (int i = 0; i < W / 2; i++) std::cout << "\xe2\x96\x91"; // light blocks
    for (int i = W / 2; i < W; i++) std::cout << "\xe2\x96\x91";
    std::cout << KS_RESET << "  ...\r" << std::flush;
}

static double msNow() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
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
    std::vector<KonScript::Program> _irIncludes;
    std::unordered_set<std::string> _irVis;
    std::function<void(const KonScript::Program&, const std::string&)> _colIR;
    _colIR = [&](const KonScript::Program& p, const std::string& base) {
        for (auto& s : p.stmts) {
            if (s->kind != KonScript::Stmt::Kind::Include) continue;
            auto* inc = static_cast<const KonScript::IncludeStmt*>(s.get());
            if (inc->isSystem || inc->path == "engine") continue;
            std::string ip = inc->path;
            if (!fs::path(ip).is_absolute())
                ip = fs::path(base).parent_path().string() + "/" + inc->path;
            ip = fs::weakly_canonical(ip).string();
            if (!fs::exists(ip) || _irVis.count(ip)) continue;
            _irVis.insert(ip);
            std::string _s = readFile(ip);
            KonScript::Lexer _lx(_s, ip); auto _t = _lx.tokenize();
            KonScript::Parser _px(std::move(_t), ip);
            auto _p = _px.parse();
            _colIR(_p, ip);
            _irIncludes.push_back(std::move(_p));
        }
    };
    _colIR(prog, prog.filename);
    KonScript::IRGen irgen;
    irgen.setTarget(resolveTarget(targetName));
    for (auto& _inc : _irIncludes) irgen.addInclude(_inc);
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
    bool testMode  = false;

    std::string path;
    std::string outPath;
    std::string targetName = "linux64";

    std::vector<std::string> args(argv + 1, argv + argc);
    for (size_t i = 0; i < args.size(); i++) {
        if      (args[i] == "--lex")    { lexOnly   = true; }
        else if (args[i] == "--parse")  { parseOnly = true; }
        else if (args[i] == "--check")  { checkOnly = true; }
        else if (args[i] == "--cpp")    { cppMode   = true; }
        else if (args[i] == "--llvm")   { llvmMode  = true; }
        else if (args[i] == "--ir")     { irDump = true; llvmMode = true; }
        else if (args[i] == "--test")   { testMode  = true; }
        else if (args[i] == "--target" && i + 1 < args.size()) { targetName = args[++i]; }
        else if (args[i] == "-o"       && i + 1 < args.size()) { outPath    = args[++i]; }
        else if (args[i][0] != '-') { if (path.empty()) path = args[i]; }
    }

    // ── Built-in test runner ──────────────────────────────────────────────
    if (testMode) {
        std::string self = selfDir();
        std::string toolchain = findToolchainDir();
        std::string toolchainParent = toolchain.empty() ? "" :
            fs::path(toolchain).parent_path().string();

        // Look for test files in several locations in priority order
        std::vector<std::string> testDirs = {
            self + "/tests",                                    // next to binary
            toolchainParent + "/tests",                         // next to toolchain
            toolchainParent + "/../tests",                      // one level up
            fs::current_path().string() + "/tests",             // CWD/tests
            fs::current_path().string(),                        // CWD itself (*.ks)
        };
        // Also accept a specific test file on the command line
        std::vector<std::string> testFiles;
        if (!path.empty()) {
            testFiles.push_back(path);
        } else {
            for (auto& td : testDirs) {
                if (fs::exists(td)) {
                    for (auto& entry : fs::directory_iterator(td)) {
                        if (entry.path().extension() == ".ks")
                            testFiles.push_back(entry.path().string());
                    }
                    break;
                }
            }
        }
        if (testFiles.empty()) {
            std::cerr << "konscript --test: no test files found.\n";
            std::cerr << "  Place .ks test files in " << self << "/tests/\n";
            std::cerr << "  Or run: konscript --test <file.ks>\n";
            return 1;
        }
        std::sort(testFiles.begin(), testFiles.end());
        int totalPass = 0, totalFail = 0;
        std::cout << "\n=== KonScript Built-in Test Runner ===\n\n";
        for (auto& tf : testFiles) {
            std::string name = fs::path(tf).filename().string();
            std::cout << "[RUN] " << name << "\n";
            // Build
            std::string binPath = "/tmp/ks_test_" + std::to_string(std::time(nullptr));
            std::string buildCmd = argv[0] + std::string(" \"") + tf + "\" -o \"" + binPath + "\"";
            int buildRet = std::system(buildCmd.c_str());
            if (buildRet != 0) {
                std::cout << "[FAIL] " << name << " — build failed\n";
                totalFail++;
                continue;
            }
            // Run
            int runRet = std::system(("\"" + binPath + "\"").c_str());
            std::remove(binPath.c_str());
            if (runRet == 0) {
                std::cout << "[PASS] " << name << "\n";
                totalPass++;
            } else {
                std::cout << "[FAIL] " << name << " — exit code " << runRet << "\n";
                totalFail++;
            }
        }
        std::cout << "\n=== Results: " << totalPass << " passed, " << totalFail << " failed ===\n";
        return totalFail > 0 ? 1 : 0;
    }

    if (path.empty()) { std::cerr << "konscript: no input file\n"; usage(); return 1; }

    std::string src = readFile(path);

    // ── Lex ───────────────────────────────────────────────────────────────
    printStageDoing(1, 4, "Lexing");
    double t0 = msNow();
    KonScript::Lexer lexer(src, path);
    auto tokens = lexer.tokenize();
    printStageOK(1, 4, "Lexing", msNow() - t0);
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
    printStageDoing(2, 4, "Parsing");
    double t1 = msNow();
    KonScript::Parser parser(std::move(tokens), path);
    auto prog = parser.parse();
    printStageOK(2, 4, "Parsing", msNow() - t1);
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
    // Pre-register symbols from included .ks files
    KonScript::TypeChecker checker;
    {
        std::unordered_set<std::string> tcVisited;
        std::function<void(const KonScript::Program&, const std::string&)> prepass;
        prepass = [&](const KonScript::Program& p, const std::string& base) {
            for (auto& s : p.stmts) {
                if (s->kind != KonScript::Stmt::Kind::Include) continue;
                auto* inc = static_cast<const KonScript::IncludeStmt*>(s.get());
                if (inc->isSystem || inc->path == "engine") continue;
                std::string incPath = inc->path;
                if (!fs::path(incPath).is_absolute())
                    incPath = fs::path(base).parent_path().string() + "/" + incPath;
                incPath = fs::weakly_canonical(incPath).string();
                if (!fs::exists(incPath) || tcVisited.count(incPath)) continue;
                tcVisited.insert(incPath);
                std::string incSrc = readFile(incPath);
                KonScript::Lexer lx(incSrc, incPath);
                auto toks = lx.tokenize();
                KonScript::Parser px(std::move(toks), incPath);
                auto incProg = px.parse();
                prepass(incProg, incPath);
                checker.addInclude(incProg);
            }
        };
        prepass(prog, path);
    }
    printStageDoing(3, 4, "Type checking");
    double t2 = msNow();
    checker.check(prog);
    printStageOK(3, 4, "Type checking", msNow() - t2);
    if (checker.hasErrors()) {
        for (auto& e : checker.errors())
            std::cerr << path << ":" << e.line << ":" << e.col
                      << ": error: " << e.message << "\n";
        return 1;
    }
    if (checkOnly) { std::cout << path << ": OK\n"; return 0; }

    // Print source file summary for multi-file builds
    {
        std::vector<std::string> _srcList;
        std::unordered_set<std::string> _srcVis;
        std::function<void(const KonScript::Program&, const std::string&)> _cntInc;
        _cntInc = [&](const KonScript::Program& p, const std::string& base) {
            for (auto& s : p.stmts) {
                if (s->kind != KonScript::Stmt::Kind::Include) continue;
                auto* inc = static_cast<const KonScript::IncludeStmt*>(s.get());
                if (inc->isSystem || inc->path == "engine") continue;
                std::string ip = inc->path;
                if (!fs::path(ip).is_absolute())
                    ip = fs::path(base).parent_path().string() + "/" + inc->path;
                ip = fs::weakly_canonical(ip).string();
                if (!fs::exists(ip) || _srcVis.count(ip)) continue;
                _srcVis.insert(ip);
                _srcList.push_back(fs::path(ip).filename().string());
            }
        };
        _cntInc(prog, path);
        if (!_srcList.empty()) {
            std::cout << "\n" << KS_DIM << "  Sources (" << (_srcList.size()+1) << " files):\n" << KS_RESET;
            for (auto& f : _srcList)
                std::cout << KS_DIM << "    " << f << "\n" << KS_RESET;
            std::cout << KS_DIM << "    " << fs::path(path).filename().string()
                      << "  " << KS_CYAN << "(entry)" << KS_RESET << "\n\n";
        }
    }

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
        printStageDoing(4, 4, "IRGen");
        double tIR = msNow();
        out << ir;
        printStageOK(4, 4, "IRGen", msNow() - tIR);
        std::cout << "\n" << KS_BOLD << KS_GREEN << "  ✓ " << KS_RESET
                  << KS_BOLD << outPath << KS_RESET
                  << KS_DIM << "  [ir/" << targetName << "]" << KS_RESET << "\n\n";
        return 0;
    }

    // ── --cpp: C++ transpiler ──────────────────────────────────────────────
    if (cppMode) {
        KonScript::Codegen cg;
        cg.setTarget(engineTarget
            ? KonScript::Codegen::Target::Engine
            : KonScript::Codegen::Target::Standalone);
        cg.setRewriteKsIncludes(true); // cmake/--cpp mode: rewrite .ks → .ks.cpp
        // Pre-register types from #include'd .ks files so cross-file structs,
        // classes, and enums are value types instead of unknown pointers.
        {
            std::unordered_set<std::string> cgVis;
            std::function<void(const KonScript::Program&, const std::string&)> cgPre;
            cgPre = [&](const KonScript::Program& p, const std::string& base) {
                for (auto& s : p.stmts) {
                    if (s->kind != KonScript::Stmt::Kind::Include) continue;
                    auto* inc = static_cast<const KonScript::IncludeStmt*>(s.get());
                    if (inc->isSystem || inc->path == "engine") continue;
                    std::string ip = inc->path;
                    if (!fs::path(ip).is_absolute())
                        ip = fs::path(base).parent_path().string() + "/" + ip;
                    ip = fs::weakly_canonical(ip).string();
                    if (!fs::exists(ip) || cgVis.count(ip)) continue;
                    cgVis.insert(ip);
                    KonScript::Lexer lx(readFile(ip), ip);
                    auto toks = lx.tokenize();
                    KonScript::Parser px(std::move(toks), ip);
                    auto incProg = px.parse();
                    cgPre(incProg, ip);
                    cg.addIncludeTypes(incProg);
                }
            };
            cgPre(prog, path);
        }
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
        printStageDoing(4, 4, "Transpiling");
        double tCpp = msNow();
        out << cpp;
        printStageOK(4, 4, "Transpiling", msNow() - tCpp);
        std::string tgtLabel = engineTarget ? "engine" : "standalone";
        std::cout << "\n" << KS_BOLD << KS_GREEN << "  ✓ " << KS_RESET
                  << KS_BOLD << outPath << KS_RESET
                  << KS_DIM << "  [" << tgtLabel << "]" << KS_RESET << "\n\n";

        // Also transpile each included .ks file to .ks.cpp next to main
        std::string cppDir = fs::path(outPath).parent_path().string();
        for (auto& s : prog.stmts) {
            if (s->kind != KonScript::Stmt::Kind::Include) continue;
            auto* inc = static_cast<const KonScript::IncludeStmt*>(s.get());
            if (inc->isSystem || inc->path == "engine") continue;
            std::string incPath = inc->path;
            if (!fs::path(incPath).is_absolute())
                incPath = fs::path(path).parent_path().string() + "/" + incPath;
            if (!fs::exists(incPath)) continue;
            std::string incSrc = readFile(incPath);
            KonScript::Lexer lx(incSrc, incPath);
            auto toks = lx.tokenize();
            KonScript::Parser px(std::move(toks), incPath);
            auto incProg = px.parse();
            KonScript::Codegen cgInc;
            cgInc.setTarget(KonScript::Codegen::Target::Engine);
            std::string incCppSrc = cgInc.generate(incProg);
            std::string incOutPath = cppDir + "/" +
                fs::path(incPath).filename().string() + ".cpp";
            std::ofstream fo(incOutPath);
            if (fo.is_open()) {
                fo << incCppSrc;
                std::cout << "[engine] " << incPath << " -> " << incOutPath << "\n";
            }
        }
        return 0;
    }

    // ── DEFAULT: full build → native binary ───────────────────────────────
    //
    //   konscript hello.ks              →  ./hello      (standalone, IRGen)
    //   konscript game.ks               →  ./game       (engine, clang++ pipeline)
    //   konscript --target windows64 game.ks  →  ./game.exe
    //
    std::string toolchainDir = findToolchainDir();

    // Engine games: transpile to C++ and compile with bundled clang++
    // This avoids needing cmake or g++ on the user's machine.
    if (engineTarget && !cppMode) {
        std::string stem = fs::path(path).stem().string();
        std::string srcDir = fs::path(path).parent_path().string();
        if (outPath.empty()) outPath = stem;
        if (targetName == "windows64" || targetName == "windows") {
            if (outPath.size() < 4 || outPath.substr(outPath.size()-4) != ".exe")
                outPath += ".exe";
        }

        std::string tmpDir = fs::path(path).parent_path().string() + "/.konbuild/" + stem;
        fs::create_directories(tmpDir);

        // Collect all .ks files to compile: main + any #include "*.ks" dependencies
        // Process them in order: includes first, then the main file
        std::vector<std::pair<std::string, std::string>> toCompile; // {ks path, cpp path}
        std::vector<std::string> allCppFiles;

        // Walk includes recursively, collecting all .ks files in order
        std::vector<std::string> ksIncludes;
        std::unordered_set<std::string> visited;

        std::function<void(const KonScript::Program&, const std::string&)> collectIncludes;
        collectIncludes = [&](const KonScript::Program& p, const std::string& base) {
            for (auto& s : p.stmts) {
                if (s->kind != KonScript::Stmt::Kind::Include) continue;
                auto* inc = static_cast<const KonScript::IncludeStmt*>(s.get());
                if (inc->isSystem || inc->path == "engine") continue;
                std::string incPath = inc->path;
                // Always resolve relative to the including file's directory
                if (!fs::path(incPath).is_absolute())
                    incPath = fs::path(base).parent_path().string() + "/" + incPath;
                incPath = fs::weakly_canonical(incPath).string();
                if (!fs::exists(incPath) || visited.count(incPath)) continue;
                visited.insert(incPath);
                // Parse this file and recurse into its includes first
                std::string incSrc = readFile(incPath);
                KonScript::Lexer lx(incSrc, incPath);
                auto toks = lx.tokenize();
                KonScript::Parser px(std::move(toks), incPath);
                auto incProg = px.parse();
                collectIncludes(incProg, incPath); // depth-first
                ksIncludes.push_back(incPath);
                // Also register with typechecker
                checker.addInclude(incProg);
            }
        };
        collectIncludes(prog, path);

        // Transpile each included .ks file in order (dependencies first)
        int _nFiles = (int)ksIncludes.size() + 1;
        int _fIdx = 0;
        for (auto& incPath : ksIncludes) {
            _fIdx++;
            std::string incStem = fs::path(incPath).stem().string();
            printStageDoing(_fIdx, _nFiles, incStem);
            double _tF = msNow();
            std::string incCpp  = tmpDir + "/" + incStem + ".cpp";
            std::string incSrc  = readFile(incPath);
            KonScript::Lexer lx(incSrc, incPath);
            auto toks = lx.tokenize();
            KonScript::Parser px(std::move(toks), incPath);
            auto incProg = px.parse();
            KonScript::Codegen cgInc;
            cgInc.setTarget(KonScript::Codegen::Target::Engine);
            std::string incCppSrc = cgInc.generate(incProg);
            { std::ofstream f(incCpp); f << incCppSrc; }
            allCppFiles.push_back(incCpp);
            printStageOK(_fIdx, _nFiles, incStem, msNow() - _tF);
        }

        // Transpile main file — codegen skips .ks includes automatically
        KonScript::Codegen cg;
        cg.setTarget(KonScript::Codegen::Target::Engine);
        std::string cppSrc = cg.generate(prog);
        if (cg.hasErrors()) {
            for (auto& e : cg.errors()) std::cerr << "codegen: " << e.message << "\n";
            return 1;
        }
        std::string mainCpp = tmpDir + "/" + stem + ".cpp";
        { std::ofstream f(mainCpp); f << cppSrc; }
        allCppFiles.push_back(mainCpp);

        // Unity build: concatenate all generated .cpp files into one translation unit
        // This lets main.cpp see Player/Box/etc. types from the included files.
        // Strip #pragma once and duplicate standard includes from non-first files.
        std::string unityFile = tmpDir + "/__unity.cpp";
        {
            std::ofstream unity(unityFile);
            bool first = true;
            for (auto& cf : allCppFiles) {
                std::ifstream in(cf);
                std::string line;
                while (std::getline(in, line)) {
                    // In non-first files, skip pragma once and redundant engine/std includes
                    if (!first) {
                        if (line.find("#pragma once") != std::string::npos) continue;
                        if (line.find("#include \"KonEngine.hpp\"") != std::string::npos) continue;
                        if (line.find("#include <string>") != std::string::npos) continue;
                        if (line.find("#include <vector>") != std::string::npos) continue;
                        if (line.find("#include <functional>") != std::string::npos) continue;
                        if (line.find("#include <iostream>") != std::string::npos) continue;
                        if (line.find("#include <optional>") != std::string::npos) continue;
                        if (line.find("#include <fstream>") != std::string::npos) continue;
                        if (line.find("#include <unordered_map>") != std::string::npos) continue;
                        if (line.find("#include <algorithm>") != std::string::npos) continue;
                        if (line.find("namespace { struct _KSInit") != std::string::npos) continue;
                        if (line.find("_KSInit() {") != std::string::npos) continue;
                        if (line.find("} _ks_init; }") != std::string::npos) continue;
                        // Strip _ks_* stdlib helpers from non-first files
                        if (line.find("_KsResult") != std::string::npos) continue;
                        if (line.find("_ks_file_") != std::string::npos) continue;
                        if (line.find("_ks_str_") != std::string::npos) continue;
                        if (line.find("_ks_has(") != std::string::npos) continue;
                        if (line.find("template<typename C") != std::string::npos) continue;
                        if (line.find("template<typename K,typename MV") != std::string::npos) continue;
                    }
                    unity << line << "\n";
                }
                first = false;
            }
        }

        // Determine target early — needed for compiler selection
        bool isWin = (targetName == "windows64" || targetName == "windows");

        // Locate clang++ — use system clang++ for Linux (bundled lacks resource dir)
        std::string clangpp;
        if (!isWin) {
            FILE* pp = popen("command -v clang++ 2>/dev/null", "r");
            if (pp) {
                char buf[256]{};
                if (fgets(buf, sizeof(buf), pp)) {
                    std::string s(buf);
                    while (!s.empty() && (s.back()=='\n'||s.back()==' ')) s.pop_back();
                    if (!s.empty()) clangpp = s;
                }
                pclose(pp);
            }
            if (clangpp.empty()) clangpp = "clang++";
        } else {
            clangpp = "clang++"; // Windows uses winGpp (llvm-mingw), not clangpp
        }
        // Locate engine lib
        std::string engineLib, engineInc, glfwLib;
        if (!toolchainDir.empty()) {
            std::string eDir = toolchainDir + "/engine/linux64";
            if (targetName == "windows64" || targetName == "windows")
                eDir = toolchainDir + "/engine/windows64";
            if (fs::exists(eDir + "/libKonEngine.a")) {
                engineLib = eDir + "/libKonEngine.a";
                engineInc = eDir + "/include";
            }
            if (fs::exists(eDir + "/libglfw3.a"))
                glfwLib = eDir + "/libglfw3.a";
        }

        std::string incFlags;
        if (!engineInc.empty()) {
            incFlags = "-I\"" + engineInc + "\""
                     + " -I\"" + engineInc + "/glad/include\""
                     + " -I\"" + engineInc + "/stb\""
                     + " -I\"" + engineInc + "\"";  // GLFW/glfw3.h is at engineInc/GLFW/
            // GLM — bundled with engine
            std::string glmPath = toolchainDir + "/../libs/glm";
            if (!fs::exists(glmPath))
                glmPath = toolchainDir + "/../../libs/glm";
            if (!fs::exists(glmPath)) {
                // Try relative to engine include
                glmPath = engineInc + "/glm";
            }
            if (fs::exists(glmPath))
                incFlags += " -I\"" + glmPath + "\"";
        }
        // For Windows cross-compile, use llvm-mingw headers (bundled)
        // or fall back to system mingw headers
        if (isWin) {
            std::string llvmMingwDir = toolchainDir + "/llvm-mingw";
            std::string llvmMingwInc = llvmMingwDir + "/x86_64-w64-mingw32/include";
            if (fs::exists(llvmMingwInc)) {
                // llvm-mingw is self-contained — headers live inside it
                incFlags += " -I\"" + llvmMingwInc + "\"";
            } else {
                // Fall back to system mingw headers
                // Find clang resource dir for mm_malloc.h etc.
                std::string clangResDir;
                FILE* p = popen("clang++ --print-resource-dir 2>/dev/null", "r");
                if (p) {
                    char buf[512]; if (fgets(buf, sizeof(buf), p)) {
                        clangResDir = buf;
                        while (!clangResDir.empty() && (clangResDir.back()=='\n'||clangResDir.back()==' '))
                            clangResDir.pop_back();
                    }
                    pclose(p);
                }
                if (!clangResDir.empty() && fs::exists(clangResDir + "/include"))
                    incFlags += " -I\"" + clangResDir + "/include\"";
                // Fall back to system mingw headers
                std::string cxxDir;
                for (auto& base : std::vector<std::string>{
                    "/usr/lib/mingw64-toolchain/x86_64-w64-mingw32/include/c++",
                    "/usr/lib/gcc/x86_64-w64-mingw32",
                    "/usr/x86_64-w64-mingw32/usr/include/c++",
                    "/usr/x86_64-w64-mingw32/include/c++"}) {
                    if (!fs::exists(base)) continue;
                    try {
                        for (auto& e : fs::directory_iterator(base)) {
                            std::string p = e.path().string();
                            if (fs::exists(p + "/string") && p > cxxDir) cxxDir = p;
                        }
                    } catch (...) {}
                    if (cxxDir.empty() && fs::exists(base + "/string")) cxxDir = base;
                    if (!cxxDir.empty()) break;
                }
                if (!cxxDir.empty())
                    incFlags += " -I\"" + cxxDir + "\""
                              + " -I\"" + cxxDir + "/x86_64-w64-mingw32\""
                              + " -I\"" + cxxDir + "/backward\"";
                for (auto& d : std::vector<std::string>{
                    "/usr/lib/mingw64-toolchain/x86_64-w64-mingw32/include",
                    "/usr/x86_64-w64-mingw32/usr/include",
                    "/usr/x86_64-w64-mingw32/include"}) {
                    if (fs::exists(d + "/windows.h") || fs::exists(d + "/stdlib.h")) {
                        incFlags += " -I\"" + d + "\"";
                        break;
                    }
                }
            } // end else (no llvm-mingw)
        } // end if (isWin)

        std::string target_triple = "";
        if (!isWin)
            target_triple = "--target=x86_64-pc-linux-gnu ";

        // For Windows, use bundled llvm-mingw — self-contained, no system deps
        std::string winGpp;
        if (isWin) {
            std::string llvmMingwBin = toolchainDir + "/llvm-mingw/bin";
            std::string bundled = llvmMingwBin + "/x86_64-w64-mingw32-clang++";
            if (fs::exists(bundled)) {
                winGpp = bundled;
            } else {
                // Fall back to system mingw
                for (auto& c : std::vector<std::string>{
                    "/usr/bin/x86_64-w64-mingw32-g++",
                    "/usr/bin/x86_64-w64-mingw32-clang++",
                    "/usr/local/bin/x86_64-w64-mingw32-g++"}) {
                    if (fs::exists(c)) { winGpp = c; break; }
                }
            }
            if (winGpp.empty())
                std::cerr << "konscript: warning: no Windows cross-compiler found. "
                          << "Run bundle-toolchain.sh to set up llvm-mingw.\n";
        }

        std::cout << "[build/" << (targetName.empty() ? "linux64" : targetName)
                  << "] " << fs::path(path).filename().string()
                  << " -> " << outPath << "\n";

        // ── Compile flags ────────────────────────────────────────────────────
        // For Windows: use winGpp (llvm-mingw or system mingw-g++).
        //   Don't add clang-specific flags or custom headers — mingw knows its own sysroot.
        // For Linux: use system clang++ with standard suppression flags.
        std::string compiler = (isWin && !winGpp.empty()) ? winGpp : clangpp;
        bool usingGpp = isWin && !winGpp.empty() &&
            (winGpp.find("g++") != std::string::npos || winGpp.find("mingw32") != std::string::npos);

        // When using mingw g++, strip all custom C++ header paths — g++ finds its own.
        // Only keep engine/glad/glm/GLFW includes.
        std::string finalIncFlags = incFlags;
        if (usingGpp && !engineInc.empty()) {
            finalIncFlags = "-I\"" + engineInc + "\""
                          + " -I\"" + engineInc + "/glad/include\""
                          + " -I\"" + engineInc + "/stb\""
                          + " -I\"" + engineInc + "\""; // GLFW
            std::string glmPath = engineInc + "/glm";
            if (fs::exists(glmPath)) finalIncFlags += " -I\"" + glmPath + "\"";
        }

        std::string compileCmd = "\"" + compiler + "\" -std=c++17 -O2 ";

        if (!isWin) {
            // Linux: clang-specific suppression flags
            compileCmd += "--target=x86_64-pc-linux-gnu "
                          "-Wno-pragma-once-outside-header "
                          "-Wno-invalid-constexpr -Wno-deprecated-builtins "
                          "-Wno-inline-namespace-reopened-noninline "
                          "-Wno-keyword-compat -Wno-unknown-attributes "
                          "-Wno-user-defined-literals -Wno-ignored-attributes "
                          "-DGLM_FORCE_PURE ";
        } else if (!usingGpp) {
            // Windows with clang (llvm-mingw): clang-compatible flags
            compileCmd += "--target=x86_64-w64-mingw32 "
                          "-Wno-pragma-once-outside-header "
                          "-Wno-invalid-constexpr -Wno-deprecated-builtins "
                          "-Wno-inline-namespace-reopened-noninline "
                          "-Wno-keyword-compat -Wno-unknown-attributes "
                          "-Wno-user-defined-literals -Wno-ignored-attributes "
                          "-DGLM_FORCE_PURE ";
        } else {
            // Windows with g++ (system mingw): only g++-compatible flags
            compileCmd += "-DGLM_FORCE_PURE -w ";
        }

        compileCmd += finalIncFlags + " \"" + unityFile + "\"";

        // ── Link flags ───────────────────────────────────────────────────────
        if (isWin) {
            compileCmd += " -o \"" + outPath + "\""
                        + (!engineLib.empty() ? " \"" + engineLib + "\"" : "")
                        + (!glfwLib.empty()   ? " \"" + glfwLib   + "\"" : "")
                        + " -lopengl32 -lgdi32 -lwinmm -lws2_32"
                        + " -static-libstdc++ -static-libgcc";
        } else {
            std::string glfwLink = !glfwLib.empty() ? " \"" + glfwLib + "\"" : " -lglfw";
            compileCmd += " -o \"" + outPath + "\""
                        + (!engineLib.empty() ? " \"" + engineLib + "\"" : "")
                        + glfwLink
                        + " -lGL -lX11 -lXrandr -lXi -ldl -lpthread -lm";
        }

        std::cout << "[4/4] Compiling & linking...\n" << std::flush;
        int r = std::system(compileCmd.c_str());
        if (r != 0) {
            std::cerr << "konscript: compile failed\n"
                      << "  Run build-engine-lib.sh to set up the engine library.\n";
            return 1;
        }
        return 0;
    }

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

    printStageDoing(4, 5, "Compiling IR");
    double t3 = msNow();
    if (std::system(llcCmd.c_str()) != 0) {
        std::cerr << "\nkonscript: llc failed\n"
                  << "  Toolchain: " << (toolchainDir.empty() ? "(system PATH)" : toolchainDir) << "\n"
                  << "  Run bundle-toolchain.sh to set up the bundled toolchain.\n";
        return 1;
    }
    printStageOK(4, 5, "Compiling IR", msNow() - t3);

    // Compile _ks_runtime.c → _ks_runtime.o (stdlib implementation for File/Str/Array/HashMap)
    bool hasRuntime = false;
    std::string runtimeObj = buildDir + "/_ks_runtime.o";
    {
        std::string runtimeSrc = selfDir() + "/_ks_runtime.c";
        if (fs::exists(runtimeSrc)) {
            std::string cc = "cc";
            if (!toolchainDir.empty()) {
                std::string bundledClang = toolchainDir + "/llvm/bin/clang" + exe;
                if (fs::exists(bundledClang)) cc = bundledClang;
            }
            std::string rtCmd;
            bool isWinTarget = (targetName == "windows64" || targetName == "windows");
            if (isWinTarget) {
                // Cross-compile runtime for Windows using clang with target triple
                std::string winClang = cc;
                // Try llvm-mingw clang first
                std::string llvmMingwClang = toolchainDir + "/llvm-mingw/bin/x86_64-w64-mingw32-clang";
                if (!toolchainDir.empty() && fs::exists(llvmMingwClang))
                    winClang = llvmMingwClang;
                else {
                    // Try system mingw gcc
                    if (std::system("x86_64-w64-mingw32-gcc --version > /dev/null 2>&1") == 0)
                        winClang = "x86_64-w64-mingw32-gcc";
                    else {
                        // Need clang for --target flag (gcc doesn't support it)
                        std::string clangPath = "clang";
                        if (!toolchainDir.empty()) {
                            std::string bc = toolchainDir + "/llvm/bin/clang";
                            if (fs::exists(bc)) clangPath = bc;
                        }
                        winClang = clangPath;
                    }
                }
                std::string winFlags = "";
                // If not using a mingw-prefixed compiler, add --target flag
                if (winClang.find("mingw") == std::string::npos)
                    winFlags = " --target=x86_64-pc-windows-gnu";
                rtCmd = "\"" + winClang + "\"" + winFlags + " -std=c11 -O2 -D_POSIX_C_SOURCE=200809L -c \""
                      + runtimeSrc + "\" -o \"" + runtimeObj + "\"";
            } else {
                rtCmd = "\"" + cc + "\" -std=c11 -O2 -D_POSIX_C_SOURCE=200809L -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 -c \""
                      + runtimeSrc + "\" -o \"" + runtimeObj + "\"";
            }
            if (std::system(rtCmd.c_str()) == 0) {
                hasRuntime = true;
            } else {
                std::cerr << "konscript: warning: _ks_runtime.c compile failed"
                          << " — stdlib File/Str/HashMap unavailable\n";
            }
        }
    }
    bool isWindows = (targetName == "windows64" || targetName == "windows");
    bool isWasm    = (targetName == "wasm32"    || targetName == "wasm");

    std::string lld;
    if (!toolchainDir.empty()) {
        if (isWindows)     lld = toolchainDir + "/llvm/bin/ld.lld" + exe; // MinGW GNU ld style
        else if (isWasm)   lld = toolchainDir + "/llvm/bin/wasm-ld"  + exe;
        else               lld = toolchainDir + "/llvm/bin/ld.lld"   + exe;
    } else {
        // Use ld.lld for all platforms (GNU ld compatible for MinGW)
        lld = isWasm ? "wasm-ld" : "ld.lld";
    }

    std::string linkCmd;

    if (isWindows) {
        std::string sr = toolchainDir.empty() ? "" : toolchainDir + "/sysroot/windows64/lib";
        // Use GNU ld-style linking (MinGW ABI)
        linkCmd = "\"" + lld + "\" -m i386pep -static";
        if (!sr.empty() && fs::exists(sr + "/crt2.o")) {
            linkCmd += " \"" + sr + "/crt2.o\""
                     + " \"" + objFile + "\"";
            if (hasRuntime) linkCmd += " \"" + runtimeObj + "\"";
            linkCmd += " -L\"" + sr + "\""
                     + " -lmingw32 -lmingwex -lmsvcrt -lkernel32";
            if (fs::exists(sr + "/libucrt.a")) linkCmd += " -lucrt";
            if (fs::exists(sr + "/libgcc.a"))  linkCmd += " -lgcc";
        } else {
            linkCmd += " \"" + objFile + "\"";
            if (hasRuntime) linkCmd += " \"" + runtimeObj + "\"";
            if (sr.empty() || !fs::exists(sr))
                std::cerr << "konscript: warning: Windows sysroot not found at "
                          << (toolchainDir + "/sysroot/windows64/lib") << "\n"
                          << "  Run bundle-toolchain.sh to set up the Windows sysroot.\n";
        }
        linkCmd += " -o \"" + outPath + "\"";
    } else if (isWasm) {
        linkCmd = "\"" + lld + "\""
            + " --export-all --no-entry"
            + " \"" + objFile + "\"";
        if (hasRuntime) linkCmd += " \"" + runtimeObj + "\"";
        linkCmd += " -o \"" + outPath + "\"";
    } else if (!toolchainDir.empty()) {
        std::string sr = toolchainDir + "/sysroot/linux64/lib";
        linkCmd = "\"" + lld + "\""
            + " -static"
            + " \"" + sr + "/crt1.o\""
            + " \"" + sr + "/crti.o\""
            + " \"" + objFile + "\"";
        if (hasRuntime) linkCmd += " \"" + runtimeObj + "\"";
        linkCmd += " \"" + sr + "/libc.a\""
            + " \"" + sr + "/libm.a\""
            + " \"" + sr + "/crtn.o\""
            + " -o \"" + outPath + "\"";
    } else {
        // System fallback: find CRT objects and library paths on the host
        std::string crtDir;
        for (const char* d : {
            "/usr/lib/x86_64-linux-gnu",
            "/usr/lib64",
            "/usr/lib",
        }) {
            if (fs::exists(std::string(d) + "/crt1.o")) { crtDir = d; break; }
        }
        linkCmd = "\"" + lld + "\"";
        if (!crtDir.empty()) {
            linkCmd += " \"" + crtDir + "/crt1.o\""
                + " \"" + crtDir + "/crti.o\"";
        }
        linkCmd += " \"" + objFile + "\"";
        if (hasRuntime) linkCmd += " \"" + runtimeObj + "\"";
        if (!crtDir.empty()) {
            linkCmd += " \"" + crtDir + "/crtn.o\"";
        }
        // Add standard library search paths
        for (const char* libDir : {
            "/usr/lib/x86_64-linux-gnu",
            "/usr/lib64",
            "/usr/lib",
            "/lib/x86_64-linux-gnu",
            "/lib64",
        }) {
            if (fs::is_directory(libDir))
                linkCmd += std::string(" -L ") + libDir;
        }
        // Dynamic linker for non-static builds
        if (fs::exists("/lib64/ld-linux-x86-64.so.2"))
            linkCmd += " -dynamic-linker /lib64/ld-linux-x86-64.so.2";
        else if (fs::exists("/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2"))
            linkCmd += " -dynamic-linker /lib/x86_64-linux-gnu/ld-linux-x86-64.so.2";
        linkCmd += std::string(" -lm -lc")
            + " -o \"" + outPath + "\"";
    }

    printStageDoing(5, 5, "Linking");
    double t4 = msNow();
    if (std::system(linkCmd.c_str()) != 0) {
        std::cerr << "\nkonscript: link failed\n";
#ifdef _WIN32
        std::cerr << "  Run: powershell -ExecutionPolicy Bypass -File bundle-toolchain.ps1\n";
#else
        std::cerr << "  Run bundle-toolchain.sh to set up the bundled toolchain.\n";
#endif
        return 1;
    }
    printStageOK(5, 5, "Linking", msNow() - t4);

    // Final success line
    std::string tgt = targetName.empty() ? "linux64" : targetName;
    std::cout << "\n" << KS_BOLD << KS_GREEN << "  ✓ " << KS_RESET
              << KS_BOLD << outPath << KS_RESET
              << KS_DIM << "  [" << tgt << "]" << KS_RESET << "\n\n";
    return 0;
}
