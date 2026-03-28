#pragma once
// -----------------------------------------------------------------------
// KonScript Type Checker
// Walks the AST, resolves types, and reports errors.
// -----------------------------------------------------------------------
#include "ast.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <sstream>

namespace KonScript {

// -----------------------------------------------------------------------
// Resolved type -- what the type checker works with internally
// -----------------------------------------------------------------------
struct Type {
    enum class Kind {
        Void,
        I8, I16, I32, I64,
        U8, U16, U32, U64,
        F32, F64,
        Bool,
        Str,
        String,
        Vec2,
        Array,
        Tuple,
        Node,       // any engine node type
        Struct,
        Enum,
        Class,
        Nullable,   // T?
        Unknown,    // error recovery
    } kind = Kind::Unknown;

    std::string name;           // for named types (Node2D, MyStruct, etc.)
    std::vector<Type> inner;   // array element, tuple elements, nullable inner
    bool nullable = false;

    static Type make(Kind k, const std::string& n = "") {
        Type t; t.kind = k; t.name = n; return t;
    }
    static Type makeNullable(Type inner_) {
        Type t; t.kind = Kind::Nullable;
        t.inner.push_back(std::move(inner_));
        t.nullable = true;
        return t;
    }
    static Type makeArray(Type elem) {
        Type t; t.kind = Kind::Array;
        t.inner.push_back(std::move(elem));
        return t;
    }
    static Type makeTuple(std::vector<Type> elems) {
        Type t; t.kind = Kind::Tuple;
        t.inner = std::move(elems);
        return t;
    }
    static Type unknown() { return make(Kind::Unknown); }
    static Type void_()   { return make(Kind::Void); }

    bool isNumeric() const {
        return kind == Kind::I8  || kind == Kind::I16 ||
               kind == Kind::I32 || kind == Kind::I64 ||
               kind == Kind::U8  || kind == Kind::U16 ||
               kind == Kind::U32 || kind == Kind::U64 ||
               kind == Kind::F32 || kind == Kind::F64;
    }
    bool isInteger() const {
        return kind == Kind::I8  || kind == Kind::I16 ||
               kind == Kind::I32 || kind == Kind::I64 ||
               kind == Kind::U8  || kind == Kind::U16 ||
               kind == Kind::U32 || kind == Kind::U64;
    }
    bool isUnknown() const { return kind == Kind::Unknown; }
    bool isVoid()    const { return kind == Kind::Void; }

    std::string toString() const {
        switch (kind) {
            case Kind::Void:    return "void";
            case Kind::I8:      return "I8";
            case Kind::I16:     return "I16";
            case Kind::I32:     return "I32";
            case Kind::I64:     return "I64";
            case Kind::U8:      return "U8";
            case Kind::U16:     return "U16";
            case Kind::U32:     return "U32";
            case Kind::U64:     return "U64";
            case Kind::F32:     return "F32";
            case Kind::F64:     return "F64";
            case Kind::Bool:    return "Bool";
            case Kind::Str:     return "str";
            case Kind::String:  return "String";
            case Kind::Vec2:    return "Vec2";
            case Kind::Unknown: return "?";
            case Kind::Array:
                return "[" + (inner.empty() ? "?" : inner[0].toString()) + "]";
            case Kind::Nullable:
                return (inner.empty() ? "?" : inner[0].toString()) + "?";
            case Kind::Tuple: {
                std::string s = "(";
                for (size_t i = 0; i < inner.size(); i++) {
                    if (i > 0) s += ", ";
                    s += inner[i].toString();
                }
                return s + ")";
            }
            default: return name.empty() ? "?" : name;
        }
    }

    bool operator==(const Type& o) const {
        if (kind != o.kind) return false;
        if (kind == Kind::Node || kind == Kind::Struct ||
            kind == Kind::Enum || kind == Kind::Class)
            return name == o.name;
        if (kind == Kind::Array || kind == Kind::Nullable)
            return inner.size() == o.inner.size() &&
                   (inner.empty() || inner[0] == o.inner[0]);
        return true;
    }
    bool operator!=(const Type& o) const { return !(*this == o); }
};

// -----------------------------------------------------------------------
// Symbol -- a declared variable, function, field, etc.
// -----------------------------------------------------------------------
struct Symbol {
    std::string name;
    Type        type;
    bool        mut      = false;
    bool        pub      = false;
    bool        isFunc   = false;
    std::vector<Type> paramTypes;
    Type              returnType;
};

// -----------------------------------------------------------------------
// Scope -- a lexical scope with a symbol table
// -----------------------------------------------------------------------
struct Scope {
    std::unordered_map<std::string, Symbol> symbols;
    Scope* parent = nullptr;

