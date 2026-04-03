// ks_codegen.ks — C++ transpiler for KonScript self-hosted compiler
// Generates C++ source from the AST, compatible with KonEngine.
// This enables engine game compilation via: konscript --cpp game.ks

// ── Output buffer ────────────────────────────────────────────────────────
let mut cg_lines: [Str] = [""];
let mut cg_indent: I32 = 0;

// ── State tracking ───────────────────────────────────────────────────────
let mut cg_ptr_vars: [Str] = [""];      // variables that are pointers
let mut cg_ptr_count: I32 = 0;
let mut cg_is_engine: Bool = false;     // true if #include <engine> found

func cg_reset() {
    cg_lines.clear(); cg_lines.push("");
    cg_indent = 0;
    cg_ptr_vars.clear(); cg_ptr_vars.push("");
    cg_ptr_count = 0;
    cg_is_engine = false;
}

func cg_emit(line: Str) {
    let mut pad: Str = "";
    let mut i: I32 = 0;
    while i < cg_indent { pad = pad + "    "; i = i + 1; }
    cg_lines.push(pad + line);
}

func cg_emit_raw(line: Str) {
    cg_lines.push(line);
}

func cg_indent_inc() { cg_indent = cg_indent + 1; }
func cg_indent_dec() { if cg_indent > 0 { cg_indent = cg_indent - 1; } }

func cg_is_ptr(name: Str) -> Bool {
    let mut i: I32 = 1;
    while i <= cg_ptr_count {
        if cg_ptr_vars[i] == name { return true; }
        i = i + 1;
    }
    return false;
}

func cg_mark_ptr(name: Str) {
    cg_ptr_count = cg_ptr_count + 1;
    cg_ptr_vars.push(name);
}

// ── Type mapping ─────────────────────────────────────────────────────────
func cg_type(t: Str) -> Str {
    if t == "I8"   { return "int8_t"; }
    if t == "I16"  { return "int16_t"; }
    if t == "I32"  { return "int32_t"; }
    if t == "I64"  { return "int64_t"; }
    if t == "U8"   { return "uint8_t"; }
    if t == "U16"  { return "uint16_t"; }
    if t == "U32"  { return "uint32_t"; }
    if t == "U64"  { return "uint64_t"; }
    if t == "F32"  { return "float"; }
    if t == "F64"  { return "double"; }
    if t == "Bool" { return "bool"; }
    if t == "Str"  { return "std::string"; }
    if t == "void" { return "void"; }
    // Array type [T] -> std::vector<T>
    if t.len() > 2 && t.starts("[") {
        let inner: Str = t.substr(1, t.len() - 2);
        return "std::vector<" + cg_type(inner) + ">";
    }
    // Nullable T? -> std::optional<T>
    if t.ends("?") {
        let inner: Str = t.substr(0, t.len() - 1);
        return "std::optional<" + cg_type(inner) + ">";
    }
    return t;
}

// ── Engine node types (always pointers) ──────────────────────────────────
func cg_is_node_type(t: Str) -> Bool {
    if t == "Node2D" || t == "Sprite2D" || t == "Collider2D" { return true; }
    if t == "AnimatedSprite2D" || t == "AnimationPlayer" { return true; }
    if t == "CameraNode2D" || t == "Camera2D" { return true; }
    return false;
}

