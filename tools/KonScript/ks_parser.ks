// ks_parser.ks — Parser for KonScript self-hosted compiler
// Includes: AST helpers, expression/statement parsing, extern/asm/lambda/union

// ── Parser state ──────��──────────────────────────────────────────────────
let mut pos: I32 = 0;

func reset_ast() {
    node_kinds.clear(); node_kinds.push(0);
    node_a.clear();     node_a.push(0);
    node_b.clear();     node_b.push(0);
    node_c.clear();     node_c.push(0);
    node_str.clear();   node_str.push("");
    node_line.clear();  node_line.push(0);
    pos = 0;
}

func alloc_node(kind: I32, a: I32, b: I32, c: I32, s: Str) -> I32 {
    let idx: I32 = node_kinds.len();
    node_kinds.push(kind);
    node_a.push(a);
    node_b.push(b);
    node_c.push(c);
    node_str.push(s);
    node_line.push(tok_lines[pos]);
    return idx;
}

// ── Token peek / consume helpers ─────────────────────────────────────────
func pk() -> I32         { return tok_kinds[pos]; }
func pk_val() -> Str     { return tok_values[pos]; }
func pk_ln() -> I32      { return tok_lines[pos]; }
func pk_col() -> I32     { return tok_cols[pos]; }
func adv()               { pos = pos + 1; }
func chk(k: I32) -> Bool { return tok_kinds[pos] == k; }

func mat(k: I32) -> Bool {
    if tok_kinds[pos] == k { pos = pos + 1; return true; }
    return false;
}

func eat(k: I32, msg: Str) -> Str {
    if tok_kinds[pos] != k {
        parse_error(pk_ln(), pk_col(), f"expected {msg}, got '{pk_val()}'");
        return "";
    }
    let v: Str = tok_values[pos];
    pos = pos + 1;
    return v;
}

func list_node(item: I32, next: I32) -> I32 {
    return alloc_node(NK_LIST, item, next, 0, "");
}

// ── Type annotation — returns type string e.g. "I32", "[Str]", "*I32", "&mut I32"
func parse_type() -> Str {
    // Pointer type: *T or *mut T or *void
    if chk(TK_STAR) {
        adv();
        if chk(TK_MUT) {
            adv();
            let inner: Str = parse_type();
            return f"*mut {inner}";
        }
        if chk(TK_IDENT) && pk_val() == "void" {
            adv();
            return "*void";
        }
        let inner: Str = parse_type();
        return f"*{inner}";
    }
    // Reference type: &T or &mut T
    if chk(TK_AMP) {
        adv();
        if chk(TK_MUT) {
            adv();
            let inner: Str = parse_type();
            return f"&mut {inner}";
        }
        let inner: Str = parse_type();
        return f"&{inner}";
    }
    // Array type: [T]
    if mat(TK_LBRACKET) {
        let inner: Str = parse_type();
        eat(TK_RBRACKET, "']'");
        let mut t: Str = f"[{inner}]";
        if mat(TK_QUESTION) { t = f"{t}?"; }
        return t;
    }
    // Tuple type: (A, B, ...)
    if chk(TK_LPAREN) {
        adv();
        let mut t: Str = "(";
        let mut first: Bool = true;
        while !chk(TK_RPAREN) && pk() != TK_EOF {
            if !first { eat(TK_COMMA, "','"); t = f"{t},"; }
            t = f"{t}{parse_type()}";
            first = false;
        }
        eat(TK_RPAREN, "')'");
        if mat(TK_QUESTION) { t = f"{t}?"; }
        return f"{t})";
    }
    // Function type: Fn(A, B) -> C
    if chk(TK_IDENT) && pk_val() == "Fn" {
        adv();
        eat(TK_LPAREN, "'('");
        let mut t: Str = "Fn(";
        let mut first: Bool = true;
        while !chk(TK_RPAREN) && pk() != TK_EOF {
            if !first { eat(TK_COMMA, "','"); t = f"{t},"; }
            t = f"{t}{parse_type()}";
            first = false;
        }
        eat(TK_RPAREN, "')'");
        t = f"{t})";
        if mat(TK_ARROW) {
            let ret: Str = parse_type();
            t = f"{t}->{ret}";
        }
        return t;
    }
    // Named type (possibly generic: Result<T>, HashMap<K,V>)
    let name: Str = eat(TK_IDENT, "type name");
    let mut t: Str = name;
    if mat(TK_LT) {
        t = f"{t}<";
        let mut first: Bool = true;
        while !chk(TK_GT) && pk() != TK_EOF {
            if !first { eat(TK_COMMA, "','"); t = f"{t},"; }
            t = f"{t}{parse_type()}";
            first = false;
        }
        eat(TK_GT, "'>'");
        t = f"{t}>";
    }
    if mat(TK_QUESTION) { t = f"{t}?"; }
    return t;
}