    Symbol* lookup(const std::string& name) {
        auto it = symbols.find(name);
        if (it != symbols.end()) return &it->second;
        if (parent) return parent->lookup(name);
        return nullptr;
    }

    bool define(const Symbol& sym) {
        if (symbols.count(sym.name)) return false; // already defined
        symbols[sym.name] = sym;
        return true;
    }
};

// -----------------------------------------------------------------------
// Type info for declared structs, enums, classes, nodes
// -----------------------------------------------------------------------
struct StructInfo {
    std::string name;
    std::vector<std::pair<std::string, Type>> fields;
};

struct EnumInfo {
    std::string name;
    struct Variant {
        std::string           name;
        std::optional<Type>   payload;
    };
    std::vector<Variant> variants;

    const Variant* findVariant(const std::string& n) const {
        for (auto& v : variants) if (v.name == n) return &v;
        return nullptr;
    }
};

struct FuncInfo {
    std::string        name;
    std::vector<Type>  params;
    Type               returnType;
    bool               pub = false;
};

// -----------------------------------------------------------------------
// Type checker
// -----------------------------------------------------------------------
class TypeChecker {
public:
    struct Error {
        std::string message;
        int line, col;
    };

    const std::vector<Error>& errors() const { return m_errors; }
    bool hasErrors() const { return !m_errors.empty(); }

    void check(const Program& prog) {
        // First pass: collect all top-level declarations
        collectDeclarations(prog);

        // Register built-in engine functions
        registerBuiltins();

        // Second pass: type-check all bodies
        Scope global;
        global.parent = nullptr;

        // Put top-level symbols into global scope
        for (auto& [name, sym] : m_globalSymbols)
            global.define(sym);

        for (auto& stmt : prog.stmts) {
            // Skip func/node decls -- bodies are checked in checkFuncDecl
            // called from collectDeclarations context. Re-checking here
            // causes infinite recursion via mutual calls.
            if (stmt->kind == Stmt::Kind::FuncDecl ||
                stmt->kind == Stmt::Kind::NodeDecl  ||
                stmt->kind == Stmt::Kind::ClassDecl ||
                stmt->kind == Stmt::Kind::Include)
                continue;
            m_checking.clear();
            checkStmt(stmt.get(), &global, Type::void_());
        }
    }

private:
    std::vector<Error> m_errors;
    std::unordered_map<std::string, StructInfo> m_structs;
    std::unordered_map<std::string, EnumInfo>   m_enums;
    std::unordered_map<std::string, Symbol>     m_globalSymbols;

    // Known engine node types
    std::unordered_set<std::string> m_checking;  // recursion guard
    std::unordered_set<std::string> m_nodeTypes = {
        "Node", "Node2D", "Sprite2D", "Collider2D",
        "AnimationPlayer", "Camera2D"
    };

    void error(const std::string& msg, int line, int col) {
        m_errors.push_back({msg, line, col});
    }

    // -----------------------------------------------------------------------
    // Convert TypeAnnotation -> Type
    // -----------------------------------------------------------------------
    Type resolve(const TypeAnnotation& ta) {
        Type t;
        if (ta.isTuple) {
            std::vector<Type> elems;
            for (auto& tt : ta.tupleTypes) elems.push_back(resolve(tt));
            t = Type::makeTuple(std::move(elems));
        } else if (ta.isArray) {
            TypeAnnotation inner; inner.base = ta.base;
            t = Type::makeArray(resolve(inner));
        } else {
            t = resolveBase(ta.base);
        }
        if (ta.nullable) t = Type::makeNullable(t);
        return t;
    }

    Type resolveBase(const std::string& name) {
        if (name == "I8")     return Type::make(Type::Kind::I8);
        if (name == "I16")    return Type::make(Type::Kind::I16);
        if (name == "I32")    return Type::make(Type::Kind::I32);
        if (name == "I64")    return Type::make(Type::Kind::I64);
        if (name == "U8")     return Type::make(Type::Kind::U8);
        if (name == "U16")    return Type::make(Type::Kind::U16);
        if (name == "U32")    return Type::make(Type::Kind::U32);
        if (name == "U64")    return Type::make(Type::Kind::U64);
        if (name == "F32")    return Type::make(Type::Kind::F32);
        if (name == "F64")    return Type::make(Type::Kind::F64);
        if (name == "Bool")   return Type::make(Type::Kind::Bool);
        if (name == "str")    return Type::make(Type::Kind::Str);
        if (name == "String") return Type::make(Type::Kind::String);
        if (name == "Vec2")   return Type::make(Type::Kind::Vec2);
		if (name == "Color")  return Type::make(Type::Kind::Struct, "Color");
        if (m_nodeTypes.count(name))
            return Type::make(Type::Kind::Node, name);
        if (m_structs.count(name))
            return Type::make(Type::Kind::Struct, name);
        if (m_enums.count(name))
            return Type::make(Type::Kind::Enum, name);
        // Unknown user type -- might be a class or forward ref
        return Type::make(Type::Kind::Class, name);
    }

