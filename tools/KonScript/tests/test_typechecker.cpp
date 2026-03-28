#include "../include/lexer.hpp"
#include "../include/parser.hpp"
#include "../include/typechecker.hpp"
#include <iostream>

int main() {
    std::string src = R"(
#include <engine>

const GRAVITY: F64 = 980.0;

enum State {
    Idle,
    Walking,
    Jumping(F64),
}

struct Point {
    let x: F64;
    let y: F64;
}

node Player : Node2D {
    let speed: F64 = 200.0;
    let mut health: I32 = 100;
    let mut state: State = Idle;

    func Ready() {
        Print("Player ready!");
    }

    func Update(dt: F64) {
        if KeyDown(32) { x += speed * dt; }

        for i: I32 = 0; i < 10; i++ {
            Print("%d");
        }

        for item: I32 in 0..10 {
            Print("%d");
        }
    }

    func OnCollisionEnter(other: Node2D) {
        if other.name == "Enemy" {
            health -= 10;
            Emit("player_hurt");
        }
    }

    func GetHealth() -> I32 {
        return health;
    }

    func GetPos() -> (F64, F64) {
        return (x, y);
    }
}

// This should trigger errors:
node Broken : Node2D {
    let immutable: I32 = 5;

    func Update(dt: F64) {
        immutable = 10;       // ERROR: assign to immutable
    }
}
)";

    KonScript::Lexer lexer(src, "test.ks");
    auto tokens = lexer.tokenize();
    if (lexer.hasErrors()) {
        for (auto& e : lexer.errors())
            std::cerr << "LEX: " << e.message << "\n";
        return 1;
    }

    KonScript::Parser parser(std::move(tokens), "test.ks");
    auto prog = parser.parse();
    if (parser.hasErrors()) {
        for (auto& e : parser.errors())
            std::cerr << "PARSE: " << e << "\n";
        return 1;
    }

    KonScript::TypeChecker checker;
    checker.check(prog);

    if (checker.hasErrors()) {
        std::cout << "Type errors found (" << checker.errors().size() << "):\n";
        for (auto& e : checker.errors())
            std::cout << "  [" << e.line << ":" << e.col << "] " << e.message << "\n";
    } else {
        std::cout << "No type errors found.\n";
    }

    std::cout << "\nTypeChecker OK\n";
    return 0;
}
