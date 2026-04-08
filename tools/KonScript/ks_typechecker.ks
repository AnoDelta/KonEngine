// ks_typechecker.ks — Typechecker with ownership, borrow checking, and strict mode
// Part of the modular KonScript self-hosted compiler

// ── Type tracking ─────────────────────────────────────────────────────────
let mut node_types: [Str] = [""];

// ── Symbol table ──────────────────────────────────────────────────────────
let mut sym_names:  [Str] = [""];
let mut sym_types:  [Str] = [""];
let mut sym_scopes: [I32] = [0];
let mut sym_count:  I32   = 0;
let mut cur_scope:  I32   = 0;

// ── Ownership tracking ───────────────────────────────────────────────────
let mut sym_moved: [Bool] = [false];  // has this variable been moved?
let mut sym_used:  [Bool] = [false];  // has this variable been used?
let mut sym_mut:   [Bool] = [false];  // is this variable mutable?

// ── Borrow tracking ──────────────────────────────────────────────────────
let mut borrow_var_names: [Str] = [""];
let mut borrow_kinds:     [Str] = [""];  // "shared" or "mut"
let mut borrow_scopes:    [I32] = [0];
let mut borrow_count:     I32   = 0;

// ── Function table ────────────────────────────────────────────────────────
let mut fn_names:    [Str] = [""];
let mut fn_rettypes: [Str] = [""];
let mut fn_count:    I32   = 0;

// ── Strict mode flag ──────────────────────────────────────────────────────
let mut strict_mode: Bool = true;

// ── Error tracking ────────────────────────────────────────────────────────
let mut tc_error_count: I32 = 0;

func tc_error(line: I32, msg: Str) {
    Print("type error line ", line, ": ", msg);
    tc_error_count = tc_error_count + 1;
}

func tc_warning(line: I32, msg: Str) {
    if strict_mode {
        tc_error(line, msg);
    } else {
        Print("warning line ", line, ": ", msg);
    }
}

// ── Copy type check ──────────────────────────────────────────────────────
func is_copy_type(t: Str) -> Bool {
    if t == "I32" || t == "I64" || t == "I8" || t == "I16" { return true; }
    if t == "U8" || t == "U16" || t == "U32" || t == "U64" { return true; }
    if t == "F32" || t == "F64" { return true; }
    if t == "Bool" { return true; }
    return false;
}

// ── Helpers ───────────────────────────────────────────────────────────────
func func_name(s: Str) -> Str {
    let mut i: I32 = 0;
    while i < s.len() {
        if s.substr(i, 1) == "|" { return s.substr(0, i); }
        i = i + 1;
    }
    return s;
}

func func_ret(s: Str) -> Str {
    let mut i: I32 = 0;
    while i < s.len() {
        if s.substr(i, 1) == "|" { return s.substr(i + 1, s.len() - i - 1); }
        i = i + 1;
    }
    return "void";
}

func reset_tc() {
    node_types.clear();
    node_types.push("");
    let mut i: I32 = 1;
    while i < node_kinds.len() { node_types.push(""); i += 1; }
    sym_names.clear();  sym_names.push("");
    sym_types.clear();  sym_types.push("");
    sym_scopes.clear(); sym_scopes.push(0);
    sym_moved.clear();  sym_moved.push(false);
    sym_used.clear();   sym_used.push(false);
    sym_mut.clear();    sym_mut.push(false);
    sym_count = 0; cur_scope = 0;
    fn_names.clear();    fn_names.push("");
    fn_rettypes.clear(); fn_rettypes.push("");
    fn_count = 0;
    borrow_var_names.clear(); borrow_var_names.push("");
    borrow_kinds.clear();     borrow_kinds.push("");
    borrow_scopes.clear();    borrow_scopes.push(0);
    borrow_count = 0;
    tc_error_count = 0;
}

func tc_push_scope() { cur_scope = cur_scope + 1; }