    // -----------------------------------------------------------------------
    // First pass: collect top-level declarations
    // -----------------------------------------------------------------------
    void collectDeclarations(const Program& prog) {
        for (auto& stmt : prog.stmts) {
            if (stmt->kind == Stmt::Kind::EnumDecl) {
                auto* e = static_cast<EnumDecl*>(stmt.get());
                EnumInfo info;
                info.name = e->name;
                for (auto& v : e->variants) {
                    EnumInfo::Variant ev;
                    ev.name = v.name;
                    if (v.payload) ev.payload = resolve(*v.payload);
                    info.variants.push_back(std::move(ev));
                }
                m_enums[e->name] = std::move(info);

            } else if (stmt->kind == Stmt::Kind::StructDecl) {
                auto* s = static_cast<StructDecl*>(stmt.get());
                StructInfo info;
                info.name = s->name;
                for (auto& f : s->fields) {
                    info.fields.push_back({f.name, resolve(f.type)});
                }
                m_structs[s->name] = std::move(info);

            } else if (stmt->kind == Stmt::Kind::FuncDecl) {
                auto* f = static_cast<FuncDecl*>(stmt.get());
                Symbol sym;
                sym.name    = f->name;
                sym.isFunc  = true;
                sym.pub     = f->pub;
                sym.returnType = f->returnType
                    ? resolve(*f->returnType) : Type::void_();
                for (auto& p : f->params)
                    sym.paramTypes.push_back(resolve(p.type));
                sym.type = sym.returnType;
                m_globalSymbols[f->name] = sym;

            } else if (stmt->kind == Stmt::Kind::NodeDecl) {
                auto* n = static_cast<NodeDecl*>(stmt.get());
                // Register as a node type
                m_nodeTypes.insert(n->name);
                // Register methods as symbols scoped to node (simplified)
                for (auto& m : n->methods) {
                    Symbol sym;
                    sym.name = n->name + "::" + m->name;
                    sym.isFunc = true;
                    sym.returnType = m->returnType
                        ? resolve(*m->returnType) : Type::void_();
                    for (auto& p : m->params)
                        sym.paramTypes.push_back(resolve(p.type));
                    m_globalSymbols[sym.name] = sym;
                }

            } else if (stmt->kind == Stmt::Kind::Const) {
                auto* c = static_cast<ConstStmt*>(stmt.get());
                Symbol sym;
                sym.name = c->name;
                sym.type = resolve(c->type);
                sym.mut  = false;
                m_globalSymbols[c->name] = sym;
            }
        }
    }