// ── Expression parsing with full precedence chain ───────────────────────
// Precedence (low to high):
//   or -> and -> bitor -> bitxor -> bitand -> eq -> cmp -> shift -> add -> mul -> unary -> postfix -> primary

func parse_primary() -> I32 {
    let ln: I32 = pk_ln();
    let cl: I32 = pk_col();

    // Integer literal
    if chk(TK_INT) {
        let v: Str = pk_val(); adv();
        return alloc_node(NK_INT, 0, 0, 0, v);
    }
    // Float literal
    if chk(TK_FLOAT) {
        let v: Str = pk_val(); adv();
        return alloc_node(NK_FLOAT, 0, 0, 0, v);
    }
    // String literal
    if chk(TK_STR) {
        let v: Str = pk_val(); adv();
        return alloc_node(NK_STR_LIT, 0, 0, 0, v);
    }
    // F-string literal
    if chk(TK_FSTR) {
        let v: Str = pk_val(); adv();
        return alloc_node(NK_FSTR, 0, 0, 0, v);
    }
    // Bool literals
    if chk(TK_TRUE)  { adv(); return alloc_node(NK_BOOL, 0, 0, 0, "true"); }
    if chk(TK_FALSE) { adv(); return alloc_node(NK_BOOL, 0, 0, 0, "false"); }
    // Null / None
    if chk(TK_IDENT) && pk_val() == "null" { adv(); return alloc_node(NK_NULL_LIT, 0, 0, 0, "null"); }
    if chk(TK_IDENT) && pk_val() == "None" { adv(); return alloc_node(NK_NULL_LIT, 0, 0, 0, "None"); }

    // Lambda/closure expression: func(params) -> RetType { body }
    if chk(TK_FUNC) {
        adv();
        eat(TK_LPAREN, "'('");
        let mut phead: I32 = 0;
        let mut ptail: I32 = 0;
        let mut pcnt: I32 = 0;
        while !chk(TK_RPAREN) && pk() != TK_EOF {
            if pcnt > 0 { eat(TK_COMMA, "','"); }
            let pname: Str = eat(TK_IDENT, "param name");
            eat(TK_COLON, "':'");
            let ptype: Str = parse_type();
            let pnode: I32 = alloc_node(NK_PARAM, 0, 0, 0, f"{pname}:{ptype}");
            let pln: I32 = list_node(pnode, 0);
            if phead == 0 { phead = pln; ptail = pln; }
            else { node_b[ptail] = pln; ptail = pln; }
            pcnt = pcnt + 1;
        }
        eat(TK_RPAREN, "')'");
        let mut ret_type: Str = "void";
        if mat(TK_ARROW) { ret_type = parse_type(); }
        let body: I32 = parse_block();
        return alloc_node(NK_FUNC_EXPR, phead, body, pcnt, f"|{ret_type}");
    }

    // Identifier (and sizeof/alignof builtins)
    if chk(TK_IDENT) {
        let name: Str = pk_val(); adv();
        // sizeof<T>() and alignof<T>()
        if (name == "sizeof" || name == "alignof") && chk(TK_LT) {
            adv();
            let t: Str = parse_type();
            eat(TK_GT, "'>'");
            eat(TK_LPAREN, "'('");
            eat(TK_RPAREN, "')'");
            return alloc_node(NK_CALL, alloc_node(NK_IDENT, 0, 0, 0, name), 0, 0, t);
        }
        return alloc_node(NK_IDENT, 0, 0, 0, name);
    }

    // Array literal [a, b, c]
    if mat(TK_LBRACKET) {
        let mut head: I32 = 0;
        let mut tail: I32 = 0;
        let mut cnt: I32 = 0;
        while !chk(TK_RBRACKET) && pk() != TK_EOF {
            if cnt > 0 { eat(TK_COMMA, "','"); }
            let elem: I32 = parse_or();
            let nd: I32 = list_node(elem, 0);
            if head == 0 { head = nd; tail = nd; }
            else { node_b[tail] = nd; tail = nd; }
            cnt = cnt + 1;
        }
        eat(TK_RBRACKET, "']'");
        return alloc_node(NK_ARRAY_LIT, head, cnt, 0, "");
    }

    // Parenthesised expression
    if mat(TK_LPAREN) {
        let inner: I32 = parse_or();
        eat(TK_RPAREN, "')'");
        return inner;
    }

    parse_error(ln, cl, f"unexpected token '{pk_val()}' in expression");
    adv();
    return 0;
}