func tc_pop_scope() {
    // Check for unused variables in this scope
    if strict_mode {
        let mut i: I32 = sym_count;
        while i > 0 {
            if sym_scopes[i] == cur_scope {
                if !sym_used[i] {
                    let vname: Str = sym_names[i];
                    if vname.len() > 0 && !vname.starts("_") {
                        tc_warning(0, f"unused variable '{vname}'");
                    }
                }
            }
            i = i - 1;
        }
    }
    // Remove borrows at this scope level
    let mut bi: I32 = borrow_count;
    while bi > 0 {
        if borrow_scopes[bi] == cur_scope {
            borrow_count = borrow_count - 1;
        }
        bi = bi - 1;
    }
    // Remove symbols at this scope level
    let mut j: I32 = sym_count;
    while j > 0 {
        if sym_scopes[j] == cur_scope { sym_count = sym_count - 1; }
        j = j - 1;
    }
    cur_scope = cur_scope - 1;
}

func tc_define(name: Str, t: Str) {
    sym_count = sym_count + 1;
    sym_names.push(name);
    sym_types.push(t);
    sym_scopes.push(cur_scope);
    sym_moved.push(false);
    sym_used.push(false);
    sym_mut.push(false);
}

func tc_define_mut(name: Str, t: Str, is_mut: Bool) {
    sym_count = sym_count + 1;
    sym_names.push(name);
    sym_types.push(t);
    sym_scopes.push(cur_scope);
    sym_moved.push(false);
    sym_used.push(false);
    sym_mut.push(is_mut);
}

func tc_lookup(name: Str) -> Str {
    let mut i: I32 = sym_count;
    while i > 0 {
        if sym_names[i] == name {
            sym_used[i] = true;
            // Check if moved
            if sym_moved[i] {
                tc_error(0, f"use of moved value '{name}'");
            }
            return sym_types[i];
        }
        i = i - 1;
    }
    let mut j: I32 = fn_count;
    while j > 0 {
        if fn_names[j] == name { return fn_rettypes[j]; }
        j = j - 1;
    }
    return "?";
}

// Mark a variable as moved (ownership transferred)
func tc_mark_moved(name: Str) {
    let mut i: I32 = sym_count;
    while i > 0 {
        if sym_names[i] == name {
            let t: Str = sym_types[i];
            if !is_copy_type(t) {
                sym_moved[i] = true;
            }
            return;
        }
        i = i - 1;
    }
}

// ── Borrow checking ──────────────────────────────────────────────────────
func tc_add_borrow(var_name: Str, kind: Str) {
    // Check existing borrows
    let mut i: I32 = borrow_count;
    while i > 0 {
        if borrow_var_names[i] == var_name {
            if kind == "mut" {
                tc_error(0, f"cannot borrow '{var_name}' as mutable — already borrowed");
                return;
            }
            if borrow_kinds[i] == "mut" {
                tc_error(0, f"cannot borrow '{var_name}' — already mutably borrowed");
                return;
            }
        }
        i = i - 1;
    }
    borrow_count = borrow_count + 1;
    borrow_var_names.push(var_name);
    borrow_kinds.push(kind);
    borrow_scopes.push(cur_scope);
}

func tc_def_fn(name: Str, ret: Str) {
    fn_count = fn_count + 1;
    fn_names.push(name);
    fn_rettypes.push(ret);
}

func tc_fn_ret(name: Str) -> Str {
    let mut i: I32 = fn_count;
    while i > 0 {
        if fn_names[i] == name { return fn_rettypes[i]; }
        i = i - 1;
    }
    return "?";
}

