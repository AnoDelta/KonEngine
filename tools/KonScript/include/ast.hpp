#pragma once
// -----------------------------------------------------------------------
// KonScript AST
// Every construct in the language is represented as a node in this tree.
// -----------------------------------------------------------------------
#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace KonScript {

// -----------------------------------------------------------------------
// Forward declarations
// -----------------------------------------------------------------------
struct Expr;
struct Stmt;
using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;

// -----------------------------------------------------------------------
// Type annotation
// e.g. I32, F64, Node2D, [I32], (F64, F64), I32?
// -----------------------------------------------------------------------
struct TypeAnnotation {
    std::string base;           // "I32", "F64", "Node2D", etc.
    bool        nullable = false; // T?
    bool        isArray  = false; // [T]
    int         arraySize = -1; // [T; N] -- -1 means dynamic
    std::vector<TypeAnnotation> tupleTypes; // (T, T, ...)
    bool        isTuple  = false;
};

// -----------------------------------------------------------------------
// Expressions
// -----------------------------------------------------------------------
struct Expr {
    int line = 0, col = 0;
    virtual ~Expr() = default;

    enum class Kind {
        // Literals
        IntLit, FloatLit, BoolLit, StrLit, NullLit, NoneLit,

        // Identifiers
        Ident,

        // Unary
        Unary,      // -x  !x  x++  x--  x!

        // Binary
        Binary,     // x + y  x == y  x && y etc.

        // Assignment
        Assign,     // x = y  x += y etc.

        // Call
        Call,       // foo(a, b)

        // Member access
        Member,     // x.y
        SafeMember, // x?.y

        // Index
        Index,      // x[i]

        // Range
        Range,      // 0..10  0..=10

        // Cast
        Cast,       // x as F64

        // Null ops
        NullCoal,   // x ?? y
        ForceUnwrap,// x!

        // Some wrapping
        Some_,      // Some(x)

        // Array literal
        ArrayLit,   // [1, 2, 3]

        // Tuple literal
        TupleLit,   // (a, b)

        // Struct init
        StructInit, // Point { x: 1.0, y: 2.0 }

        // Spawn
        Spawn,      // spawn foo()
    } kind;
};

// Literal expressions
struct IntLitExpr : Expr {
    int64_t value;
    IntLitExpr(int64_t v, int l, int c) : value(v) { kind = Kind::IntLit; line=l; col=c; }
};

struct FloatLitExpr : Expr {
    double      value;
    std::string rawValue; // BUG-07: preserve source text for lossless emit
    FloatLitExpr(double v, const std::string& raw, int l, int c)
        : value(v), rawValue(raw) { kind = Kind::FloatLit; line=l; col=c; }
};

struct BoolLitExpr : Expr {
    bool value;
    BoolLitExpr(bool v, int l, int c) : value(v) { kind = Kind::BoolLit; line=l; col=c; }
};

struct StrLitExpr : Expr {
    std::string value;
    StrLitExpr(const std::string& v, int l, int c) : value(v) { kind = Kind::StrLit; line=l; col=c; }
};

struct NullLitExpr : Expr {
    NullLitExpr(int l, int c) { kind = Kind::NullLit; line=l; col=c; }
};

struct NoneLitExpr : Expr {
    NoneLitExpr(int l, int c) { kind = Kind::NoneLit; line=l; col=c; }
};

struct IdentExpr : Expr {
    std::string name;
    IdentExpr(const std::string& n, int l, int c) : name(n) { kind = Kind::Ident; line=l; col=c; }
};

struct UnaryExpr : Expr {
    std::string op;   // "-", "!", "++", "--", "!" (postfix force unwrap)
    ExprPtr     operand;
    bool        postfix = false;
    UnaryExpr(const std::string& o, ExprPtr e, bool post, int l, int c)
        : op(o), operand(std::move(e)), postfix(post) {
        kind = Kind::Unary; line=l; col=c;
    }
};

struct BinaryExpr : Expr {
    std::string op;
    ExprPtr     left, right;
    BinaryExpr(const std::string& o, ExprPtr l, ExprPtr r, int ln, int c)
        : op(o), left(std::move(l)), right(std::move(r)) {
        kind = Kind::Binary; line=ln; col=c;
    }
};