func parse_postfix() -> I32 {
    let mut nd: I32 = parse_primary();
    loop {
        if mat(TK_LPAREN) {
            let mut head: I32 = 0;
            let mut tail: I32 = 0;
            let mut cnt: I32 = 0;
            while !chk(TK_RPAREN) && pk() != TK_EOF {
                if cnt > 0 { eat(TK_COMMA, "','"); }
                let arg: I32 = parse_or();
                let ln2: I32 = list_node(arg, 0);
                if head == 0 { head = ln2; tail = ln2; }
                else { node_b[tail] = ln2; tail = ln2; }
                cnt = cnt + 1;
            }
            eat(TK_RPAREN, "')'");
            nd = alloc_node(NK_CALL, nd, head, 0, "");
            continue;
        }
        if mat(TK_DOT) {
            let member: Str = eat(TK_IDENT, "member name");
            nd = alloc_node(NK_MEMBER, nd, 0, 0, member);
            continue;
        }
        if mat(TK_LBRACKET) {
            let idx: I32 = parse_or();
            eat(TK_RBRACKET, "']'");
            nd = alloc_node(NK_INDEX, nd, idx, 0, "");
            continue;
        }
        if mat(TK_AS) {
            let t: Str = parse_type();
            nd = alloc_node(NK_CAST, nd, 0, 0, t);
            continue;
        }
        break;
    }
    return nd;
}

func parse_unary() -> I32 {
    if chk(TK_MINUS) { adv(); let a: I32 = parse_unary(); return alloc_node(NK_UNARY, a, 0, 0, "-"); }
    if chk(TK_BANG)  { adv(); let a: I32 = parse_unary(); return alloc_node(NK_UNARY, a, 0, 0, "!"); }
    if chk(TK_TILDE) { adv(); let a: I32 = parse_unary(); return alloc_node(NK_UNARY, a, 0, 0, "~"); }
    // & and &mut for references
    if chk(TK_AMP) {
        adv();
        if chk(TK_MUT) {
            adv();
            let a: I32 = parse_unary();
            return alloc_node(NK_REF_MUT, a, 0, 0, "");
        }
        let a: I32 = parse_unary();
        return alloc_node(NK_REF, a, 0, 0, "");
    }
    // *expr for dereference
    if chk(TK_STAR) {
        adv();
        let a: I32 = parse_unary();
        return alloc_node(NK_DEREF, a, 0, 0, "");
    }
    return parse_postfix();
}

func parse_mul() -> I32 {
    let mut left: I32 = parse_unary();
    while chk(TK_STAR) || chk(TK_SLASH) || chk(TK_PERCENT) {
        let op: Str = pk_val(); adv();
        let right: I32 = parse_unary();
        left = alloc_node(NK_BINARY, left, right, 0, op);
    }
    return left;
}

func parse_add() -> I32 {
    let mut left: I32 = parse_mul();
    while chk(TK_PLUS) || chk(TK_MINUS) {
        let op: Str = pk_val(); adv();
        let right: I32 = parse_mul();
        left = alloc_node(NK_BINARY, left, right, 0, op);
    }
    return left;
}

func parse_shift() -> I32 {
    let mut left: I32 = parse_add();
    while chk(TK_LTLT) || chk(TK_GTGT) {
        let op: Str = pk_val(); adv();
        let right: I32 = parse_add();
        left = alloc_node(NK_BINARY, left, right, 0, op);
    }
    return left;
}