// ── Expression type checking ─────────────────────────────────────────────
func tc_expr(idx: I32) -> Str {
    if idx == 0 { return "void"; }
    let k: I32 = node_kinds[idx];
    if k == NK_INT     { node_types[idx] = "I32";  return "I32"; }
    if k == NK_FLOAT   { node_types[idx] = "F32";  return "F32"; }
    if k == NK_BOOL    { node_types[idx] = "Bool"; return "Bool"; }
    if k == NK_STR_LIT { node_types[idx] = "Str";  return "Str"; }
    if k == NK_FSTR    { node_types[idx] = "Str";  return "Str"; }
    if k == NK_NULL_LIT { node_types[idx] = "?";   return "?"; }
    if k == NK_IDENT {
        let t: Str = tc_lookup(node_str[idx]);
        node_types[idx] = t; return t;
    }
    if k == NK_BINARY {
        let lt: Str = tc_expr(node_a[idx]);
        let rt: Str = tc_expr(node_b[idx]);
        let op: Str = node_str[idx];
        let mut t: Str = lt;
        if op == "==" || op == "!=" || op == "<" || op == ">" ||
           op == "<=" || op == ">=" || op == "&&" || op == "||" {
            t = "Bool";
        }
        // Bitwise ops return the left type
        if op == "&" || op == "|" || op == "^" || op == "<<" || op == ">>" {
            t = lt;
        }
        node_types[idx] = t; return t;
    }
    if k == NK_UNARY {
        let t: Str = tc_expr(node_a[idx]);
        if node_str[idx] == "!" { node_types[idx] = "Bool"; return "Bool"; }
        if node_str[idx] == "~" { node_types[idx] = t; return t; }
        node_types[idx] = t; return t;
    }
    if k == NK_ASSIGN {
        tc_expr(node_a[idx]);
        let t: Str = tc_expr(node_b[idx]);
        node_types[idx] = t; return t;
    }
    if k == NK_CALL {
        let callee: I32 = node_a[idx];
        let mut ret: Str = "?";
        if node_kinds[callee] == NK_IDENT { ret = tc_fn_ret(node_str[callee]); }
        if node_kinds[callee] == NK_MEMBER {
            let method: Str = node_str[callee];
            if method == "len" { ret = "I32"; }
            if method == "trim" || method == "upper" || method == "lower" ||
               method == "replace" || method == "substr" { ret = "Str"; }
            if method == "contains" || method == "starts" || method == "ends" ||
               method == "isEmpty" || method == "has" { ret = "Bool"; }
            if method == "split" { ret = "[Str]"; }
            if method == "clone" { ret = tc_expr(node_a[callee]); }
        }
        let mut arg: I32 = node_b[idx];
        while arg != 0 { tc_expr(node_a[arg]); arg = node_b[arg]; }
        node_types[idx] = ret; return ret;
    }
    if k == NK_MEMBER {
        tc_expr(node_a[idx]);
        let member: Str = node_str[idx];
        let mut t: Str = "?";
        if member == "ok" { t = "Bool"; }
        if member == "value" || member == "error" { t = "Str"; }
        if member == "len" { t = "I32"; }
        node_types[idx] = t; return t;
    }
    if k == NK_INDEX {
        let obj_t: Str = tc_expr(node_a[idx]);
        tc_expr(node_b[idx]);
        let mut t: Str = "?";
        if obj_t.len() > 2 && obj_t.starts("[") {
            t = obj_t.substr(1, obj_t.len() - 2);
        }
        node_types[idx] = t; return t;
    }
    if k == NK_CAST {
        tc_expr(node_a[idx]);
        node_types[idx] = node_str[idx]; return node_str[idx];
    }
    if k == NK_ARRAY_LIT {
        let mut elem: I32 = node_a[idx];
        let mut elem_t: Str = "?";
        if elem != 0 { elem_t = tc_expr(node_a[elem]); }
        let t: Str = f"[{elem_t}]";
        node_types[idx] = t; return t;
    }
    // Reference expressions
    if k == NK_REF {
        let inner_t: Str = tc_expr(node_a[idx]);
        // Track shared borrow
        if node_kinds[node_a[idx]] == NK_IDENT {
            tc_add_borrow(node_str[node_a[idx]], "shared");
        }
        let t: Str = f"&{inner_t}";
        node_types[idx] = t; return t;
    }
    if k == NK_REF_MUT {
        let inner_t: Str = tc_expr(node_a[idx]);
        // Track mutable borrow
        if node_kinds[node_a[idx]] == NK_IDENT {
            tc_add_borrow(node_str[node_a[idx]], "mut");
        }
        let t: Str = f"&mut {inner_t}";
        node_types[idx] = t; return t;
    }
    if k == NK_DEREF {
        let ptr_t: Str = tc_expr(node_a[idx]);
        // Strip leading * or & from type
        let mut t: Str = "?";
        if ptr_t.starts("*") { t = ptr_t.substr(1, ptr_t.len() - 1); }
        if ptr_t.starts("&mut ") { t = ptr_t.substr(5, ptr_t.len() - 5); }
        if ptr_t.starts("&") { t = ptr_t.substr(1, ptr_t.len() - 1); }
        node_types[idx] = t; return t;
    }
    // Lambda/closure expression
    if k == NK_FUNC_EXPR {
        let ret_str: Str = node_str[idx]; // "|rettype"
        let ret_t: Str = ret_str.substr(1, ret_str.len() - 1);
        // Build Fn type
        let mut fn_t: Str = "Fn(";
        let mut pm: I32 = node_a[idx];
        let mut first: Bool = true;
        while pm != 0 {
            let pn: I32 = node_a[pm];
            let ps: Str = node_str[pn];
            // Extract type from "name:type"
            let mut ci: I32 = 0;
            let mut ptype: Str = "";
            while ci < ps.len() {
                if ps.substr(ci, 1) == ":" {
                    ptype = ps.substr(ci + 1, ps.len() - ci - 1);
                    ci = ps.len();
                }
                ci = ci + 1;
            }
            if !first { fn_t = f"{fn_t},{ptype}"; }
            else { fn_t = f"{fn_t}{ptype}"; }
            first = false;
            pm = node_b[pm];
        }
        fn_t = f"{fn_t})->{ret_t}";
        node_types[idx] = fn_t; return fn_t;
    }
    return "?";
}

