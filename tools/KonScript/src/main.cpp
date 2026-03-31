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

// -----------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------
static void usage() {
    std::cout <<
        "KonScript v0.2.0\n\n"
        "USAGE\n"
        "  konscript <file.ks>                    compile → .cpp  (auto-detects engine/standalone)\n"
        "  konscript <file.ks> -o out.cpp         compile → custom path\n"
        "  konscript --llvm  <file.ks>            compile → .ll   (LLVM IR)\n"
        "  konscript --llvm  <file.ks> -o out.ll  compile → custom .ll path\n"
        "  konscript --target <triple> <file.ks>  set target triple for --llvm\n"
        "                                          built-ins: linux64, windows64, wasm32\n"
        "                                          or any LLVM triple e.g. aarch64-linux-gnu\n\n"
        "DEBUG\n"
        "  konscript --lex   <file.ks>            dump tokens\n"
        "  konscript --parse <file.ks>            dump AST\n"
        "  konscript --check <file.ks>            typecheck only\n"
        "  konscript --ir    <file.ks>            emit IR and print to stdout (no file)\n\n"
        "EXAMPLES\n"
        "  konscript game.ks                      → game.cpp\n"
        "  konscript --llvm game.ks               → game.ll  (linux64 target)\n"
        "  konscript --llvm --target windows64 game.ks  → game.ll for Windows\n"
        "  konscript --target aarch64-linux-gnu --llvm game.ks\n";
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

// Resolve a user-supplied target name to an IRGen::Target
static KonScript::IRGen::Target resolveTarget(const std::string& name) {
    if (name == "linux64"   || name == "linux")   return KonScript::IRGen::Target::linux64();
    if (name == "windows64" || name == "windows") return KonScript::IRGen::Target::windows64();
    if (name == "wasm32"    || name == "wasm")    return KonScript::IRGen::Target::wasm32();
    // Treat as a raw LLVM triple — use linux64 datalayout as a reasonable default
    KonScript::IRGen::Target t = KonScript::IRGen::Target::linux64();
    t.triple = name;
    return t;
}

// -----------------------------------------------------------------------
// AST printer (unchanged from v0.1.0)
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
        std::cout << pad << "struct " << st->name
                  << " (" << st->fields.size() << " fields)\n";
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
// Shared pipeline: lex → parse → typecheck
// Returns false and prints errors if any stage fails.
// -----------------------------------------------------------------------
static bool runPipeline(
    const std::string& src,
    const std::string& path,
    bool engineTarget,
    KonScript::Program& progOut)
{
    // Lex
    KonScript::Lexer lexer(src, path);
    auto tokens = lexer.tokenize();
    if (lexer.hasErrors()) {
        for (auto& e : lexer.errors())
            std::cerr << e.message << "\n";
        return false;
    }

    // Parse
    KonScript::Parser parser(std::move(tokens), path);
    progOut = parser.parse();
    if (parser.hasErrors()) {
        for (auto& e : parser.errors())
            std::cerr << e << "\n";
        return false;
    }

    // Typecheck
    KonScript::TypeChecker checker;
    checker.check(progOut);
    if (checker.hasErrors()) {
        for (auto& e : checker.errors())
            std::cerr << path << ":" << e.line << ":" << e.col
                      << ": error: " << e.message << "\n";
        return false;
    }

    return true;
}

// -----------------------------------------------------------------------
// main
// -----------------------------------------------------------------------
int main(int argc, char** argv) {
    if (argc < 2) { usage(); return 0; }

    std::string first = argv[1];
    if (first == "--help" || first == "-h") { usage(); return 0; }

    // ── Parse flags ──────────────────────────────────────────────────────
    bool lexOnly   = false;
    bool parseOnly = false;
    bool checkOnly = false;
    bool llvmMode  = false;
    bool irDump    = false;  // --ir: print IR to stdout, don't write file

    std::string path;
    std::string outPath;
    std::string targetName = "linux64";  // default LLVM target

    // Walk all args
    std::vector<std::string> args(argv + 1, argv + argc);
    for (size_t i = 0; i < args.size(); i++) {
        if      (args[i] == "--lex")   { lexOnly   = true; }
        else if (args[i] == "--parse") { parseOnly = true; }
        else if (args[i] == "--check") { checkOnly = true; }
        else if (args[i] == "--llvm")  { llvmMode  = true; }
        else if (args[i] == "--ir")    { irDump    = true; llvmMode = true; }
        else if (args[i] == "--target" && i + 1 < args.size()) {
            targetName = args[++i];
        }
        else if (args[i] == "-o" && i + 1 < args.size()) {
            outPath = args[++i];
        }
        else if (args[i][0] != '-') {
            if (path.empty()) path = args[i];
        }
    }

    if (path.empty()) {
        std::cerr << "konscript: no input file\n";
        usage();
        return 1;
    }

    std::string src = readFile(path);

    // ── Lex-only mode ─────────────────────────────────────────────────────
    KonScript::Lexer lexerPeek(src, path);
    auto tokens = lexerPeek.tokenize();
    if (lexerPeek.hasErrors()) {
        for (auto& e : lexerPeek.errors()) std::cerr << e.message << "\n";
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

    // ── Parse-only mode ───────────────────────────────────────────────────
    if (parseOnly) {
        KonScript::Parser parser(std::move(tokens), path);
        auto prog = parser.parse();
        if (parser.hasErrors()) {
            for (auto& e : parser.errors()) std::cerr << e << "\n";
            return 1;
        }
        std::cout << prog.stmts.size() << " top-level declarations:\n";
        for (auto& s : prog.stmts) printStmt(s.get(), 1);
        return 0;
    }

    // ── Full pipeline ─────────────────────────────────────────────────────
    // Re-lex so we can move tokens into the parser
    KonScript::Lexer lexer2(src, path);
    auto tokens2 = lexer2.tokenize();

    KonScript::Parser parser(std::move(tokens2), path);
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

    KonScript::TypeChecker checker;
    checker.check(prog);
    if (checker.hasErrors()) {
        for (auto& e : checker.errors())
            std::cerr << path << ":" << e.line << ":" << e.col
                      << ": error: " << e.message << "\n";
        return 1;
    }
    if (checkOnly) {
        std::cout << path << ": OK\n";
        return 0;
    }

    // ── LLVM IR mode ──────────────────────────────────────────────────────
    if (llvmMode) {
        KonScript::IRGen irgen;
        irgen.setTarget(resolveTarget(targetName));
        std::string ir = irgen.generate(prog);

        if (irgen.hasErrors()) {
            for (auto& e : irgen.errors())
                std::cerr << path << ":" << e.line << ":" << e.col
                          << ": irgen error: " << e.message << "\n";
            return 1;
        }

        // --ir: print to stdout only
        if (irDump) {
            std::cout << ir;
            return 0;
        }

        // Default output: replace .ks with .ll
        if (outPath.empty()) {
            outPath = path;
            auto dot = outPath.rfind('.');
            if (dot != std::string::npos) outPath = outPath.substr(0, dot);
            outPath += ".ll";
        }

        std::ofstream out(outPath);
        if (!out.is_open()) {
            std::cerr << "konscript: cannot write '" << outPath << "'\n";
            return 1;
        }
        out << ir;

        std::cout << "[llvm/" << targetName << "] " << path << " -> " << outPath << "\n";
        return 0;
    }

    // ── C++ transpiler mode (default) ─────────────────────────────────────
    KonScript::Codegen cg;
    cg.setTarget(engineTarget
        ? KonScript::Codegen::Target::Engine
        : KonScript::Codegen::Target::Standalone);

    std::string cpp = cg.generate(prog);
    if (cg.hasErrors()) {
        for (auto& e : cg.errors())
            std::cerr << "codegen: " << e.message << "\n";
        return 1;
    }

    if (outPath.empty()) {
        outPath = path;
        auto dot = outPath.rfind('.');
        if (dot != std::string::npos) outPath = outPath.substr(0, dot);
        outPath += ".cpp";
    }

    std::ofstream out(outPath);
    if (!out.is_open()) {
        std::cerr << "konscript: cannot write '" << outPath << "'\n";
        return 1;
    }
    out << cpp;

    std::cout << (engineTarget ? "[engine] " : "[standalone] ")
              << path << " -> " << outPath << "\n";
    return 0;
}