func parse_cmp() -> I32 {
    let mut left: I32 = parse_shift();
    while chk(TK_LT) || chk(TK_LTEQ) || chk(TK_GT) || chk(TK_GTEQ) {
        let op: Str = pk_val(); adv();
        let right: I32 = parse_shift();
        left = alloc_node(NK_BINARY, left, right, 0, op);
    }
    return left;
}

func parse_eq() -> I32 {
    let mut left: I32 = parse_cmp();
    while chk(TK_EQEQ) || chk(TK_BANGEQ) {
        let op: Str = pk_val(); adv();
        let right: I32 = parse_cmp();
        left = alloc_node(NK_BINARY, left, right, 0, op);
    }
    return left;
}

func parse_bitand() -> I32 {
    let mut left: I32 = parse_eq();
    while chk(TK_AMP) {
        adv();
        let right: I32 = parse_eq();
        left = alloc_node(NK_BINARY, left, right, 0, "&");
    }
    return left;
}

func parse_bitxor() -> I32 {
    let mut left: I32 = parse_bitand();
    while chk(TK_CARET) {
        adv();
        let right: I32 = parse_bitand();
        left = alloc_node(NK_BINARY, left, right, 0, "^");
    }
    return left;
}

func parse_bitor() -> I32 {
    let mut left: I32 = parse_bitxor();
    while chk(TK_PIPE) {
        adv();
        let right: I32 = parse_bitxor();
        left = alloc_node(NK_BINARY, left, right, 0, "|");
    }
    return left;
}

func parse_and() -> I32 {
    let mut left: I32 = parse_bitor();
    while chk(TK_AMPERAMPER) {
        adv();
        let right: I32 = parse_bitor();
        left = alloc_node(NK_BINARY, left, right, 0, "&&");
    }
    return left;
}

func parse_or() -> I32 {
    let mut left: I32 = parse_and();
    while chk(TK_PIPEPIPE) {
        adv();
        let right: I32 = parse_and();
        left = alloc_node(NK_BINARY, left, right, 0, "||");
    }
    return left;
}

func parse_expr() -> I32 {
    let left: I32 = parse_or();
    if chk(TK_EQ) || chk(TK_PLUSEQ) || chk(TK_MINUSEQ) || chk(TK_STAREQ) || chk(TK_SLASHEQ) ||
       chk(TK_AMPEQ) || chk(TK_PIPEEQ) || chk(TK_CARETEQ) {
        let op: Str = pk_val(); adv();
        let right: I32 = parse_or();
        return alloc_node(NK_ASSIGN, left, right, 0, op);
    }
    return left;
}

// ── Statement parser ────────────���─────────────────────────────────────────
func parse_block() -> I32 {
    eat(TK_LBRACE, "'{'");
    let mut head: I32 = 0;
    let mut tail: I32 = 0;
    let mut cnt: I32 = 0;
    while !chk(TK_RBRACE) && pk() != TK_EOF {
        let s: I32 = parse_stmt();
        if s != 0 {
            let ln: I32 = list_node(s, 0);
            if head == 0 { head = ln; tail = ln; }
            else { node_b[tail] = ln; tail = ln; }
            cnt = cnt + 1;
        }
    }
    eat(TK_RBRACE, "'}'");
    return alloc_node(NK_BLOCK, head, cnt, 0, "");
}