    // -----------------------------------------------------------------------
    // Register built-in engine functions
    // -----------------------------------------------------------------------
    void registerBuiltins() {
        auto reg = [&](const std::string& name,
                       std::vector<Type> params,
                       Type ret) {
            Symbol s;
            s.name       = name;
            s.isFunc     = true;
            s.paramTypes = std::move(params);
            s.returnType = ret;
            s.type       = ret;
            m_globalSymbols[name] = s;
        };

        auto I32  = Type::make(Type::Kind::I32);
        auto F64  = Type::make(Type::Kind::F64);
        auto Bool = Type::make(Type::Kind::Bool);
        auto Str  = Type::make(Type::Kind::Str);
        auto Void = Type::void_();
        auto Node = Type::make(Type::Kind::Node, "Node");

		// Color type + engine presets
		auto Color = Type::make(Type::Kind::Struct, "Color");
		auto regConst = [&](const std::string& n, Type t) {
			Symbol s; s.name = n; s.type = t; s.mut = false;
			m_globalSymbols[n] = s;
		};
		regConst("WHITE",      Color);
		regConst("BLACK",      Color);
		regConst("RED",        Color);
		regConst("GREEN",      Color);
		regConst("BLUE",       Color);
		regConst("YELLOW",     Color);
		regConst("CYAN",       Color);
		regConst("MAGENTA",    Color);
		regConst("ORANGE",     Color);
		regConst("GRAY",       Color);
		regConst("DARKGRAY",   Color);
		regConst("LIGHTGRAY",  Color);
		regConst("PINK",       Color);
		regConst("PURPLE",     Color);
		regConst("BROWN",      Color);
		regConst("BLANK",      Color);

        // Window
        reg("InitWindow",    {I32, I32, Str}, Void);
        reg("WindowShouldClose", {}, Bool);
        reg("Present",       {}, Void);
        reg("PollEvents",    {}, Void);
        reg("ClearBackground", {F64, F64, F64}, Void);
        reg("SetTargetFPS",  {I32}, Void);
        reg("GetDeltaTime",  {}, F64);
        reg("GetFPS",        {}, I32);
        reg("GetTime",       {}, F64);
        reg("SetTimeScale",  {F64}, Void);
        reg("DebugMode",     {Bool}, Void);
        reg("IsDebugMode",   {}, Bool);
		reg("GetWindowWidth",  {}, I32);
		reg("GetWindowHeight", {}, I32);
		reg("SetVsync",        {Bool}, Void);

        // Input
        reg("KeyDown",       {I32}, Bool);
        reg("KeyPressed",    {I32}, Bool);
        reg("KeyReleased",   {I32}, Bool);
        reg("MouseDown",     {I32}, Bool);
        reg("MousePressed",  {I32}, Bool);
        reg("GetMouseX",     {}, F64);
        reg("GetMouseY",     {}, F64);
		reg("MouseReleased",   {I32}, Bool);
		reg("GetMouseDeltaX",  {}, F64);
		reg("GetMouseDeltaY",  {}, F64);
		reg("GetMouseScroll",  {}, F64);

        // Node
        reg("GetNode",       {Str}, Type::makeNullable(Node));
        reg("GetChild",      {Str}, Type::makeNullable(Node));
        reg("GetScene",      {}, Type::make(Type::Kind::Node, "Scene"));
        reg("Emit",          {Str}, Void);
        reg("Connect",       {Str, Str}, Void);

        // Output -- Print is variadic, register with empty params to skip arg checking
        reg("Print",         {}, Void);
        reg("ToString",      {}, Str);  // ToString(any) -> str

        // Drawing
        reg("DrawRectangle", {}, Void);
        reg("DrawCircle",    {}, Void);
        reg("DrawLine",      {}, Void);
        reg("DrawTexture",   {}, Void);
        reg("DrawText",      {}, Void);

        // Camera
        reg("BeginCamera2D", {}, Void);
        reg("EndCamera2D",   {}, Void);

        // Audio
        reg("PlaySound",     {}, Void);
        reg("PlayMusic",     {}, Void);
        reg("StopMusic",     {}, Void);
        reg("PauseMusic",    {}, Void);
        reg("ResumeMusic",   {}, Void);
        reg("SetMusicVolume",{}, Void);
    }