// ── Expression codegen ───────────────────────────────────────────────────
func cg_gen_expr(idx: I32) -> Str {
    if idx == 0 { return ""; }
    let k: I32 = node_kinds[idx];

    if k == NK_INT   { return node_str[idx]; }
    if k == NK_FLOAT { return node_str[idx] + "f"; }
    if k == NK_BOOL  {
        if node_str[idx] == "true" { return "true"; }
        return "false";
    }
    if k == NK_STR_LIT {
        return "std::string(\"" + node_str[idx] + "\")";
    }
    if k == NK_NULL_LIT { return "nullptr"; }

    if k == NK_FSTR {
        // F-strings: build with snprintf
        // For simplicity, emit a _ks_fstr helper call
        return "\"" + node_str[idx] + "\"";
    }

    if k == NK_IDENT {
        return node_str[idx];
    }

    if k == NK_BINARY {
        let left: Str = cg_gen_expr(node_a[idx]);
        let right: Str = cg_gen_expr(node_b[idx]);
        let op: Str = node_str[idx];
        return "(" + left + " " + op + " " + right + ")";
    }

    if k == NK_UNARY {
        let operand: Str = cg_gen_expr(node_a[idx]);
        return "(" + node_str[idx] + operand + ")";
    }

    if k == NK_CALL {
        let callee: I32 = node_a[idx];
        // Build args
        let mut args: Str = "";
        let mut arg: I32 = node_b[idx];
        let mut first: Bool = true;
        while arg != 0 {
            let aexpr: Str = cg_gen_expr(node_a[arg]);
            if !first { args = args + ", "; }
            args = args + aexpr;
            first = false;
            arg = node_b[arg];
        }
        if node_kinds[callee] == NK_IDENT {
            let fname: Str = node_str[callee];
            // Print → std::cout
            if fname == "Print" {
                let mut cout_expr: Str = "std::cout";
                let mut parg: I32 = node_b[idx];
                while parg != 0 {
                    let pexpr: Str = cg_gen_expr(node_a[parg]);
                    cout_expr = cout_expr + " << " + pexpr;
                    parg = node_b[parg];
                }
                return cout_expr + " << std::endl";
            }
            // ToString
            if fname == "ToString" { return "std::to_string(" + args + ")"; }
            // Math
            if fname == "Sqrt" { return "sqrtf(" + args + ")"; }
            if fname == "Abs"  { return "fabsf(" + args + ")"; }
            // Engine API mappings
            if fname == "KeyDown"     { return "IsKeyDown(" + args + ")"; }
            if fname == "KeyPressed"  { return "IsKeyPressed(" + args + ")"; }
            if fname == "KeyReleased" { return "IsKeyReleased(" + args + ")"; }
            if fname == "MouseDown"    { return "IsMouseButtonDown(" + args + ")"; }
            if fname == "MousePressed" { return "IsMouseButtonPressed(" + args + ")"; }
            return fname + "(" + args + ")";
        }
        if node_kinds[callee] == NK_MEMBER {
            let obj: Str = cg_gen_expr(node_a[callee]);
            let method: Str = node_str[callee];
            let accessor: Str = ".";
            // Collection methods
            if method == "len"     { return obj + ".size()"; }
            if method == "isEmpty" { return obj + ".empty()"; }
            if method == "push"    { return obj + ".push_back(" + args + ")"; }
            if method == "pop"     { return "([&](){ auto _v = " + obj + ".back(); " + obj + ".pop_back(); return _v; }())"; }
            if method == "clear"   { return obj + ".clear()"; }
            // String methods
            if method == "trim"     { return "_ks_str_trim(" + obj + ")"; }
            if method == "upper"    { return "_ks_str_upper(" + obj + ")"; }
            if method == "lower"    { return "_ks_str_lower(" + obj + ")"; }
            if method == "contains" { return "_ks_str_contains(" + obj + ", " + args + ")"; }
            if method == "starts"   { return "_ks_str_starts(" + obj + ", " + args + ")"; }
            if method == "ends"     { return "_ks_str_ends(" + obj + ", " + args + ")"; }
            if method == "replace"  { return "_ks_str_replace(" + obj + ", " + args + ")"; }
            if method == "substr"   { return "_ks_str_substr(" + obj + ", " + args + ")"; }
            if method == "split"    { return "_ks_str_split(" + obj + ", " + args + ")"; }
            if method == "toInt"    { return "_ks_str_toInt(" + obj + ")"; }
            if method == "toFloat"  { return "_ks_str_toFloat(" + obj + ")"; }
            if method == "charAt"   { return "_ks_str_charAt(" + obj + ", " + args + ")"; }
            // HashMap methods
            if method == "set"    { return obj + "[" + args + "]"; }
            if method == "get"    { return obj + ".count(" + args + ") ? std::make_optional(" + obj + "[" + args + "]) : std::nullopt"; }
            if method == "has"    { return obj + ".count(" + args + ") > 0"; }
            if method == "remove" { return obj + ".erase(" + args + ")"; }
            // Scene methods (engine)
            if method == "update" { return obj + ".Update(" + args + ")"; }
            if method == "draw"   { return obj + ".Draw()"; }
            if method == "scan"   { return obj + ".Scan()"; }
            // File methods
            if method == "read"   { return "_ks_file_read(" + args + ")"; }
            if method == "write"  { return "_ks_file_write(" + args + ")"; }
            if method == "exists" { return "_ks_file_exists(" + args + ")"; }
            if method == "delete" { return "_ks_file_delete(" + args + ")"; }
            if method == "append" { return "_ks_file_append(" + args + ")"; }
            // Scene.add<T>("name")
            if method == "add" {
                return obj + ".Add<" + args + ">()";
            }
            // Default: method call
            return obj + accessor + method + "(" + args + ")";
        }
        return "/* unknown call */";
    }

    if k == NK_MEMBER {
        let obj: Str = cg_gen_expr(node_a[idx]);
        let member: Str = node_str[idx];
        // Result members
        if member == "ok"    { return obj + ".ok"; }
        if member == "value" { return obj + ".value"; }
        if member == "error" { return obj + ".error"; }
        return obj + "." + member;
    }

    if k == NK_INDEX {
        let obj: Str = cg_gen_expr(node_a[idx]);
        let index: Str = cg_gen_expr(node_b[idx]);
        return obj + "[" + index + "]";
    }

    if k == NK_CAST {
        let val: Str = cg_gen_expr(node_a[idx]);
        let target: Str = cg_type(node_str[idx]);
        return "static_cast<" + target + ">(" + val + ")";
    }

    if k == NK_ARRAY_LIT {
        let mut elems: Str = "";
        let mut el: I32 = node_a[idx];
        let mut first: Bool = true;
        while el != 0 {
            let eexpr: Str = cg_gen_expr(node_a[el]);
            if !first { elems = elems + ", "; }
            elems = elems + eexpr;
            first = false;
            el = node_b[el];
        }
        return "{" + elems + "}";
    }

    if k == NK_ASSIGN {
        let target: Str = cg_gen_expr(node_a[idx]);
        let value: Str = cg_gen_expr(node_b[idx]);
        let op: Str = node_str[idx];
        return target + " " + op + " " + value;
    }

    if k == NK_FUNC_EXPR {
        // Lambda: [&](params) -> RetType { body }
        let ret_str: Str = node_str[idx];
        let ret_t: Str = ret_str.substr(1, ret_str.len() - 1);
        let mut params: Str = "";
        let mut pm: I32 = node_a[idx];
        let mut first: Bool = true;
        while pm != 0 {
            let pn: I32 = node_a[pm];
            let ps: Str = node_str[pn];
            // Parse "name:type"
            let mut ci: I32 = 0;
            let mut pname: Str = "";
            let mut ptype: Str = "";
            while ci < ps.len() {
                if ps.substr(ci, 1) == ":" {
                    pname = ps.substr(0, ci);
                    ptype = ps.substr(ci + 1, ps.len() - ci - 1);
                    ci = ps.len();
                }
                ci = ci + 1;
            }
            if !first { params = params + ", "; }
            params = params + cg_type(ptype) + " " + pname;
            first = false;
            pm = node_b[pm];
        }
        return "[&](" + params + ") -> " + cg_type(ret_t) + " { /* body */ }";
    }

    return "/* expr " + node_str[idx] + " */";
}