func parse_stmt() -> I32 {
    let ln: I32 = pk_ln();
    let cl: I32 = pk_col();

    // let / let mut
    if chk(TK_LET) {
        adv();
        let mut is_mut: I32 = 0;
        if mat(TK_MUT) { is_mut = 1; }
        let name: Str = eat(TK_IDENT, "variable name");
        if mat(TK_COLON) { parse_type(); }
        let mut init: I32 = 0;
        if mat(TK_EQ) { init = parse_expr(); }
        mat(TK_SEMICOLON);
        return alloc_node(NK_LET, init, is_mut, 0, name);
    }

    // const
    if chk(TK_CONST) {
        adv();
        let name: Str = eat(TK_IDENT, "const name");
        if mat(TK_COLON) { parse_type(); }
        eat(TK_EQ, "'='");
        let init: I32 = parse_expr();
        mat(TK_SEMICOLON);
        return alloc_node(NK_CONST_D, init, 0, 0, name);
    }

    // return
    if chk(TK_RETURN) {
        adv();
        let mut val: I32 = 0;
        if !chk(TK_SEMICOLON) && !chk(TK_RBRACE) {
            val = parse_expr();
        }
        mat(TK_SEMICOLON);
        return alloc_node(NK_RETURN, val, 0, 0, "");
    }

    // if / else
    if chk(TK_IF) {
        adv();
        let cond: I32 = parse_expr();
        let then_b: I32 = parse_block();
        let mut else_b: I32 = 0;
        if mat(TK_ELSE) {
            if chk(TK_IF) { else_b = parse_stmt(); }
            else { else_b = parse_block(); }
        }
        return alloc_node(NK_IF, cond, then_b, else_b, "");
    }

    // while
    if chk(TK_WHILE) {
        adv();
        let cond: I32 = parse_expr();
        let body: I32 = parse_block();
        return alloc_node(NK_WHILE, cond, body, 0, "");
    }

    // loop
    if chk(TK_LOOP) {
        adv();
        let body: I32 = parse_block();
        return alloc_node(NK_LOOP, body, 0, 0, "");
    }

    // for x in iter
    if chk(TK_FOR) {
        adv();
        let var_name: Str = eat(TK_IDENT, "loop variable");
        if mat(TK_COLON) { parse_type(); }
        eat(TK_IN, "'in'");
        let iter: I32 = parse_expr();
        let body: I32 = parse_block();
        return alloc_node(NK_FOR_IN, iter, body, 0, var_name);
    }

    // break / continue
    if chk(TK_BREAK)    { adv(); mat(TK_SEMICOLON); return alloc_node(NK_BREAK,    0,0,0,""); }
    if chk(TK_CONTINUE) { adv(); mat(TK_SEMICOLON); return alloc_node(NK_CONTINUE, 0,0,0,""); }

    // asm statement: asm("template" : outputs : inputs : clobbers)
    if chk(TK_ASM) {
        adv();
        eat(TK_LPAREN, "'('");
        let tmpl: Str = eat(TK_STR, "asm template string");
        let mut outputs: Str = "";
        let mut inputs: Str = "";
        let mut clobbers: Str = "";
        if mat(TK_COLON) {
            if chk(TK_STR) { outputs = pk_val(); adv(); }
            if mat(TK_COLON) {
                if chk(TK_STR) { inputs = pk_val(); adv(); }
                if mat(TK_COLON) {
                    if chk(TK_STR) { clobbers = pk_val(); adv(); }
                }
            }
        }
        eat(TK_RPAREN, "')'");
        mat(TK_SEMICOLON);
        return alloc_node(NK_ASM, 0, 0, 0, f"{tmpl}|{outputs}|{inputs}|{clobbers}");
    }

    // block
    if chk(TK_LBRACE) { return parse_block(); }

    // Expression statement
    let expr: I32 = parse_expr();
    mat(TK_SEMICOLON);
    return expr;
}

// ── Top-level declarations ────────────────────────────────────────────────
func parse_func() -> I32 {
    eat(TK_FUNC, "'func'");
    let name: Str = eat(TK_IDENT, "function name");
    eat(TK_LPAREN, "'('");
    let mut phead: I32 = 0;
    let mut ptail: I32 = 0;
    let mut pcnt: I32 = 0;
    while !chk(TK_RPAREN) && pk() != TK_EOF {
        if pcnt > 0 { eat(TK_COMMA, "','"); }
        let pname: Str = eat(TK_IDENT, "parameter name");
        eat(TK_COLON, "':'");
        let ptype: Str = parse_type();
        let pnode: I32 = alloc_node(NK_PARAM, 0, 0, 0, f"{pname}:{ptype}");
        let pln: I32 = list_node(pnode, 0);
        if phead == 0 { phead = pln; ptail = pln; }
        else { node_b[ptail] = pln; ptail = pln; }
        pcnt = pcnt + 1;
    }
    eat(TK_RPAREN, "')'");
    let mut ret_type_str: Str = "void";
    if mat(TK_ARROW) { ret_type_str = parse_type(); }
    let body: I32 = parse_block();
    return alloc_node(NK_FUNC, phead, body, pcnt, f"{name}|{ret_type_str}");
}