    // -----------------------------------------------------------------------
    // Check statements
    // -----------------------------------------------------------------------
    void checkStmt(const Stmt* s, Scope* scope, const Type& returnType) {
        if (!s) return;
        switch (s->kind) {

            case Stmt::Kind::Let: {
                auto* let = static_cast<const LetStmt*>(s);
                Type declared = resolve(let->type);
                Type actual   = checkExpr(let->init.get(), scope);
                if (!actual.isUnknown() && !declared.isUnknown() &&
                    !typesCompatible(declared, actual))
                    error("type mismatch: expected '" + declared.toString() +
                          "' but got '" + actual.toString() + "'",
                          let->line, let->col);
                Symbol sym;
                sym.name = let->name;
                sym.type = declared;
                sym.mut  = let->mut;
                if (!scope->define(sym))
                    error("'" + let->name + "' is already declared",
                          let->line, let->col);
                break;
            }

            case Stmt::Kind::Const: {
                auto* c = static_cast<const ConstStmt*>(s);
                Type declared = resolve(c->type);
                Type actual   = checkExpr(c->init.get(), scope);
                if (!actual.isUnknown() && !declared.isUnknown() &&
                    !typesCompatible(declared, actual))
                    error("type mismatch in const '" + c->name + "'",
                          c->line, c->col);
                Symbol sym;
                sym.name = c->name;
                sym.type = declared;
                sym.mut  = false;
                scope->define(sym);
                break;
            }

            case Stmt::Kind::ExprStmt: {
                auto* e = static_cast<const ExprStmt*>(s);
                checkExpr(e->expr.get(), scope);
                break;
            }

            case Stmt::Kind::Return: {
                auto* r = static_cast<const ReturnStmt*>(s);
                if (r->value) {
                    Type actual = checkExpr(r->value.get(), scope);
                    if (!returnType.isVoid() && !actual.isUnknown() &&
                        !typesCompatible(returnType, actual))
                        error("return type mismatch: expected '" +
                              returnType.toString() + "' but got '" +
                              actual.toString() + "'",
                              r->line, r->col);
                } else if (!returnType.isVoid()) {
                    error("missing return value", r->line, r->col);
                }
                break;
            }

            case Stmt::Kind::If: {
                auto* i = static_cast<const IfStmt*>(s);
                Type cond = checkExpr(i->cond.get(), scope);
                if (!cond.isUnknown() && cond.kind != Type::Kind::Bool)
                    error("if condition must be Bool, got '" +
                          cond.toString() + "'", i->line, i->col);
                checkBlock(i->then_.get(), scope, returnType);
                if (i->else_) checkBlock(i->else_.get(), scope, returnType);
                break;
            }

            case Stmt::Kind::While: {
                auto* w = static_cast<const WhileStmt*>(s);
                Type cond = checkExpr(w->cond.get(), scope);
                if (!cond.isUnknown() && cond.kind != Type::Kind::Bool)
                    error("while condition must be Bool", w->line, w->col);
                checkBlock(w->body.get(), scope, returnType);
                break;
            }

            case Stmt::Kind::Loop: {
                auto* l = static_cast<const LoopStmt*>(s);
                checkBlock(l->body.get(), scope, returnType);
                break;
            }

            case Stmt::Kind::ForIn: {
                auto* f = static_cast<const ForInStmt*>(s);
                checkExpr(f->iterable.get(), scope);
                Scope inner; inner.parent = scope;
                Symbol sym;
                sym.name = f->var;
                sym.type = resolve(f->type);
                sym.mut  = true;
                inner.define(sym);
                checkBlock(f->body.get(), &inner, returnType);
                break;
            }

            case Stmt::Kind::ForC: {
                auto* f = static_cast<const ForCStmt*>(s);
                Scope inner; inner.parent = scope;
                Symbol sym;
                sym.name = f->var;
                sym.type = resolve(f->type);
                sym.mut  = true;
                inner.define(sym);
                checkExpr(f->init.get(), &inner);
                Type cond = checkExpr(f->cond.get(), &inner);
                if (!cond.isUnknown() && cond.kind != Type::Kind::Bool)
                    error("for condition must be Bool", f->line, f->col);
                checkExpr(f->step.get(), &inner);
                checkBlock(f->body.get(), &inner, returnType);
                break;
            }

            case Stmt::Kind::Switch: {
                auto* sw = static_cast<const SwitchStmt*>(s);
                checkExpr(sw->expr.get(), scope);
                for (auto& c : sw->cases) {
                    for (auto& v : c.values)
                        checkExpr(v.get(), scope);
                    Scope caseScope; caseScope.parent = scope;
                    for (auto& b : c.body)
                        checkStmt(b.get(), &caseScope, returnType);
                }
                break;
            }

            case Stmt::Kind::Block: {
                auto* b = static_cast<const BlockStmt*>(s);
                checkBlock(b, scope, returnType);
                break;
            }

            case Stmt::Kind::FuncDecl: {
                auto* f = static_cast<const FuncDecl*>(s);
                checkFuncDecl(f, scope);
                break;
            }

            case Stmt::Kind::NodeDecl: {
                auto* n = static_cast<const NodeDecl*>(s);
                checkNodeDecl(n, scope);
                break;
            }

            case Stmt::Kind::StructDecl:
            case Stmt::Kind::EnumDecl:
            case Stmt::Kind::ClassDecl:
            case Stmt::Kind::Include:
            case Stmt::Kind::Break:
            case Stmt::Kind::Continue:
            case Stmt::Kind::Wait:
                // Nothing to type-check beyond what was collected
                break;

            default:
                break;
        }
    }

    void checkBlock(const BlockStmt* block, Scope* parent,
                    const Type& returnType) {
        if (!block) return;
        Scope scope; scope.parent = parent;
        for (auto& s : block->stmts)
            checkStmt(s.get(), &scope, returnType);
    }

    void checkFuncDecl(const FuncDecl* f, Scope* scope) {
        // Recursion guard -- skip if already checking this function
        if (m_checking.count(f->name)) return;
        m_checking.insert(f->name);

        Scope funcScope; funcScope.parent = scope;
        for (auto& p : f->params) {
            Symbol sym;
            sym.name = p.name;
            sym.type = resolve(p.type);
            sym.mut  = true;
            funcScope.define(sym);
        }
        Type ret = f->returnType ? resolve(*f->returnType) : Type::void_();
        checkBlock(f->body.get(), &funcScope, ret);
        m_checking.erase(f->name);
    }