// ── Statement codegen ────────────────────────────────────────────────────
func cg_gen_stmt(idx: I32) {
    if idx == 0 { return; }
    let k: I32 = node_kinds[idx];

    if k == NK_LET {
        let name: Str = node_str[idx];
        let init: I32 = node_a[idx];
        if init != 0 {
            let val: Str = cg_gen_expr(init);
            cg_emit("auto " + name + " = " + val + ";");
        } else {
            cg_emit("auto " + name + " = 0;");
        }
        return;
    }

    if k == NK_CONST_D {
        let name: Str = node_str[idx];
        let init: I32 = node_a[idx];
        let val: Str = cg_gen_expr(init);
        cg_emit("constexpr auto " + name + " = " + val + ";");
        return;
    }

    if k == NK_RETURN {
        if node_a[idx] != 0 {
            let val: Str = cg_gen_expr(node_a[idx]);
            cg_emit("return " + val + ";");
        } else {
            cg_emit("return;");
        }
        return;
    }

    if k == NK_IF {
        let cond: Str = cg_gen_expr(node_a[idx]);
        cg_emit("if (" + cond + ") {");
        cg_indent_inc();
        cg_gen_stmt(node_b[idx]);
        cg_indent_dec();
        if node_c[idx] != 0 {
            cg_emit("} else {");
            cg_indent_inc();
            cg_gen_stmt(node_c[idx]);
            cg_indent_dec();
        }
        cg_emit("}");
        return;
    }

    if k == NK_WHILE {
        let cond: Str = cg_gen_expr(node_a[idx]);
        cg_emit("while (" + cond + ") {");
        cg_indent_inc();
        cg_gen_stmt(node_b[idx]);
        cg_indent_dec();
        cg_emit("}");
        return;
    }

    if k == NK_LOOP {
        cg_emit("while (true) {");
        cg_indent_inc();
        cg_gen_stmt(node_a[idx]);
        cg_indent_dec();
        cg_emit("}");
        return;
    }

    if k == NK_FOR_IN {
        let var_name: Str = node_str[idx];
        let iter: Str = cg_gen_expr(node_a[idx]);
        cg_emit("for (auto& " + var_name + " : " + iter + ") {");
        cg_indent_inc();
        cg_gen_stmt(node_b[idx]);
        cg_indent_dec();
        cg_emit("}");
        return;
    }

    if k == NK_BREAK    { cg_emit("break;"); return; }
    if k == NK_CONTINUE { cg_emit("continue;"); return; }

    if k == NK_BLOCK {
        let mut s: I32 = node_a[idx];
        while s != 0 {
            cg_gen_stmt(node_a[s]);
            s = node_b[s];
        }
        return;
    }

    // Expression statement
    let expr: Str = cg_gen_expr(idx);
    if expr.len() > 0 {
        cg_emit(expr + ";");
    }
}