func parse_struct() -> I32 {
    eat(TK_STRUCT, "'struct'");
    let name: Str = eat(TK_IDENT, "struct name");
    eat(TK_LBRACE, "'{'");
    let mut fhead: I32 = 0;
    let mut ftail: I32 = 0;
    let mut fcnt: I32 = 0;
    while !chk(TK_RBRACE) && pk() != TK_EOF {
        let fname: Str = eat(TK_IDENT, "field name");
        eat(TK_COLON, "':'");
        let ftype: Str = parse_type();
        mat(TK_COMMA);
        let fn2: I32 = alloc_node(NK_FIELD, 0, 0, 0, f"{fname}:{ftype}");
        let ln2: I32 = list_node(fn2, 0);
        if fhead == 0 { fhead = ln2; ftail = ln2; }
        else { node_b[ftail] = ln2; ftail = ln2; }
        fcnt = fcnt + 1;
    }
    eat(TK_RBRACE, "'}'");
    return alloc_node(NK_STRUCT_D, fhead, fcnt, 0, name);
}

// extern "C" func name(params...) -> RetType;
func parse_extern() -> I32 {
    // Already consumed 'extern'
    let mut linkage: Str = "C";
    if chk(TK_STR) { linkage = pk_val(); adv(); }
    eat(TK_FUNC, "'func'");
    let name: Str = eat(TK_IDENT, "function name");
    eat(TK_LPAREN, "'('");
    let mut params: Str = "";
    let mut first: Bool = true;
    let mut is_variadic: Bool = false;
    while !chk(TK_RPAREN) && pk() != TK_EOF {
        if !first { eat(TK_COMMA, "','"); }
        // Check for variadic ...
        if chk(TK_DOT) {
            adv(); adv(); adv(); // consume ...
            is_variadic = true;
            break;
        }
        // Named param: name: Type or just Type
        let mut ptype: Str = "";
        let pname: Str = eat(TK_IDENT, "parameter");
        if mat(TK_COLON) {
            ptype = parse_type();
        } else {
            ptype = pname;
        }
        if !first { params = f"{params},{ptype}"; }
        else { params = ptype; }
        first = false;
    }
    eat(TK_RPAREN, "')'");
    let mut ret: Str = "void";
    if mat(TK_ARROW) { ret = parse_type(); }
    mat(TK_SEMICOLON);
    let mut variadic_str: Str = "";
    if is_variadic { variadic_str = "..."; }
    return alloc_node(NK_EXTERN, 0, 0, 0, f"{name}|{linkage}|{ret}|{params}|{variadic_str}");
}

// union Name { field1: Type1, field2: Type2 }
func parse_union() -> I32 {
    eat(TK_UNION, "'union'");
    let name: Str = eat(TK_IDENT, "union name");
    eat(TK_LBRACE, "'{'");
    let mut fhead: I32 = 0;
    let mut ftail: I32 = 0;
    let mut fcnt: I32 = 0;
    while !chk(TK_RBRACE) && pk() != TK_EOF {
        let fname: Str = eat(TK_IDENT, "field name");
        eat(TK_COLON, "':'");
        let ftype: Str = parse_type();
        mat(TK_COMMA);
        let fn2: I32 = alloc_node(NK_FIELD, 0, 0, 0, f"{fname}:{ftype}");
        let ln2: I32 = list_node(fn2, 0);
        if fhead == 0 { fhead = ln2; ftail = ln2; }
        else { node_b[ftail] = ln2; ftail = ln2; }
        fcnt = fcnt + 1;
    }
    eat(TK_RBRACE, "'}'");
    return alloc_node(NK_UNION_D, fhead, fcnt, 0, name);
}

func parse_top_level() -> I32 {
    if chk(TK_PUB) { adv(); }
    if chk(TK_FUNC)   { return parse_func(); }
    if chk(TK_STRUCT) { return parse_struct(); }
    if chk(TK_UNION)  { return parse_union(); }
    if chk(TK_CONST)  { return parse_stmt(); }
    if chk(TK_LET)    { return parse_stmt(); }
    if chk(TK_EXTERN) { adv(); return parse_extern(); }
    if chk(TK_INCLUDE) {
        let path: Str = pk_val(); adv();
        return alloc_node(NK_INCLUDE_D, 0, 0, 0, path);
    }
    parse_error(pk_ln(), pk_col(), f"unexpected token '{pk_val()}' at top level");
    adv();
    return 0;
}

