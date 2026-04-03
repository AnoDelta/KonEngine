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
        Result,     // Result<T> — ok/value/error
        HashMap,    // HashMap<K, V>
        FuncType,   // func(I32, Str) -> Bool — first-class function
        Ptr,        // *T or *mut T — raw pointer
        Unknown,    // error recovery
    } kind = Kind::Unknown;

    std::string name;           // for named types (Node2D, MyStruct, etc.)
    std::vector<Type> inner;   // array element, tuple elements, nullable inner, Result<T>
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
    // Result<T> — inner[0] = value type
    static Type makeResult(Type valueType) {
        Type t; t.kind = Kind::Result;
        t.inner.push_back(std::move(valueType));
        return t;
    }
    // HashMap<K, V> — inner[0] = key, inner[1] = value
    static Type makeHashMap(Type keyType, Type valueType) {
        Type t; t.kind = Kind::HashMap;
        t.inner.push_back(std::move(keyType));
        t.inner.push_back(std::move(valueType));
        return t;
    }
    static Type unknown() { return make(Kind::Unknown); }
    // FuncType: inner[0..n-2] = param types, inner[n-1] = return type
    static Type makeFuncType(std::vector<Type> paramTypes, Type retType) {
        Type t; t.kind = Kind::FuncType;
        t.inner = std::move(paramTypes);
        t.inner.push_back(std::move(retType));
        return t;
    }
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
    bool isResult()  const { return kind == Kind::Result; }
    Type resultInner() const { return inner.empty() ? unknown() : inner[0]; }

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
            case Kind::Result:
                return "Result<" + (inner.empty() ? "?" : inner[0].toString()) + ">";
            case Kind::Ptr:
                return inner.empty()?"*void":"*"+inner[0].toString();
            case Kind::FuncType: {
                std::string s = "func(";
                for (size_t i = 0; i + 1 < inner.size(); i++) {
                    if (i > 0) s += ", ";
                    s += inner[i].toString();
                }
                s += ") -> ";
                s += inner.empty() ? "void" : inner.back().toString();
                return s;
            }
            case Kind::HashMap:
                return "HashMap<" + (inner.size()<2 ? "?,?" :
                    inner[0].toString()+","+inner[1].toString()) + ">";
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

    // Pre-register top-level declarations from included .ks files
    // Call this for each included file BEFORE calling check()
    void addInclude(const Program& inc) {
        collectDeclarations(inc);
    }

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

        // Check all top-level function and node bodies
        for (auto& stmt : prog.stmts) {
            if (stmt->kind == Stmt::Kind::FuncDecl) {
                auto* f = static_cast<FuncDecl*>(stmt.get());
                m_checking.clear();
                checkFuncDecl(f, &global);
            } else if (stmt->kind == Stmt::Kind::NodeDecl) {
                auto* n = static_cast<NodeDecl*>(stmt.get());
                m_checking.clear();
                checkNodeDecl(n, &global);
            } else if (stmt->kind == Stmt::Kind::ClassDecl) {
                // Check implements compliance
                checkStmt(stmt.get(), &global, Type::void_());
                continue;
            } else if (stmt->kind == Stmt::Kind::Include) {
                continue; // includes are resolved at codegen time
            } else {
                m_checking.clear();
                checkStmt(stmt.get(), &global, Type::void_());
            }
        }
    }

