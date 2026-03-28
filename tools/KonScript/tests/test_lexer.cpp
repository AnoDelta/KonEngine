#include "../include/lexer.hpp"
#include <iostream>

int main() {
    std::string src = R"(
#include <engine>

const GRAVITY: F64 = 980.0;

node Player : Node2D {
    let speed: F64 = 200.0;
    let mut health: I32 = 100;

    func Update(dt: F64) {
        if KeyDown(Key.D) { x += speed * dt; }
        if KeyDown(Key.Space) && grounded {
            y -= 400.0;
        }
    }

    func OnCollisionEnter(other: Node2D) {
        if other.name == "Enemy" {
            health -= 10;
            Emit("player_hurt", health);
        }
    }

    func GetHealth() -> I32 {
        return health;
    }
}
)";

    KonScript::Lexer lexer(src, "test.ks");
    auto tokens = lexer.tokenize();

    if (lexer.hasErrors()) {
        for (auto& e : lexer.errors())
            std::cerr << "ERROR: " << e.message << "\n";
        return 1;
    }

    std::cout << "Tokens (" << tokens.size() << "):\n";
    for (auto& t : tokens) {
        if (t.type == KonScript::TokenType::Eof) break;
        std::cout << "  " << t.line << ":" << t.col
                  << "\t" << t.value << "\n";
    }

    std::cout << "\nLexer OK -- " << tokens.size() << " tokens\n";
    return 0;
}
