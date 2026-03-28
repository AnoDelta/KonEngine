#include "../include/lexer.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

static void usage() {
    std::cout << "KonScript v0.1.0\n\n"
              << "Usage:\n"
              << "  konscript <file.ks>        -- run (interpreter mode)\n"
              << "  konscript --lex <file.ks>  -- dump tokens\n"
              << "  konscript --help\n";
}

int main(int argc, char** argv) {
    if (argc < 2) { usage(); return 0; }

    std::string arg = argv[1];

    if (arg == "--help") { usage(); return 0; }

    bool lexOnly = false;
    std::string path;

    if (arg == "--lex" && argc >= 3) {
        lexOnly = true;
        path = argv[2];
    } else {
        path = arg;
    }

    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "konscript: cannot open '" << path << "'\n";
        return 1;
    }

    std::ostringstream ss;
    ss << f.rdbuf();
    std::string src = ss.str();

    KonScript::Lexer lexer(src, path);
    auto tokens = lexer.tokenize();

    if (lexer.hasErrors()) {
        for (auto& e : lexer.errors())
            std::cerr << e.message << "\n";
        return 1;
    }

    if (lexOnly) {
        for (auto& t : tokens) {
            if (t.type == KonScript::TokenType::Eof) break;
            std::cout << t.line << ":" << t.col
                      << "\t" << t.value << "\n";
        }
        return 0;
    }

    std::cout << "Parser not implemented yet.\n";
    return 0;
}