// ── Function codegen ─────────────────────────────────────────────────────
func cg_gen_func(idx: I32) {
    let full_name: Str = node_str[idx];
    let name: Str = func_name(full_name);
    let ret: Str = func_ret(full_name);
    let ret_cpp: Str = cg_type(ret);

    // Build params
    let mut params: Str = "";
    let mut pm: I32 = node_a[idx];
    let mut first: Bool = true;
    while pm != 0 {
        let pn: I32 = node_a[pm];
        let ps: Str = node_str[pn];
        let mut ci: I32 = 0;
        let mut pname: Str = "";
        let mut ptype: Str = "";
        let mut found: Bool = false;
        while ci < ps.len() {
            let ch: Str = ps.substr(ci, 1);
            if ch == ":" && !found {
                found = true;
                pname = ps.substr(0, ci);
                ptype = ps.substr(ci + 1, ps.len() - ci - 1);
            }
            ci = ci + 1;
        }
        if !first { params = params + ", "; }
        params = params + cg_type(ptype) + " " + pname;
        first = false;
        pm = node_b[pm];
    }

    cg_emit(ret_cpp + " " + name + "(" + params + ") {");
    cg_indent_inc();
    cg_gen_stmt(node_b[idx]);
    cg_indent_dec();
    cg_emit("}");
    cg_emit("");
}