struct AssignExpr : Expr {
    std::string op;   // "=", "+=", "-=", "*=", "/="
    ExprPtr     target, value;
    AssignExpr(const std::string& o, ExprPtr t, ExprPtr v, int l, int c)
        : op(o), target(std::move(t)), value(std::move(v)) {
        kind = Kind::Assign; line=l; col=c;
    }
};

struct CallExpr : Expr {
    ExprPtr                  callee;
    std::vector<ExprPtr>     args;
    CallExpr(ExprPtr c, std::vector<ExprPtr> a, int l, int col_)
        : callee(std::move(c)), args(std::move(a)) {
        kind = Kind::Call; line=l; col=col_;
    }
};

struct MemberExpr : Expr {
    ExprPtr     object;
    std::string member;
    bool        safe = false; // ?.
    MemberExpr(ExprPtr o, const std::string& m, bool s, int l, int c)
        : object(std::move(o)), member(m), safe(s) {
        kind = safe ? Kind::SafeMember : Kind::Member; line=l; col=c;
    }
};

struct IndexExpr : Expr {
    ExprPtr object, index;
    IndexExpr(ExprPtr o, ExprPtr i, int l, int c)
        : object(std::move(o)), index(std::move(i)) {
        kind = Kind::Index; line=l; col=c;
    }
};

struct RangeExpr : Expr {
    ExprPtr from, to;
    bool    inclusive = false; // ..=
    RangeExpr(ExprPtr f, ExprPtr t, bool inc, int l, int c)
        : from(std::move(f)), to(std::move(t)), inclusive(inc) {
        kind = Kind::Range; line=l; col=c;
    }
};

struct CastExpr : Expr {
    ExprPtr        value;
    TypeAnnotation target;
    CastExpr(ExprPtr v, TypeAnnotation t, int l, int c)
        : value(std::move(v)), target(t) {
        kind = Kind::Cast; line=l; col=c;
    }
};

struct NullCoalExpr : Expr {
    ExprPtr left, right;
    NullCoalExpr(ExprPtr l, ExprPtr r, int ln, int c)
        : left(std::move(l)), right(std::move(r)) {
        kind = Kind::NullCoal; line=ln; col=c;
    }
};

struct ForceUnwrapExpr : Expr {
    ExprPtr value;
    ForceUnwrapExpr(ExprPtr v, int l, int c) : value(std::move(v)) {
        kind = Kind::ForceUnwrap; line=l; col=c;
    }
};

struct SomeExpr : Expr {
    ExprPtr value;
    SomeExpr(ExprPtr v, int l, int c) : value(std::move(v)) {
        kind = Kind::Some_; line=l; col=c;
    }
};

struct ArrayLitExpr : Expr {
    std::vector<ExprPtr> elements;
    ArrayLitExpr(std::vector<ExprPtr> e, int l, int c)
        : elements(std::move(e)) {
        kind = Kind::ArrayLit; line=l; col=c;
    }
};

struct TupleLitExpr : Expr {
    std::vector<ExprPtr> elements;
    TupleLitExpr(std::vector<ExprPtr> e, int l, int c)
        : elements(std::move(e)) {
        kind = Kind::TupleLit; line=l; col=c;
    }
};

struct StructInitField {
    std::string name;
    ExprPtr     value;
};

struct StructInitExpr : Expr {
    std::string                  typeName;
    std::vector<StructInitField> fields;
    StructInitExpr(const std::string& t, std::vector<StructInitField> f, int l, int c)
        : typeName(t), fields(std::move(f)) {
        kind = Kind::StructInit; line=l; col=c;
    }
};

struct SpawnExpr : Expr {
    ExprPtr call;
    SpawnExpr(ExprPtr c, int l, int col_) : call(std::move(c)) {
        kind = Kind::Spawn; line=l; col=col_;
    }
};

// -----------------------------------------------------------------------
// Statements
// -----------------------------------------------------------------------
struct Stmt {
    int line = 0, col = 0;
    virtual ~Stmt() = default;

    enum class Kind {
        Block,
        ExprStmt,
        Let,
        Const,
        Return,
        Break,
        Continue,
        If,
        While,
        Loop,
        ForC,       // for i: I32 = 0; i < 10; i++
        ForIn,      // for i: I32 in 0..10
        Switch,
        Wait,
        Include,