// ── Unreachable code tracking ─────────────────────────────────────────────
let mut tc_terminated: Bool = false;

func tc_stmt(idx: I32) {
    if idx == 0 { return; }
    let k: I32 = node_kinds[idx];
    if k == NK_LET {
        let t: Str = tc_expr(node_a[idx]);
        let is_mut: Bool = node_b[idx] == 1;
        tc_define_mut(node_str[idx], t, is_mut);
        // Move semantics: if initializer is an identifier of non-copy type, mark it moved
        if node_a[idx] != 0 && node_kinds[node_a[idx]] == NK_IDENT {
            let src_name: Str = node_str[node_a[idx]];
            tc_mark_moved(src_name);
        }
        node_types[idx] = t; return;
    }
    if k == NK_CONST_D {
        let t: Str = tc_expr(node_a[idx]);
        tc_define(node_str[idx], t);
        node_types[idx] = t; return;
    }
    if k == NK_RETURN {
        if node_a[idx] != 0 { tc_expr(node_a[idx]); }
        tc_terminated = true;
        return;
    }
    if k == NK_IF {
        tc_expr(node_a[idx]);
        tc_stmt(node_b[idx]);
        if node_c[idx] != 0 { tc_stmt(node_c[idx]); }
        return;
    }
    if k == NK_WHILE { tc_expr(node_a[idx]); tc_stmt(node_b[idx]); return; }
    if k == NK_LOOP  { tc_stmt(node_a[idx]); return; }
    if k == NK_FOR_IN {
        let iter_t: Str = tc_expr(node_a[idx]);
        tc_push_scope();
        let mut elem_t: Str = "?";
        if iter_t.len() > 2 && iter_t.starts("[") {
            elem_t = iter_t.substr(1, iter_t.len() - 2);
        }
        tc_define(node_str[idx], elem_t);
        tc_stmt(node_b[idx]);
        tc_pop_scope(); return;
    }
    if k == NK_BLOCK {
        tc_push_scope();
        tc_terminated = false;
        let mut s: I32 = node_a[idx];
        while s != 0 {
            if tc_terminated && strict_mode {
                tc_warning(node_line[node_a[s]], "unreachable code");
            }
            tc_stmt(node_a[s]);
            s = node_b[s];
        }
        tc_pop_scope(); return;
    }
    if k == NK_BREAK || k == NK_CONTINUE {
        tc_terminated = true;
        return;
    }
    // Extern declarations don't need type checking in the body
    if k == NK_EXTERN { return; }
    // ASM statements are unchecked
    if k == NK_ASM { return; }
    tc_expr(idx);
}

// ── Has-return check for strict mode ──────────────────────────────────────
func tc_has_return(idx: I32) -> Bool {
    if idx == 0 { return false; }
    let k: I32 = node_kinds[idx];
    if k == NK_RETURN { return true; }
    if k == NK_BLOCK {
        // Check last statement in block
        let mut s: I32 = node_a[idx];
        let mut last: I32 = 0;
        while s != 0 { last = node_a[s]; s = node_b[s]; }
        return tc_has_return(last);
    }
    if k == NK_IF {
        // Both branches must return
        if node_c[idx] == 0 { return false; }
        return tc_has_return(node_b[idx]) && tc_has_return(node_c[idx]);
    }
    return false;
}