// ── Struct codegen ───────────────────────────────────────────────────────
func cg_gen_struct(idx: I32) {
    let name: Str = node_str[idx];
    cg_emit("struct " + name + " {");
    cg_indent_inc();
    let mut f: I32 = node_a[idx];
    while f != 0 {
        let fn2: I32 = node_a[f];
        let fs: Str = node_str[fn2];
        // Parse "name:type"
        let mut ci: I32 = 0;
        let mut fname: Str = "";
        let mut ftype: Str = "";
        while ci < fs.len() {
            if fs.substr(ci, 1) == ":" {
                fname = fs.substr(0, ci);
                ftype = fs.substr(ci + 1, fs.len() - ci - 1);
                ci = fs.len();
            }
            ci = ci + 1;
        }
        cg_emit(cg_type(ftype) + " " + fname + ";");
        f = node_b[f];
    }
    cg_indent_dec();
    cg_emit("};");
    cg_emit("");
}

// ── Node codegen (engine) ────────────────────────────────────────────────
func cg_gen_node(idx: I32) {
    // Nodes are parsed as NK_FUNC with node-specific handling
    // For now, emit as a class inheriting from Node2D
    // This would need the parser to produce NK_NODE nodes
    cg_emit("// node codegen placeholder");
}

// ── Main codegen entry point ─────────────────────────────────────────────
func cg_generate(prog_idx: I32) -> Str {
    cg_reset();

    // Headers
    cg_emit_raw("#include <iostream>");
    cg_emit_raw("#include <string>");
    cg_emit_raw("#include <vector>");
    cg_emit_raw("#include <optional>");
    cg_emit_raw("#include <unordered_map>");
    cg_emit_raw("#include <cmath>");
    cg_emit_raw("#include <cstdio>");
    cg_emit_raw("#include <fstream>");
    cg_emit_raw("#include <functional>");
    if cg_is_engine {
        cg_emit_raw("#include \"KonEngine.hpp\"");
    }
    cg_emit_raw("");

    // Stdlib helpers
    cg_emit_raw("// KonScript runtime helpers");
    cg_emit_raw("#include <cstring>");
    cg_emit_raw("");

    // Emit top-level declarations
    let mut decl: I32 = node_a[prog_idx];
    while decl != 0 {
        let d: I32 = node_a[decl];
        if node_kinds[d] == NK_FUNC {
            cg_gen_func(d);
        }
        if node_kinds[d] == NK_STRUCT_D {
            cg_gen_struct(d);
        }
        if node_kinds[d] == NK_CONST_D {
            let val: Str = cg_gen_expr(node_a[d]);
            cg_emit("constexpr auto " + node_str[d] + " = " + val + ";");
        }
        if node_kinds[d] == NK_LET {
            let val: Str = cg_gen_expr(node_a[d]);
            cg_emit("auto " + node_str[d] + " = " + val + ";");
        }
        if node_kinds[d] == NK_INCLUDE_D {
            let path: Str = node_str[d];
            if path == "engine" {
                cg_is_engine = true;
            }
        }
        decl = node_b[decl];
    }

    // Build output string line by line
    let mut out: Str = "";
    let mut i: I32 = 1;
    while i < cg_lines.len() {
        out = out + cg_lines[i] + "\n";
        i = i + 1;
    }
    return out;
}

// ── Write C++ output to file ─────────────────────────────────────────────
func cg_write_to_file(path: Str) -> Bool {
    let first_result: Result<Str> = File.write(path, "");
    if !first_result.ok { return false; }
    let mut i: I32 = 1;
    while i < cg_lines.len() {
        let line: Str = cg_lines[i] + "\n";
        let r: Result<Str> = File.append(path, line);
        if !r.ok { return false; }
        i = i + 1;
    }
    return true;
}