private:
    std::vector<Error> m_errors;
    std::unordered_map<std::string, StructInfo> m_structs;
    std::unordered_map<std::string, EnumInfo>   m_enums;
    std::unordered_map<std::string, Symbol>     m_globalSymbols;

    // Known engine node types
    std::unordered_set<std::string> m_activeTypeParams;
    std::unordered_set<std::string> m_classTypeNames; // class names for method dispatch
    // interface name → set of required method names
    std::unordered_map<std::string, std::unordered_set<std::string>> m_interfaces;
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
            inner.typeParams = ta.typeParams;
            t = Type::makeArray(resolve(inner));
        } else if (ta.base == "Result") {
            // Result<T> — inner type is the success value type
            Type inner = ta.typeParams.empty() ? Type::unknown() : resolve(ta.typeParams[0]);
            t = Type::makeResult(inner);
        } else if (ta.isPtr) {
            t = Type::make(Type::Kind::Ptr);
            TypeAnnotation inner=ta; inner.isPtr=false; inner.isPtrMut=false;
            t.inner.push_back(resolve(inner));
        } else if (ta.isFuncType) {
            std::vector<Type> paramTs;
            for (auto& pt : ta.funcParamTypes) paramTs.push_back(resolve(pt));
            Type retT = ta.funcReturnType ? resolve(*ta.funcReturnType) : Type::void_();
            t = Type::makeFuncType(std::move(paramTs), retT);
        } else if (ta.base == "HashMap") {
            // HashMap<K, V>
            Type key = ta.typeParams.empty() ? Type::unknown() : resolve(ta.typeParams[0]);
            Type val = ta.typeParams.size() < 2 ? Type::unknown() : resolve(ta.typeParams[1]);
            t = Type::makeHashMap(key, val);
        } else {
            t = resolveBase(ta.base);
        }
        if (ta.nullable) t = Type::makeNullable(t);
        return t;
    }

    Type resolveBase(const std::string& name) {
        if (m_activeTypeParams.count(name)) return Type::unknown();
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
        if (name == "str" || name == "Str")    return Type::make(Type::Kind::Str);
        if (name == "String") return Type::make(Type::Kind::String);
        if (name == "Vec2")   return Type::make(Type::Kind::Vec2);
		if (name == "Color")  return Type::make(Type::Kind::Struct, "Color");
		if (name == "Scene")         return Type::make(Type::Kind::Struct, "Scene");
		// Engine resource types
		if (name == "Sound")         return Type::make(Type::Kind::Struct, "Sound");
		if (name == "Music")         return Type::make(Type::Kind::Struct, "Music");
		// Texture aliased to Texture2D above
		if (name == "Texture2D" || name == "Texture") return Type::make(Type::Kind::Struct, "Texture2D");
		if (name == "Font")          return Type::make(Type::Kind::Struct, "Font");
		if (name == "Shader")        return Type::make(Type::Kind::Struct, "Shader");
		if (name == "RenderTexture") return Type::make(Type::Kind::Struct, "RenderTexture");
		// Engine node component types
		if (name == "AnimationPlayer") return Type::make(Type::Kind::Struct, "AnimationPlayer");
		if (name == "Collider2D")    return Type::make(Type::Kind::Struct, "Collider2D");
		if (name == "Camera2D")      return Type::make(Type::Kind::Struct, "Camera2D"); // opaque engine struct
		if (name == "CameraNode2D")  return Type::make(Type::Kind::Node,   "CameraNode2D");
		if (name == "TileMap")       return Type::make(Type::Kind::Struct, "TileMap");
		// Input types
		if (name == "Key")           return Type::make(Type::Kind::Struct, "Key");
		if (name == "Mouse")         return Type::make(Type::Kind::Struct, "Mouse");
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

            } else if (stmt->kind == Stmt::Kind::ClassDecl) {
                auto* c = static_cast<ClassDecl*>(stmt.get());
                // Register fields in m_structs so StructInit syntax works
                StructInfo info; info.name = c->name;
                for (auto& f : c->fields)
                    info.fields.push_back({f.name, resolve(f.type)});
                m_structs[c->name] = std::move(info);
                m_classTypeNames.insert(c->name);
                // Register constructor so `let x: Foo = Foo { ... }` type-checks
                {
                    Symbol ctor;
                    ctor.name       = c->name;
                    ctor.isFunc     = true;
                    ctor.returnType = Type::make(Type::Kind::Class, c->name);
                    ctor.type       = ctor.returnType;
                    m_globalSymbols[c->name] = ctor;
                }
                // Register methods so member calls type-check
                for (auto& m : c->methods) {
                    Symbol sym;
                    sym.name = c->name + "::" + m->name;
                    sym.isFunc = true;
                    sym.returnType = m->returnType
                        ? resolve(*m->returnType) : Type::void_();
                    for (auto& p : m->params)
                        sym.paramTypes.push_back(resolve(p.type));
                    sym.type = sym.returnType;
                    m_globalSymbols[sym.name] = sym;
                }
            } else if (stmt->kind == Stmt::Kind::InterfaceDecl) {
                auto* id = static_cast<InterfaceDecl*>(stmt.get());
                std::unordered_set<std::string> mnames;
                for (auto& m : id->methods) mnames.insert(m.name);
                m_interfaces[id->name] = std::move(mnames);
            } else if (stmt->kind == Stmt::Kind::ExternDecl) {
                auto* e = static_cast<ExternDecl*>(stmt.get());
                Symbol sym; sym.name=e->name; sym.isFunc=true;
                sym.returnType=e->returnType.base.empty()?Type::void_():resolve(e->returnType);
                for(auto& p:e->params) sym.paramTypes.push_back(resolve(p.type));
                sym.type=sym.returnType;
                m_globalSymbols[e->name]=sym;
            } else if (stmt->kind == Stmt::Kind::FuncDecl) {
                auto* f = static_cast<FuncDecl*>(stmt.get());
                Symbol sym;
                sym.name    = f->name;
                sym.isFunc  = true;
                sym.pub     = f->pub;
                if (f->isCoroutine)
                    sym.returnType = Type::make(Type::Kind::Struct, "_KsTask");
                else if (!f->typeParams.empty())
                    sym.returnType = Type::unknown();
                else
                    sym.returnType = f->returnType
                        ? resolve(*f->returnType) : Type::void_();
                for (auto& p : f->params)
                    sym.paramTypes.push_back(
                        !f->typeParams.empty() ? Type::unknown() : resolve(p.type));
                sym.type = sym.returnType;
                m_globalSymbols[f->name] = sym;

            } else if (stmt->kind == Stmt::Kind::NodeDecl) {
                auto* n = static_cast<NodeDecl*>(stmt.get());
                // Register as a node type
                m_nodeTypes.insert(n->name);
                // Register as a constructor function so "let p = Player(...)" works
                {
                    Symbol ctor;
                    ctor.name       = n->name;
                    ctor.isFunc     = true;
                    ctor.returnType = Type::make(Type::Kind::Node, n->name);
                    ctor.type       = ctor.returnType;
                    m_globalSymbols[n->name] = ctor;
                }
                // Register methods
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
		reg("SetVsync",         {Bool}, Void);

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
        reg("GetScene",      {}, Type::make(Type::Kind::Struct, "Scene"));
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
        auto Sound  = Type::make(Type::Kind::Struct, "Sound");
        auto Music  = Type::make(Type::Kind::Struct, "Music");
        auto Tex    = Type::make(Type::Kind::Struct, "Texture2D");
        auto Font_t = Type::make(Type::Kind::Struct, "Font");
        reg("LoadSound",        {Str},       Sound);
        reg("PlaySound",        {Sound},     Void);
        reg("StopSound",        {Sound},     Void);
        reg("UnloadSound",      {Sound},     Void);
        reg("IsSoundPlaying",   {Sound},     Bool);
        reg("SetSoundVolume",   {Sound,F64}, Void);
        reg("LoadMusicStream",  {Str},       Music);
        reg("PlayMusicStream",  {Music},     Void);
        reg("StopMusicStream",  {Music},     Void);
        reg("UpdateMusicStream",{Music},     Void);
        // Textures
        reg("LoadTexture",      {Str},       Tex);
        reg("UnloadTexture",    {Tex},       Void);
        reg("DrawTexture",      {Tex,F64,F64}, Void);
        // Text
        reg("LoadFont",         {Str},       Font_t);
        reg("MeasureText",      {Str,I32},   I32);
        // Audio (old names kept for compat)
        reg("PlaySound",        {}, Void);
        reg("PlayMusic",     {}, Void);
        reg("StopMusic",     {}, Void);
        reg("PauseMusic",    {}, Void);
        reg("ResumeMusic",   {}, Void);
        reg("SetMusicVolume",{}, Void);

        // Engine namespace objects — registered as opaque struct instances
        // so member calls like AssetManager.init() type-check without errors
        auto regObj = [&](const std::string& name) {
            Symbol s;
            s.name = name;
            s.type = Type::make(Type::Kind::Struct, name);
            s.mut  = false;
            s.isFunc = false;
            m_globalSymbols[name] = s;
        };
        regObj("AssetManager");
        regObj("SceneManager");
        regObj("PhysicsWorld");
        regObj("InputManager");
        regObj("AudioManager");
        regObj("Renderer");

        // Scene type — let mut scene: Scene = Scene()
        auto SceneT = Type::make(Type::Kind::Struct, "Scene");
        {
            Symbol s;
            s.name       = "Scene";
            s.isFunc     = true;
            s.returnType = SceneT;
            s.type       = SceneT;
            m_globalSymbols["Scene"] = s;
        }
        // Register scene free functions so calls are validated
        reg("RunEngine", {SceneT}, Void);

        // RunEngine() — replaces manual game loop
        reg("RunEngine",     {}, Void);
        reg("LoadScene",     {Str}, SceneT);

        // Node builtins — available inside node scripts
        reg("GetNode",       {Str}, Type::make(Type::Kind::Node, "Node"));
        reg("GetParent",     {}, Type::make(Type::Kind::Node, "Node"));
        reg("Destroy",       {}, Void);

        // CameraNode2D — a node that acts as the scene camera
        {
            Symbol ctor;
            ctor.name       = "CameraNode2D";
            ctor.isFunc     = true;
            ctor.returnType = Type::make(Type::Kind::Struct, "CameraNode2D");
            ctor.type       = ctor.returnType;
            m_globalSymbols["CameraNode2D"] = ctor;
        }

        // Camera2D is a plain struct (x, y, zoom, rotation)
        {
            Symbol ctor;
            ctor.name       = "Camera2D";
            ctor.isFunc     = true;
            ctor.paramTypes = {F64, F64, F64, F64}; // x, y, zoom, rotation
            ctor.returnType = Type::make(Type::Kind::Struct, "Camera2D");
            ctor.type       = ctor.returnType;
            m_globalSymbols["Camera2D"] = ctor;
        }
        // BeginCamera2D takes a Camera2D struct
        reg("BeginCamera2D", {Type::make(Type::Kind::Struct, "Camera2D")}, Void);
        reg("EndCamera2D",   {}, Void);
        reg("GetWorldMouseX", {Type::make(Type::Kind::Struct, "Camera2D")}, F64);
        reg("GetWorldMouseY", {Type::make(Type::Kind::Struct, "Camera2D")}, F64);

        // ── File I/O ────────────────────────────────────────────────────
        // All file ops return Result<T> so errors are handled explicitly.
        // File.read(path)           -> Result<Str>
        // File.write(path, content) -> Result<Str>   (ok="" on success)
        // File.append(path,content) -> Result<Str>
        // File.exists(path)         -> Bool
        // File.delete(path)         -> Result<Str>
        // File.lines(path)          -> Result<[Str]>
        auto StrResult  = Type::makeResult(Str);
        auto StrArrRes  = Type::makeResult(Type::makeArray(Str));
        auto FileT      = Type::make(Type::Kind::Struct, "File");
        // Register File as a namespace object with method-style calls
        // e.g. File.read("x.txt") typechecks as a member call on File struct
        {
            Symbol s; s.name = "File";
            s.type   = FileT;
            s.mut    = false;
            s.isFunc = false;
            m_globalSymbols["File"] = s;
        }
        // Also register as free functions for direct call support
        reg("File_read",   {Str},      StrResult);
        reg("File_write",  {Str, Str}, StrResult);
        reg("File_append", {Str, Str}, StrResult);
        reg("File_exists", {Str},      Bool);
        reg("File_delete", {Str},      StrResult);
        reg("File_lines",  {Str},      StrArrRes);

        // ── String methods ──────────────────────────────────────────────
        // Registered as free functions: Str_split, Str_trim etc.
        // Member calls (str.split(",")) are handled in checkMemberCall.
        reg("Str_len",      {Str},      I32);
        reg("Str_split",    {Str, Str}, Type::makeArray(Str));
        reg("Str_trim",     {Str},      Str);
        reg("Str_contains", {Str, Str}, Bool);
        reg("Str_replace",  {Str, Str, Str}, Str);
        reg("Str_starts",   {Str, Str}, Bool);
        reg("Str_ends",     {Str, Str}, Bool);
        reg("Str_upper",    {Str},      Str);
        reg("Str_lower",    {Str},      Str);
        reg("Str_substr",   {Str, I32, I32}, Str);
        reg("Str_toInt",    {Str},      I32);
        reg("Str_toFloat",  {Str},      F64);

        // ── HashMap ─────────────────────────────────────────────────────
        // HashMap<K,V> constructor — variadic, skip arg check
        reg("HashMap",     {}, Type::make(Type::Kind::Struct, "HashMap"));
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

            case Stmt::Kind::AsmStmt:
            case Stmt::Kind::ExternDecl:
            case Stmt::Kind::InterfaceDecl: break; // validated at collect time
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
                break; // nothing to check here
            case Stmt::Kind::ClassDecl: {
                auto* c = static_cast<const ClassDecl*>(s);
                // Check implements compliance
                for (auto& iface : c->implements) {
                    if (!m_interfaces.count(iface)) {
                        error("unknown interface '" + iface + "'", s->line, s->col);
                        continue;
                    }
                    std::unordered_set<std::string> provided;
                    for (auto& m : c->methods) provided.insert(m->name);
                    for (auto& req : m_interfaces.at(iface)) {
                        if (!provided.count(req))
                            error("class '" + c->name + "' missing '" + req + "' from interface '" + iface + "'" , c->line, c->col);
                    }
                }
                break;
            }
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
        for (auto& tp : f->typeParams) {
            Symbol s; s.name = tp; s.type = Type::unknown(); s.mut = false;
            funcScope.define(s);
            m_activeTypeParams.insert(tp);
        }
        for (auto& p : f->params) {
            Symbol sym;
            sym.name = p.name;
            sym.type = resolve(p.type);
            sym.mut  = true; // params are always mutable inside the body
            funcScope.define(sym);
        }
        Type ret = f->returnType ? resolve(*f->returnType) : Type::void_();
        checkBlock(f->body.get(), &funcScope, ret);
        for (auto& tp : f->typeParams) m_activeTypeParams.erase(tp);
        m_checking.erase(f->name);
    }

    void checkNodeDecl(const NodeDecl* n, Scope* scope) {
        // Extra fields for CameraNode2D base
        if (n->base == "CameraNode2D") {
            Symbol zoomSym; zoomSym.name = "zoom";     zoomSym.type = Type::make(Type::Kind::F64);  zoomSym.mut = true; scope->define(zoomSym);
            Symbol rotSym;  rotSym.name  = "rotation"; rotSym.type  = Type::make(Type::Kind::F64);  rotSym.mut = true; scope->define(rotSym);
            Symbol curSym;  curSym.name  = "current";  curSym.type  = Type::make(Type::Kind::Bool); curSym.mut = true; scope->define(curSym);
        }
        // Catch: node Camera2D : Camera2D — name same as base type
        if (n->name == n->base)
            error("node '" + n->name + "' cannot have the same name as its base type '" +
                  n->base + "'. Use a different name (e.g. 'MyCam : Camera2D').", 0, 0);
        // Build a scope with the node's fields and inherited Node2D properties
        Scope nodeScope; nodeScope.parent = scope;

        auto F64  = Type::make(Type::Kind::F64);
        auto Bool = Type::make(Type::Kind::Bool);
        auto NodeT = Type::make(Type::Kind::Node, n->name);
        auto Unk   = Type::unknown();

        // 'this' — refers to the node itself
        { Symbol s; s.name = "this"; s.type = NodeT; s.mut = false;
          nodeScope.define(s); }

        // Built-in Node2D fields available directly inside node body
        for (auto& nm : {"x","y","scaleX","scaleY","rotation",
                         "originX","originY","alpha",
                         "width","height","z"}) {
            Symbol s; s.name = nm; s.type = F64; s.mut = true;
            nodeScope.define(s);
        }
        { Symbol s; s.name = "active";  s.type = Bool; s.mut = true;  nodeScope.define(s); }
        { Symbol s; s.name = "visible"; s.type = Bool; s.mut = true;  nodeScope.define(s); }
        { Symbol s; s.name = "name";    s.type = Type::make(Type::Kind::Str); s.mut = false; nodeScope.define(s); }

        // Base-type specific fields
        auto I32T = Type::make(Type::Kind::I32);
        if (n->base == "Collider2D" || n->base == "CollisionShape2D") {
            for (auto& nm : {"debugDraw","solid","staticBody","touching"}) {
                Symbol s; s.name = nm; s.type = Bool; s.mut = true; nodeScope.define(s);
            }
            for (auto& nm : {"radius"}) {
                Symbol s; s.name = nm; s.type = F64; s.mut = true; nodeScope.define(s);
            }
            for (auto& nm : {"layer","mask"}) {
                Symbol s; s.name = nm; s.type = I32T; s.mut = true; nodeScope.define(s);
            }
        }
        if (n->base == "Sprite2D" || n->base == "AnimatedSprite2D") {
            for (auto& nm : {"flipH","flipV"}) {
                Symbol s; s.name = nm; s.type = Bool; s.mut = true; nodeScope.define(s);
            }
        }
        if (n->base == "CameraNode2D" || n->base == "Camera2D") {
            for (auto& nm : {"smoothing","current"}) {
                Symbol s; s.name = nm; s.type = Bool; s.mut = true; nodeScope.define(s);
            }
            for (auto& nm : {"zoom","smoothSpeed"}) {
                Symbol s; s.name = nm; s.type = F64; s.mut = true; nodeScope.define(s);
            }
        }

        // this.add(Type, "name") — returns unknown, skip type-check
        // Register engine node component types as constructors in node scope
        for (auto& nm : {"AnimationPlayer","Collider2D","Sprite2D",
                         "Camera2D","TileMap","AudioPlayer","Timer"}) {
            Symbol s; s.name = nm; s.isFunc = true;
            s.returnType = Type::make(Type::Kind::Struct, nm);
            s.type       = s.returnType;
            nodeScope.define(s);
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
                // Safe member on non-nullable is an error — operand is never null
                if (e->kind == Expr::Kind::SafeMember &&
                    obj.kind != Type::Kind::Nullable && !obj.isUnknown())
                    error("safe access '?.' used on non-nullable type '" +
                          obj.toString() + "' — operand is never null", e->line, e->col);

                // ── Result<T> members ──────────────────────────────────
                // result.ok    -> Bool
                // result.value -> T
                // result.error -> Str
                if (obj.kind == Type::Kind::Result) {
                    if (m->member == "ok")    return Type::make(Type::Kind::Bool);
                    if (m->member == "value") return obj.inner.empty() ? Type::unknown() : obj.inner[0];
                    if (m->member == "error") return Type::make(Type::Kind::Str);
                    error("Result has no member '" + m->member + "' (use .ok, .value, .error)",
                          e->line, e->col);
                    return Type::unknown();
                }

                // ── HashMap<K,V> members ───────────────────────────────
                // map.get(key) -> V?    map.set(key, val)  map.has(key) -> Bool
                // map.remove(key)       map.len() -> I32   map.keys() -> [K]
                if (obj.kind == Type::Kind::HashMap) {
                    Type K = obj.inner.size() > 0 ? obj.inner[0] : Type::unknown();
                    Type V = obj.inner.size() > 1 ? obj.inner[1] : Type::unknown();
                    if (m->member == "get")    return Type::makeNullable(V);
                    if (m->member == "set")    return Type::void_();
                    if (m->member == "has")    return Type::make(Type::Kind::Bool);
                    if (m->member == "remove") return Type::void_();
                    if (m->member == "len")    return Type::make(Type::Kind::I32);
                    if (m->member == "keys")   return Type::makeArray(K);
                    if (m->member == "values") return Type::makeArray(V);
                    if (m->member == "clear")  return Type::void_();
                    return Type::unknown();
                }

                // ── File namespace member calls ────────────────────────
                // File.read(path) -> Result<Str>  etc.
                if (obj.kind == Type::Kind::Struct && obj.name == "File") {
                    auto Str2   = Type::make(Type::Kind::Str);
                    auto Bool2  = Type::make(Type::Kind::Bool);
                    if (m->member == "read")   return Type::makeResult(Str2);
                    if (m->member == "write")  return Type::makeResult(Str2);
                    if (m->member == "append") return Type::makeResult(Str2);
                    if (m->member == "exists") return Bool2;
                    if (m->member == "delete") return Type::makeResult(Str2);
                    if (m->member == "lines")  return Type::makeResult(Type::makeArray(Str2));
                    error("File has no member '" + m->member + "'", e->line, e->col);
                    return Type::unknown();
                }

                // ── Str/String method calls ────────────────────────────
                // "hello".len() -> I32  etc.
                if (obj.kind == Type::Kind::Str || obj.kind == Type::Kind::String) {
                    auto Str2  = Type::make(Type::Kind::Str);
                    auto Bool2 = Type::make(Type::Kind::Bool);
                    auto I32T  = Type::make(Type::Kind::I32);
                    auto F64T  = Type::make(Type::Kind::F64);
                    if (m->member == "len")      return I32T;
                    if (m->member == "split")    return Type::makeArray(Str2);
                    if (m->member == "trim")     return Str2;
                    if (m->member == "contains") return Bool2;
                    if (m->member == "replace")  return Str2;
                    if (m->member == "starts")   return Bool2;
                    if (m->member == "ends")     return Bool2;
                    if (m->member == "upper")    return Str2;
                    if (m->member == "lower")    return Str2;
                    if (m->member == "substr")   return Str2;
                    if (m->member == "toInt")    return I32T;
                    if (m->member == "toFloat")  return F64T;
                    if (m->member == "isEmpty")  return Bool2;
                    // Unknown string method — let through
                }

                // ── Array members ──────────────────────────────────────
                if (obj.kind == Type::Kind::Array) {
                    Type elem = obj.inner.empty() ? Type::unknown() : obj.inner[0];
                    if (m->member == "len")    return Type::make(Type::Kind::I32);
                    if (m->member == "push")   return Type::void_();
                    if (m->member == "pop")    return elem;
                    if (m->member == "first")  return Type::makeNullable(elem);
                    if (m->member == "last")   return Type::makeNullable(elem);
                    if (m->member == "isEmpty")return Type::make(Type::Kind::Bool);
                    if (m->member == "clear")  return Type::void_();
                    if (m->member == "has")    return Type::make(Type::Kind::Bool);
                }

                // Struct field access
                if (obj.kind == Type::Kind::Struct) {
                    auto it = m_structs.find(obj.name);
                    if (it != m_structs.end()) {
                        for (auto& [fname, ftype] : it->second.fields)
                            if (fname == m->member) return ftype;
                        // For class types (have methods), suppress missing field errors
                        if (!it->second.fields.empty() && !m_classTypeNames.count(obj.name))
                            error("struct '" + obj.name + "' has no field '" +
                                  m->member + "'", e->line, e->col);
                    }
                    // Unknown struct (opaque engine object) — let it through
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
                // Check all required fields are provided
                for (auto& [fname, ftype] : it->second.fields) {
                    bool found = false;
                    for (auto& f : si->fields) {
                        if (f.name == fname) { found = true; break; }
                    }
                    if (!found && ftype.kind != Type::Kind::Nullable)
                        error("missing field '" + fname + "' in struct '" +
                              si->typeName + "' initializer", e->line, e->col);
                }
                return Type::make(Type::Kind::Struct, si->typeName);
            }

            case Expr::Kind::Range:
                checkExpr(static_cast<const RangeExpr*>(e)->from.get(), scope);
                checkExpr(static_cast<const RangeExpr*>(e)->to.get(), scope);
                return Type::make(Type::Kind::Struct, "Range");

            case Expr::Kind::Spawn:
                checkExpr(static_cast<const SpawnExpr*>(e)->call.get(), scope);
                return Type::make(Type::Kind::Struct, "_KsTask");
            case Expr::Kind::Deref: {
                auto pt=checkExpr(static_cast<const DerefExpr*>(e)->ptr.get(),scope);
                return (pt.kind==Type::Kind::Ptr&&!pt.inner.empty())?pt.inner[0]:Type::unknown();
            }
            case Expr::Kind::AddrOf: {
                auto vt=checkExpr(static_cast<const AddrOfExpr*>(e)->value.get(),scope);
                Type pt=Type::make(Type::Kind::Ptr); pt.inner.push_back(vt); return pt;
            }

            case Expr::Kind::FuncExpr: {
                // Closure: infer param types and return type
                auto* fe = static_cast<const FuncExpr*>(e);
                Scope cls; cls.parent = scope;
                std::vector<Type> paramTypes;
                for (auto& p : fe->params) {
                    Type pt = resolve(p.type);
                    paramTypes.push_back(pt);
                    Symbol ps; ps.name = p.name; ps.type = pt; ps.mut = true;
                    cls.define(ps);
                }
                Type retT = fe->returnType ? resolve(*fe->returnType) : Type::void_();
                checkBlock(fe->body.get(), &cls, retT);
                return Type::makeFuncType(std::move(paramTypes), retT);
            }

            case Expr::Kind::Propagate: {
                // expr? — must be Result<T>, returns T
                auto* pe = static_cast<const PropagateExpr*>(e);
                Type t = checkExpr(pe->value.get(), scope);
                if (!t.isUnknown() && t.kind != Type::Kind::Result)
                    error("'?' operator requires Result<T>, got '" + t.toString() + "'",
                          e->line, e->col);
                return t.isResult() ? t.resultInner() : Type::unknown();
            }

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

        // Array covariance: [?] (empty literal []) is compatible with any [T]
        if (expected.kind == Type::Kind::Array && actual.kind == Type::Kind::Array) {
            if (actual.inner.empty() || actual.inner[0].isUnknown()) return true;
            if (expected.inner.empty() || expected.inner[0].isUnknown()) return true;
            return typesCompatible(expected.inner[0], actual.inner[0]);
        }

        // Nullable: T? is compatible with T? (if inner types match), T, and null
        if (expected.kind == Type::Kind::Nullable) {
            if (actual.kind == Type::Kind::Nullable) {
                // Both nullable: check inner types match
                if (expected.inner.empty() || actual.inner.empty()) return true;
                return typesCompatible(expected.inner[0], actual.inner[0]);
            }
            // Non-nullable T assigned to T? — allowed (implicit wrap)
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

        // HashMap<K,V> is compatible with bare HashMap struct (from constructor)
        if (expected.kind == Type::Kind::HashMap &&
            actual.kind == Type::Kind::Struct &&
            actual.name == "HashMap") return true;

        // Result<T> is compatible with bare Result struct
        if (expected.kind == Type::Kind::Result &&
            actual.kind == Type::Kind::Struct &&
            actual.name == "Result") return true;

        // Two HashMaps are compatible if key and value types match
        if (expected.kind == Type::Kind::HashMap &&
            actual.kind == Type::Kind::HashMap) {
            if (expected.inner.size() < 2 || actual.inner.size() < 2) return true;
            return typesCompatible(expected.inner[0], actual.inner[0]) &&
                   typesCompatible(expected.inner[1], actual.inner[1]);
        }

        // Two Results are compatible if inner types match
        if (expected.kind == Type::Kind::Result &&
            actual.kind == Type::Kind::Result) {
            if (expected.inner.empty() || actual.inner.empty()) return true;
            return typesCompatible(expected.inner[0], actual.inner[0]);
        }

        // Function types: compatible if param types and return type match
        if (expected.kind == Type::Kind::FuncType &&
            actual.kind == Type::Kind::FuncType) {
            if (expected.inner.size() != actual.inner.size()) return false;
            for (size_t i = 0; i < expected.inner.size(); i++) {
                if (!typesCompatible(expected.inner[i], actual.inner[i])) return false;
            }
            return true;
        }

        // Pointer types: ptr ↔ ptr (with compatible inner types), ptr ↔ integer for casts
        if (expected.kind == Type::Kind::Ptr && actual.kind == Type::Kind::Ptr) {
            // *void is compatible with any pointer
            if (expected.inner.empty() || actual.inner.empty()) return true;
            return typesCompatible(expected.inner[0], actual.inner[0]);
        }
        // ptr ↔ integer for low-level casts (needed for systems programming)
        if (expected.kind == Type::Kind::Ptr && actual.isNumeric()) return true;
        if (actual.kind == Type::Kind::Ptr && expected.isNumeric()) return true;

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
