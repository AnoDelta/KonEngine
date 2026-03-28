#include "../include/lexer.hpp"
#include "../include/parser.hpp"
#include "../include/typechecker.hpp"
#include "../include/codegen.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

static void usage() {
    std::cout << "KonScript v0.1.0\n\n"
              << "Usage:\n"
              << "  konscript <file.ks>            -- compile (auto-detects engine/standalone)\n"
              << "  konscript <file.ks> -o out.cpp -- compile with custom output path\n"
              << "  konscript --lex   <file.ks>    -- dump tokens (debug)\n"
              << "  konscript --parse <file.ks>    -- dump AST (debug)\n"
              << "  konscript --check <file.ks>    -- typecheck only (debug)\n"
              << "  konscript --help\n";
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
                      << (f->returnType ? " -> " + f->returnType->base : "") << "\n";
            break;
        }
        case KonScript::Stmt::Kind::Const: {
            auto* c = static_cast<const KonScript::ConstStmt*>(s);
            std::cout << pad << "const " << c->name << ": " << c->type.base << "\n";
            break;
        }
        case KonScript::Stmt::Kind::EnumDecl: {
            auto* e = static_cast<const KonScript::EnumDecl*>(s);
            std::cout << pad << "enum " << e->name
                      << " (" << e->variants.size() << " variants)\n";
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
                      << (i->isSystem ? "<" + i->path + ">" : "\"" + i->path + "\"")
                      << "\n";
            break;
        }
        default:
            std::cout << pad << "stmt(kind=" << (int)s->kind << ")\n";
            break;
    }
}

int main(int argc, char** argv) {
    if (argc < 2) { usage(); return 0; }

    std::string first = argv[1];
    if (first == "--help") { usage(); return 0; }

    // Debug modes
    bool lexOnly   = false;
    bool parseOnly = false;
    bool checkOnly = false;
    std::string path;
    std::string outPath;

    if (first == "--lex"   && argc >= 3) { lexOnly   = true; path = argv[2]; }
    else if (first == "--parse" && argc >= 3) { parseOnly = true; path = argv[2]; }
    else if (first == "--check" && argc >= 3) { checkOnly = true; path = argv[2]; }
    else {
        // Default: compile
        path = first;
        if (argc >= 4 && std::string(argv[2]) == "-o")
            outPath = argv[3];
    }

    std::string src = readFile(path);

    // ---- Lex ----
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

    // Detect target before parsing
    bool engineTarget = hasEngineInclude(tokens);

    // ---- Parse ----
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

    // ---- Typecheck ----
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

    // ---- Compile to C++ ----
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

    // Default output path: replace .ks with .cpp
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