        // Top-level declarations
        FuncDecl,
        NodeDecl,
        StructDecl,
        EnumDecl,
        ClassDecl,
    } kind;
};

struct BlockStmt : Stmt {
    std::vector<StmtPtr> stmts;
    BlockStmt(std::vector<StmtPtr> s, int l, int c)
        : stmts(std::move(s)) { kind = Kind::Block; line=l; col=c; }
};

struct ExprStmt : Stmt {
    ExprPtr expr;
    ExprStmt(ExprPtr e, int l, int c) : expr(std::move(e)) {
        kind = Kind::ExprStmt; line=l; col=c;
    }
};

struct LetStmt : Stmt {
    std::string    name;
    TypeAnnotation type;
    ExprPtr        init;
    bool           mut = false;
    LetStmt(const std::string& n, TypeAnnotation t, ExprPtr i, bool m, int l, int c)
        : name(n), type(t), init(std::move(i)), mut(m) {
        kind = Kind::Let; line=l; col=c;
    }
};

struct ConstStmt : Stmt {
    std::string    name;
    TypeAnnotation type;
    ExprPtr        init;
    ConstStmt(const std::string& n, TypeAnnotation t, ExprPtr i, int l, int c)
        : name(n), type(t), init(std::move(i)) {
        kind = Kind::Const; line=l; col=c;
    }
};

struct ReturnStmt : Stmt {
    ExprPtr value; // optional
    ReturnStmt(ExprPtr v, int l, int c) : value(std::move(v)) {
        kind = Kind::Return; line=l; col=c;
    }
};

struct BreakStmt : Stmt {
    std::string label; // optional 'label
    BreakStmt(const std::string& lbl, int l, int c) : label(lbl) {
        kind = Kind::Break; line=l; col=c;
    }
};

struct ContinueStmt : Stmt {
    std::string label;
    ContinueStmt(const std::string& lbl, int l, int c) : label(lbl) {
        kind = Kind::Continue; line=l; col=c;
    }
};

struct IfStmt : Stmt {
    ExprPtr              cond;
    std::unique_ptr<BlockStmt> then_;
    std::unique_ptr<BlockStmt> else_; // optional
    IfStmt(ExprPtr c,
           std::unique_ptr<BlockStmt> t,
           std::unique_ptr<BlockStmt> e,
           int l, int col_)
        : cond(std::move(c)), then_(std::move(t)), else_(std::move(e)) {
        kind = Kind::If; line=l; col=col_;
    }
};

struct WhileStmt : Stmt {
    ExprPtr                    cond;
    std::unique_ptr<BlockStmt> body;
    WhileStmt(ExprPtr c, std::unique_ptr<BlockStmt> b, int l, int col_)
        : cond(std::move(c)), body(std::move(b)) {
        kind = Kind::While; line=l; col=col_;
    }
};

struct LoopStmt : Stmt {
    std::unique_ptr<BlockStmt> body;
    LoopStmt(std::unique_ptr<BlockStmt> b, int l, int c)
        : body(std::move(b)) { kind = Kind::Loop; line=l; col=c; }
};

struct ForCStmt : Stmt {
    // for i: I32 = 0; i < 10; i++
    std::string    var;
    TypeAnnotation type;
    ExprPtr        init;
    ExprPtr        cond;
    ExprPtr        step;
    std::unique_ptr<BlockStmt> body;
    ForCStmt(const std::string& v, TypeAnnotation t,
             ExprPtr i, ExprPtr c, ExprPtr s,
             std::unique_ptr<BlockStmt> b, int l, int col_)
        : var(v), type(t), init(std::move(i)),
          cond(std::move(c)), step(std::move(s)), body(std::move(b)) {
        kind = Kind::ForC; line=l; col=col_;
    }
};

struct ForInStmt : Stmt {
    // for i: I32 in expr
    std::string    var;
    TypeAnnotation type;
    ExprPtr        iterable;
    std::unique_ptr<BlockStmt> body;
    std::string    label;
    ForInStmt(const std::string& v, TypeAnnotation t,
              ExprPtr it, std::unique_ptr<BlockStmt> b,
              const std::string& lbl, int l, int col_)
        : var(v), type(t), iterable(std::move(it)), body(std::move(b)), label(lbl) {
        kind = Kind::ForIn; line=l; col=col_;
    }
};