    void checkNodeDecl(const NodeDecl* n, Scope* scope) {
        // Build a scope with the node's fields and inherited Node2D properties
        Scope nodeScope; nodeScope.parent = scope;

        // Built-in Node2D fields
        auto F64  = Type::make(Type::Kind::F64);
        auto Bool = Type::make(Type::Kind::Bool);
        for (auto& name : {"x","y","scaleX","scaleY","rotation",
                           "originX","originY","alpha"}) {
            Symbol s; s.name = name; s.type = F64; s.mut = true;
            nodeScope.define(s);
        }
        {
            Symbol s; s.name = "active"; s.type = Bool; s.mut = true;
            nodeScope.define(s);
            Symbol s2; s2.name = "name";
            s2.type = Type::make(Type::Kind::Str); s2.mut = false;
            nodeScope.define(s2);
        }

        // Node's own fields
        for (auto& f : n->fields) {
            Symbol sym;
            sym.name = f.name;
            sym.type = resolve(f.type);
            sym.mut  = f.mut;
            if (f.init) {
                Type actual = checkExpr(f.init.get(), &nodeScope);
                if (!actual.isUnknown() && !sym.type.isUnknown() &&
                    !typesCompatible(sym.type, actual))
                    error("field '" + f.name + "' type mismatch",
                          0, 0);
            }
            nodeScope.define(sym);
        }

        // Check methods
        for (auto& m : n->methods)
            checkFuncDecl(m.get(), &nodeScope);
    }

