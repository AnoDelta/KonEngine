#include "../include/lexer.hpp"
#include "../include/parser.hpp"
#include <iostream>

// Simple AST printer for verification
void printExpr(const KonScript::Expr* e, int indent = 0) {
    std::string pad(indent * 2, ' ');
    if (!e) { std::cout << pad << "(null)\n"; return; }
    switch (e->kind) {
        case KonScript::Expr::Kind::IntLit:
            std::cout << pad << "Int(" << static_cast<const KonScript::IntLitExpr*>(e)->value << ")\n"; break;
        case KonScript::Expr::Kind::FloatLit:
            std::cout << pad << "Float(" << static_cast<const KonScript::FloatLitExpr*>(e)->value << ")\n"; break;
        case KonScript::Expr::Kind::BoolLit:
            std::cout << pad << "Bool(" << (static_cast<const KonScript::BoolLitExpr*>(e)->value ? "true" : "false") << ")\n"; break;
        case KonScript::Expr::Kind::StrLit:
            std::cout << pad << "Str(\"" << static_cast<const KonScript::StrLitExpr*>(e)->value << "\")\n"; break;
        case KonScript::Expr::Kind::Ident:
            std::cout << pad << "Ident(" << static_cast<const KonScript::IdentExpr*>(e)->name << ")\n"; break;
        case KonScript::Expr::Kind::Binary: {
            auto* b = static_cast<const KonScript::BinaryExpr*>(e);
            std::cout << pad << "Binary(" << b->op << ")\n";
            printExpr(b->left.get(), indent+1);
            printExpr(b->right.get(), indent+1);
            break;
        }
        case KonScript::Expr::Kind::Call: {
            auto* c = static_cast<const KonScript::CallExpr*>(e);
            std::cout << pad << "Call\n";
            printExpr(c->callee.get(), indent+1);
            for (auto& a : c->args) printExpr(a.get(), indent+2);
            break;
        }
        case KonScript::Expr::Kind::Member: {
            auto* m = static_cast<const KonScript::MemberExpr*>(e);
            std::cout << pad << "Member(." << m->member << ")\n";
            printExpr(m->object.get(), indent+1);
            break;
        }
        default:
            std::cout << pad << "Expr(kind=" << (int)e->kind << ")\n"; break;
    }
}

void printStmt(const KonScript::Stmt* s, int indent = 0) {
    std::string pad(indent * 2, ' ');
    if (!s) return;
    switch (s->kind) {
        case KonScript::Stmt::Kind::NodeDecl: {
            auto* n = static_cast<const KonScript::NodeDecl*>(s);
            std::cout << pad << "NodeDecl(" << n->name << " : " << n->base << ")\n";
            std::cout << pad << "  fields: " << n->fields.size() << "\n";
            std::cout << pad << "  methods: " << n->methods.size() << "\n";
            for (auto& m : n->methods)
                std::cout << pad << "    func " << m->name
                          << "(" << m->params.size() << " params)\n";
            break;
        }
        case KonScript::Stmt::Kind::FuncDecl: {
            auto* f = static_cast<const KonScript::FuncDecl*>(s);
            std::cout << pad << "FuncDecl(" << f->name << ")\n";
            break;
        }
        case KonScript::Stmt::Kind::Let: {
            auto* l = static_cast<const KonScript::LetStmt*>(s);
            std::cout << pad << "Let(" << (l->mut?"mut ":"") << l->name
                      << ": " << l->type.base << ")\n";
            break;
        }
        case KonScript::Stmt::Kind::Const: {
            auto* c = static_cast<const KonScript::ConstStmt*>(s);
            std::cout << pad << "Const(" << c->name << ": " << c->type.base << ")\n";
            break;
        }
        case KonScript::Stmt::Kind::If:
            std::cout << pad << "If\n"; break;
        case KonScript::Stmt::Kind::While:
            std::cout << pad << "While\n"; break;
        case KonScript::Stmt::Kind::ForIn:
            std::cout << pad << "ForIn(" << static_cast<const KonScript::ForInStmt*>(s)->var << ")\n"; break;
        case KonScript::Stmt::Kind::ForC:
            std::cout << pad << "ForC(" << static_cast<const KonScript::ForCStmt*>(s)->var << ")\n"; break;
        case KonScript::Stmt::Kind::Switch:
            std::cout << pad << "Switch(" << static_cast<const KonScript::SwitchStmt*>(s)->cases.size() << " cases)\n"; break;
        case KonScript::Stmt::Kind::Return:
            std::cout << pad << "Return\n"; break;
        case KonScript::Stmt::Kind::EnumDecl: {
            auto* e = static_cast<const KonScript::EnumDecl*>(s);
            std::cout << pad << "EnumDecl(" << e->name << ", " << e->variants.size() << " variants)\n";
            break;
        }
        case KonScript::Stmt::Kind::StructDecl: {
            auto* st = static_cast<const KonScript::StructDecl*>(s);
            std::cout << pad << "StructDecl(" << st->name << ", " << st->fields.size() << " fields)\n";
            break;
        }
        case KonScript::Stmt::Kind::Include: {
            auto* i = static_cast<const KonScript::IncludeStmt*>(s);
            std::cout << pad << "Include(" << i->path << ")\n";
            break;
        }
        default:
            std::cout << pad << "Stmt(kind=" << (int)s->kind << ")\n"; break;
    }
}

int main() {
    std::string src = R"(
#include <engine>
#include "Player.ks"

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
        if KeyDown(Key.D) { x += speed * dt; }
        if KeyDown(Key.A) { x -= speed * dt; }

        for i: I32 = 0; i < 10; i++ {
            Print("%d", i);
        }

        for item: I32 in 0..10 {
            Print("%d", item);
        }

        switch state {
            case Idle:
                Print("idle");
                break;
            case Walking:
                x += speed * dt;
                break;
            default:
                break;
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

    func GetPos() -> (F64, F64) {
        return (x, y);
    }
}
)";

    KonScript::Lexer lexer(src, "test.ks");
    auto tokens = lexer.tokenize();
    if (lexer.hasErrors()) {
        for (auto& e : lexer.errors()) std::cerr << "LEX: " << e.message << "\n";
        return 1;
    }

    KonScript::Parser parser(std::move(tokens), "test.ks");
    auto prog = parser.parse();

    if (parser.hasErrors()) {
        for (auto& e : parser.errors()) std::cerr << "PARSE: " << e << "\n";
        return 1;
    }

    std::cout << "AST (" << prog.stmts.size() << " top-level nodes):\n\n";
    for (auto& s : prog.stmts)
        printStmt(s.get(), 0);

    std::cout << "\nParser OK\n";
    return 0;
}