func typecheck(prog_idx: I32) {
    reset_tc();
    let mut decl: I32 = node_a[prog_idx];
    while decl != 0 {
        let d: I32 = node_a[decl];
        if node_kinds[d] == NK_FUNC    { tc_def_fn(func_name(node_str[d]), func_ret(node_str[d])); }
        if node_kinds[d] == NK_CONST_D { let t: Str = tc_expr(node_a[d]); tc_define(node_str[d], t); }
        if node_kinds[d] == NK_LET     { let t: Str = tc_expr(node_a[d]); tc_define(node_str[d], t); }
        if node_kinds[d] == NK_EXTERN  { /* registered in irgen */ }
        decl = node_b[decl];
    }
    // Pre-register known return types for IRGen functions
    tc_def_fn("ir_escape_str", "Str");
    tc_def_fn("ir_get_v", "Str");
    tc_def_fn("ir_get_t", "Str");
    tc_def_fn("ir_val", "Str");
    tc_def_fn("func_name", "Str");
    tc_def_fn("func_ret", "Str");
    tc_def_fn("ir_type", "Str");
    tc_def_fn("tc_fn_ret", "Str");
    tc_def_fn("tc_lookup", "Str");
    tc_def_fn("loop_cond_top", "Str");
    tc_def_fn("loop_end_top", "Str");
    tc_def_fn("ir_to_string", "Str");
    tc_def_fn("irv_lookup_reg", "Str");
    tc_def_fn("irv_lookup_type", "Str");
    tc_def_fn("gconst_lookup", "Str");
    tc_def_fn("ir_gen_expr", "Str");
    tc_def_fn("ir_arglist_append", "Str");
    tc_def_fn("ir_arg_str", "Str");
    tc_def_fn("ir_tmp_id", "I32");
    tc_def_fn("ir_label_id", "I32");
    tc_def_fn("ir_str_const", "I32");
    tc_def_fn("ir_gen_func", "void");
    tc_def_fn("ir_gen_stmt", "void");
    tc_def_fn("ir_emit", "void");
    tc_def_fn("ir_emiti", "void");
    tc_def_fn("irv_push_scope", "void");
    tc_def_fn("irv_pop_scope", "void");
    tc_def_fn("irv_def", "void");
    tc_def_fn("loop_push", "void");
    tc_def_fn("loop_pop", "void");
    tc_def_fn("typecheck", "void");
    tc_def_fn("irgen", "void");
    tc_def_fn("ir_reset", "void");
    tc_def_fn("irv_reset", "void");
    tc_def_fn("reset_tc", "void");
    tc_def_fn("tc_define", "void");
    tc_def_fn("tc_push_scope", "void");
    tc_def_fn("tc_pop_scope", "void");
    tc_def_fn("gconst_define", "void");
    tc_def_fn("tc_stmt", "void");
    tc_def_fn("tc_define_mut", "void");
    tc_def_fn("tc_mark_moved", "void");
    tc_def_fn("tc_add_borrow", "void");
    tc_def_fn("tc_error", "void");
    tc_def_fn("tc_warning", "void");
    tc_def_fn("is_copy_type", "Bool");
    tc_def_fn("tc_has_return", "Bool");
    decl = node_a[prog_idx];
    while decl != 0 {
        let d: I32 = node_a[decl];
        if node_kinds[d] == NK_FUNC {
            tc_push_scope();
            let mut pm: I32 = node_a[d];
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
                        ptype = ps.substr(ci + 1, ps.len() - ci - 1);
                        ci = ps.len();
                    } else {
                        if !found { pname = f"{pname}{ch}"; }
                    }
                    ci += 1;
                }
                if pname.len() > 0 { tc_define(pname, ptype); }
                pm = node_b[pm];
            }
            tc_stmt(node_b[d]);
            // Check for missing return in non-void functions
            let ret_t: Str = func_ret(node_str[d]);
            if strict_mode && ret_t != "void" && ret_t != "?" {
                if !tc_has_return(node_b[d]) {
                    let fname: Str = func_name(node_str[d]);
                    // Skip main — it gets a default return
                    if fname != "main" {
                        tc_warning(node_line[d], f"function '{fname}' may not return a value on all paths");
                    }
                }
            }
            tc_pop_scope();
        }
        decl = node_b[decl];
    }
}