    // -----------------------------------------------------------------------
    // Check expressions -- returns the type of the expression
    // -----------------------------------------------------------------------
    Type checkExpr(const Expr* e, Scope* scope) {
        if (!e) return Type::unknown();
        switch (e->kind) {

            case Expr::Kind::IntLit:
                return Type::make(Type::Kind::I32);

            case Expr::Kind::FloatLit:
                return Type::make(Type::Kind::F64);

            case Expr::Kind::BoolLit:
                return Type::make(Type::Kind::Bool);

            case Expr::Kind::StrLit:
                return Type::make(Type::Kind::Str);

            case Expr::Kind::NullLit:
            case Expr::Kind::NoneLit:
                return Type::makeNullable(Type::unknown());

            case Expr::Kind::Ident: {
                auto* id = static_cast<const IdentExpr*>(e);
                // Engine namespaces -- Key.A, Mouse.Left, Gamepad.A etc.
                static const std::unordered_set<std::string> engineNamespaces = {
                    "Key", "Mouse", "Gamepad"
                };
                if (engineNamespaces.count(id->name))
                    return Type::unknown();
                // Check enum variants first
                for (auto& [enumName, info] : m_enums) {
                    if (info.findVariant(id->name))
                        return Type::make(Type::Kind::Enum, enumName);
                }
                auto* sym = scope->lookup(id->name);
                if (!sym) {
                    error("undefined variable '" + id->name + "'",
                          e->line, e->col);
                    return Type::unknown();
                }
                return sym->type;
            }

            case Expr::Kind::Assign: {
                auto* a = static_cast<const AssignExpr*>(e);
                // Check target is mutable
                if (a->target->kind == Expr::Kind::Ident) {
                    auto* id = static_cast<const IdentExpr*>(a->target.get());
                    auto* sym = scope->lookup(id->name);
                    if (sym && !sym->mut && !sym->isFunc)
                        error("cannot assign to immutable variable '" +
                              id->name + "'", e->line, e->col);
                }
                Type left  = checkExpr(a->target.get(), scope);
                Type right = checkExpr(a->value.get(), scope);
                if (!left.isUnknown() && !right.isUnknown() &&
                    !typesCompatible(left, right))
                    error("assignment type mismatch: expected '" +
                          left.toString() + "' but got '" +
                          right.toString() + "'", e->line, e->col);
                return left;
            }

            case Expr::Kind::Binary: {
                auto* b = static_cast<const BinaryExpr*>(e);
                Type left  = checkExpr(b->left.get(), scope);
                Type right = checkExpr(b->right.get(), scope);
                // Comparison operators always return Bool
                if (b->op == "==" || b->op == "!=" ||
                    b->op == "<"  || b->op == ">"  ||
                    b->op == "<=" || b->op == ">=" ||
                    b->op == "&&" || b->op == "||")
                    return Type::make(Type::Kind::Bool);
                // Arithmetic -- return widest numeric type
                if (left.isNumeric() && right.isNumeric())
                    return widenNumeric(left, right);
                // String concatenation
                if ((left.kind == Type::Kind::Str ||
                     left.kind == Type::Kind::String) && b->op == "+")
                    return left;
                return left.isUnknown() ? right : left;
            }

            case Expr::Kind::Unary: {
                auto* u = static_cast<const UnaryExpr*>(e);
                Type t = checkExpr(u->operand.get(), scope);
                if (u->op == "!" && t.kind != Type::Kind::Bool &&
                    !t.isUnknown())
                    error("'!' requires Bool operand", e->line, e->col);
                return t;
            }

            case Expr::Kind::Call: {
                auto* c = static_cast<const CallExpr*>(e);
                // Resolve callee
                if (c->callee->kind == Expr::Kind::Ident) {
                    auto* id = static_cast<const IdentExpr*>(c->callee.get());
                    auto* sym = scope->lookup(id->name);
                    if (!sym) {
                        // Check global symbols
                        auto it = m_globalSymbols.find(id->name);
                        if (it == m_globalSymbols.end()) {
                            // Unknown function -- could be a user-defined node
                            // constructor or engine function not yet registered.
                            // Warn but don't error to allow engine-mode flexibility.
                            for (auto& a : c->args) checkExpr(a.get(), scope);
                            return Type::unknown();
                        }
                        sym = &it->second;
                    }
                    // Check arg count (varargs functions like Print get a pass)
                    if (id->name != "Print" && !sym->paramTypes.empty() &&
                        c->args.size() != sym->paramTypes.size()) {
                        error("function '" + id->name + "' expects " +
                              std::to_string(sym->paramTypes.size()) +
                              " argument(s), got " +
                              std::to_string(c->args.size()),
                              e->line, e->col);
                    }
                    // Check arg types
                    for (size_t i = 0; i < c->args.size() &&
                                       i < sym->paramTypes.size(); i++) {
                        Type at = checkExpr(c->args[i].get(), scope);
                        if (!at.isUnknown() &&
                            !typesCompatible(sym->paramTypes[i], at))
                            error("argument " + std::to_string(i+1) +
                                  " type mismatch: expected '" +
                                  sym->paramTypes[i].toString() +
                                  "' but got '" + at.toString() + "'",
                                  e->line, e->col);
                    }
                    return sym->returnType;
                }
                // Method call or other callee -- just check args
                checkExpr(c->callee.get(), scope);
                for (auto& a : c->args) checkExpr(a.get(), scope);
                return Type::unknown();
            }

            case Expr::Kind::Member:
            case Expr::Kind::SafeMember: {
                auto* m = static_cast<const MemberExpr*>(e);
                Type obj = checkExpr(m->object.get(), scope);
                // Safe member on non-nullable is a warning but not an error
                if (e->kind == Expr::Kind::SafeMember &&
                    obj.kind != Type::Kind::Nullable && !obj.isUnknown())
                    error("safe access '?.' used on non-nullable type '" +
                          obj.toString() + "'", e->line, e->col);
                // Struct field access
                if (obj.kind == Type::Kind::Struct) {
                    auto it = m_structs.find(obj.name);
                    if (it != m_structs.end()) {
                        for (auto& [fname, ftype] : it->second.fields)
                            if (fname == m->member) return ftype;
                        error("struct '" + obj.name + "' has no field '" +
                              m->member + "'", e->line, e->col);
                    }
                }
                // For node/class members we return unknown (no full class model yet)
                return Type::unknown();
            }

            case Expr::Kind::Index: {
                auto* i = static_cast<const IndexExpr*>(e);
                Type obj = checkExpr(i->object.get(), scope);
                checkExpr(i->index.get(), scope);
                if (obj.kind == Type::Kind::Array && !obj.inner.empty())
                    return obj.inner[0];
                if (obj.kind == Type::Kind::Str ||
                    obj.kind == Type::Kind::String)
                    return Type::make(Type::Kind::U8);
                return Type::unknown();
            }

            case Expr::Kind::Cast: {
                auto* c = static_cast<const CastExpr*>(e);
                checkExpr(c->value.get(), scope);
                return resolve(c->target);
            }

            case Expr::Kind::NullCoal: {
                auto* n = static_cast<const NullCoalExpr*>(e);
                Type left = checkExpr(n->left.get(), scope);
                Type right = checkExpr(n->right.get(), scope);
                // Result is the inner type of the nullable
                if (left.kind == Type::Kind::Nullable && !left.inner.empty())
                    return left.inner[0];
                return right;
            }

            case Expr::Kind::ForceUnwrap: {
                auto* f = static_cast<const ForceUnwrapExpr*>(e);
                Type t = checkExpr(f->value.get(), scope);
                if (t.kind != Type::Kind::Nullable && !t.isUnknown())
                    error("force unwrap '!' on non-nullable type '" +
                          t.toString() + "'", e->line, e->col);
                if (t.kind == Type::Kind::Nullable && !t.inner.empty())
                    return t.inner[0];
                return Type::unknown();
            }

            case Expr::Kind::Some_: {
                auto* s = static_cast<const SomeExpr*>(e);
                Type inner = checkExpr(s->value.get(), scope);
                return Type::makeNullable(inner);
            }

            case Expr::Kind::ArrayLit: {
                auto* a = static_cast<const ArrayLitExpr*>(e);
                if (a->elements.empty())
                    return Type::makeArray(Type::unknown());
                Type elem = checkExpr(a->elements[0].get(), scope);
                for (size_t i = 1; i < a->elements.size(); i++) {
                    Type et = checkExpr(a->elements[i].get(), scope);
                    if (!et.isUnknown() && !typesCompatible(elem, et))
                        error("array element type mismatch at index " +
                              std::to_string(i), e->line, e->col);
                }
                return Type::makeArray(elem);
            }

            case Expr::Kind::TupleLit: {
                auto* t = static_cast<const TupleLitExpr*>(e);
                std::vector<Type> elems;
                for (auto& el : t->elements)
                    elems.push_back(checkExpr(el.get(), scope));
                return Type::makeTuple(std::move(elems));
            }

            case Expr::Kind::StructInit: {
                auto* si = static_cast<const StructInitExpr*>(e);
                auto it = m_structs.find(si->typeName);
                if (it == m_structs.end()) {
                    error("unknown struct '" + si->typeName + "'",
                          e->line, e->col);
                    for (auto& f : si->fields) checkExpr(f.value.get(), scope);
                    return Type::unknown();
                }
                for (auto& f : si->fields) {
                    Type actual = checkExpr(f.value.get(), scope);
                    // Find expected type
                    for (auto& [fname, ftype] : it->second.fields) {
                        if (fname == f.name && !actual.isUnknown() &&
                            !typesCompatible(ftype, actual))
                            error("field '" + f.name + "' type mismatch",
                                  e->line, e->col);
                    }
                }
                return Type::make(Type::Kind::Struct, si->typeName);
            }

            case Expr::Kind::Range:
                checkExpr(static_cast<const RangeExpr*>(e)->from.get(), scope);
                checkExpr(static_cast<const RangeExpr*>(e)->to.get(), scope);
                return Type::make(Type::Kind::Struct, "Range");

            case Expr::Kind::Spawn:
                checkExpr(static_cast<const SpawnExpr*>(e)->call.get(), scope);
                return Type::void_();

            default:
                return Type::unknown();
        }
    }