struct SwitchCase {
    std::vector<ExprPtr>       values; // empty = default
    std::vector<std::string>   bindings; // for enum payloads
    std::vector<StmtPtr>       body;
    bool                       isDefault = false;
};

struct SwitchStmt : Stmt {
    ExprPtr                 expr;
    std::vector<SwitchCase> cases;
    SwitchStmt(ExprPtr e, std::vector<SwitchCase> c, int l, int col_)
        : expr(std::move(e)), cases(std::move(c)) {
        kind = Kind::Switch; line=l; col=col_;
    }
};

struct WaitStmt : Stmt {
    ExprPtr duration;
    WaitStmt(ExprPtr d, int l, int c) : duration(std::move(d)) {
        kind = Kind::Wait; line=l; col=c;
    }
};

struct IncludeStmt : Stmt {
    std::string path;
    bool        isSystem; // <> vs ""
    IncludeStmt(const std::string& p, bool sys, int l, int c)
        : path(p), isSystem(sys) { kind = Kind::Include; line=l; col=c; }
};

// -----------------------------------------------------------------------
// Function parameter
// -----------------------------------------------------------------------
struct Param {
    std::string    name;
    TypeAnnotation type;
};

// -----------------------------------------------------------------------
// Top-level declarations
// -----------------------------------------------------------------------
struct FuncDecl : Stmt {
    std::string                name;
    std::vector<Param>         params;
    std::optional<TypeAnnotation> returnType;
    std::unique_ptr<BlockStmt> body;
    bool                       pub = false;
    FuncDecl(const std::string& n, std::vector<Param> p,
             std::optional<TypeAnnotation> r,
             std::unique_ptr<BlockStmt> b, bool pub_, int l, int c)
        : name(n), params(std::move(p)), returnType(r),
          body(std::move(b)), pub(pub_) {
        kind = Kind::FuncDecl; line=l; col=c;
    }
};

struct FieldDecl {
    std::string    name;
    TypeAnnotation type;
    ExprPtr        init;
    bool           mut = false;
    bool           pub = false;
};

struct NodeDecl : Stmt {
    std::string              name;
    std::string              base;   // Node2D, Sprite2D, etc.
    std::vector<FieldDecl>   fields;
    std::vector<std::unique_ptr<FuncDecl>> methods;
    NodeDecl(const std::string& n, const std::string& b,
             std::vector<FieldDecl> f,
             std::vector<std::unique_ptr<FuncDecl>> m,
             int l, int c)
        : name(n), base(b), fields(std::move(f)), methods(std::move(m)) {
        kind = Kind::NodeDecl; line=l; col=c;
    }
};

struct StructDecl : Stmt {
    std::string            name;
    std::vector<FieldDecl> fields;
    StructDecl(const std::string& n, std::vector<FieldDecl> f, int l, int c)
        : name(n), fields(std::move(f)) { kind = Kind::StructDecl; line=l; col=c; }
};

struct EnumVariant {
    std::string                        name;
    std::optional<TypeAnnotation>      payload; // Jumping(F64)
};

struct EnumDecl : Stmt {
    std::string                name;
    std::vector<EnumVariant>   variants;
    EnumDecl(const std::string& n, std::vector<EnumVariant> v, int l, int c)
        : name(n), variants(std::move(v)) { kind = Kind::EnumDecl; line=l; col=c; }
};

struct ClassDecl : Stmt {
    std::string              name;
    std::string              base;   // optional inheritance
    std::vector<FieldDecl>   fields;
    std::vector<std::unique_ptr<FuncDecl>> methods;
    ClassDecl(const std::string& n, const std::string& b,
              std::vector<FieldDecl> f,
              std::vector<std::unique_ptr<FuncDecl>> m,
              int l, int c)
        : name(n), base(b), fields(std::move(f)), methods(std::move(m)) {
        kind = Kind::ClassDecl; line=l; col=c;
    }
};

// -----------------------------------------------------------------------
// Program -- the root of the AST
// -----------------------------------------------------------------------
struct Program {
    std::string          filename;
    std::vector<StmtPtr> stmts;
};

} // namespace KonScript