func parse(ntoks: I32) -> I32 {
    reset_ast();
    pos = 0;
    let mut head: I32 = 0;
    let mut tail: I32 = 0;
    let mut cnt: I32 = 0;
    while pk() != TK_EOF {
        let decl: I32 = parse_top_level();
        if decl != 0 {
            let ln: I32 = list_node(decl, 0);
            if head == 0 { head = ln; tail = ln; }
            else { node_b[tail] = ln; tail = ln; }
            cnt = cnt + 1;
        }
    }
    return alloc_node(NK_PROGRAM, head, cnt, 0, "");
}

// ── AST dump for debugging ────────────────────────────────────────────────
func node_kind_name(k: I32) -> Str {
    if k == NK_NULL      { return "null"; }
    if k == NK_INT       { return "int"; }
    if k == NK_FLOAT     { return "float"; }
    if k == NK_BOOL      { return "bool"; }
    if k == NK_STR_LIT   { return "str"; }
    if k == NK_IDENT     { return "ident"; }
    if k == NK_BINARY    { return "binary"; }
    if k == NK_UNARY     { return "unary"; }
    if k == NK_CALL      { return "call"; }
    if k == NK_MEMBER    { return "member"; }
    if k == NK_INDEX     { return "index"; }
    if k == NK_ASSIGN    { return "assign"; }
    if k == NK_LIST      { return "list"; }
    if k == NK_LET       { return "let"; }
    if k == NK_CONST_D   { return "const"; }
    if k == NK_RETURN    { return "return"; }
    if k == NK_IF        { return "if"; }
    if k == NK_WHILE     { return "while"; }
    if k == NK_FOR_IN    { return "for_in"; }
    if k == NK_LOOP      { return "loop"; }
    if k == NK_BREAK     { return "break"; }
    if k == NK_CONTINUE  { return "continue"; }
    if k == NK_BLOCK     { return "block"; }
    if k == NK_FUNC      { return "func"; }
    if k == NK_PARAM     { return "param"; }
    if k == NK_PROGRAM   { return "program"; }
    if k == NK_CAST      { return "cast"; }
    if k == NK_ARRAY_LIT { return "array"; }
    if k == NK_INCLUDE_D { return "include"; }
    if k == NK_NULL_LIT  { return "null_lit"; }
    if k == NK_STRUCT_D  { return "struct"; }
    if k == NK_FIELD     { return "field"; }
    if k == NK_EXTERN    { return "extern"; }
    if k == NK_ASM       { return "asm"; }
    if k == NK_UNION_D   { return "union"; }
    if k == NK_REF       { return "ref"; }
    if k == NK_REF_MUT   { return "ref_mut"; }
    if k == NK_DEREF     { return "deref"; }
    if k == NK_FUNC_EXPR { return "closure"; }
    return "?";
}

func dump_node(idx: I32, indent: I32) {
    if idx == 0 { return; }
    let mut pad: Str = "";
    let mut i: I32 = 0;
    while i < indent { pad = f"{pad}  "; i = i + 1; }
    let k: I32   = node_kinds[idx];
    let s: Str   = node_str[idx];
    let name: Str = node_kind_name(k);
    if s.len() > 0 {
        Print(pad, name, "  '", s, "'");
    } else {
        Print(pad, name);
    }
    if k == NK_FUNC {
        let mut p: I32 = node_a[idx];
        while p != 0 { dump_node(node_a[p], indent + 1); p = node_b[p]; }
        dump_node(node_b[idx], indent + 1);
    } else {
        if k == NK_BLOCK {
            let mut p: I32 = node_a[idx];
            while p != 0 { dump_node(node_a[p], indent + 1); p = node_b[p]; }
        } else {
            if k == NK_PROGRAM {
                let mut p: I32 = node_a[idx];
                while p != 0 { dump_node(node_a[p], indent + 1); p = node_b[p]; }
            } else {
                if node_a[idx] != 0 { dump_node(node_a[idx], indent + 1); }
                if node_b[idx] != 0 { dump_node(node_b[idx], indent + 1); }
                if node_c[idx] != 0 { dump_node(node_c[idx], indent + 1); }
            }
        }
    }
}