    // -----------------------------------------------------------------------
    // Type compatibility
    // -----------------------------------------------------------------------
    bool typesCompatible(const Type& expected, const Type& actual) {
        if (expected.isUnknown() || actual.isUnknown()) return true;

        // Exact match
        if (expected == actual) return true;

        // Numeric widening: I32 -> I64, F32 -> F64 etc.
        if (expected.isNumeric() && actual.isNumeric()) return true;

        // Nullable: T? is compatible with T and null
        if (expected.kind == Type::Kind::Nullable) {
            if (actual.kind == Type::Kind::Nullable) return true;
            if (!expected.inner.empty())
                return typesCompatible(expected.inner[0], actual);
        }

        // null/None is compatible with any nullable
        if (actual.kind == Type::Kind::Nullable &&
            actual.inner.empty()) return true;

        // str and String are interchangeable for now
        if ((expected.kind == Type::Kind::Str ||
             expected.kind == Type::Kind::String) &&
            (actual.kind == Type::Kind::Str ||
             actual.kind == Type::Kind::String)) return true;

        // Node subtype: any Node2D/Sprite2D etc. is compatible with Node
        if (expected.kind == Type::Kind::Node &&
            actual.kind == Type::Kind::Node) return true;

        return false;
    }

    // Widen two numeric types to the wider one
    Type widenNumeric(const Type& a, const Type& b) {
        // Float beats int
        if (a.kind == Type::Kind::F64 || b.kind == Type::Kind::F64)
            return Type::make(Type::Kind::F64);
        if (a.kind == Type::Kind::F32 || b.kind == Type::Kind::F32)
            return Type::make(Type::Kind::F32);
        if (a.kind == Type::Kind::I64 || b.kind == Type::Kind::I64)
            return Type::make(Type::Kind::I64);
        if (a.kind == Type::Kind::U64 || b.kind == Type::Kind::U64)
            return Type::make(Type::Kind::U64);
        if (a.kind == Type::Kind::I32 || b.kind == Type::Kind::I32)
            return Type::make(Type::Kind::I32);
        return a;
    }
};

} // namespace KonScript
