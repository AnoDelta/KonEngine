// ks_lexer.ks — Lexer for KonScript self-hosted compiler

// -----------------------------------------------------------------------
// Error helpers
// -----------------------------------------------------------------------
func lex_error(line: I32, col: I32, msg: Str) {
    Print("lexer error ", line, ":", col, ": ", msg);
}

func parse_error(line: I32, col: I32, msg: Str) {
    Print("parse error ", line, ":", col, ": ", msg);
}

// -----------------------------------------------------------------------
// Character helpers
// -----------------------------------------------------------------------
func is_digit(c: Str) -> Bool {
    return c >= "0" && c <= "9";
}

func is_alpha(c: Str) -> Bool {
    return (c >= "a" && c <= "z") || (c >= "A" && c <= "Z") || c == "_";
}

func is_alnum(c: Str) -> Bool {
    return is_alpha(c) || is_digit(c);
}

func is_hex(c: Str) -> Bool {
    return is_digit(c) || (c >= "a" && c <= "f") || (c >= "A" && c <= "F");
}

// -----------------------------------------------------------------------
// Keyword dispatch
// -----------------------------------------------------------------------
func keyword_kind(w: Str) -> I32 {
    if w == "func"     { return TK_FUNC; }
    if w == "let"      { return TK_LET; }
    if w == "mut"      { return TK_MUT; }
    if w == "return"   { return TK_RETURN; }
    if w == "if"       { return TK_IF; }
    if w == "else"     { return TK_ELSE; }
    if w == "while"    { return TK_WHILE; }
    if w == "for"      { return TK_FOR; }
    if w == "in"       { return TK_IN; }
    if w == "loop"     { return TK_LOOP; }
    if w == "struct"   { return TK_STRUCT; }
    if w == "enum"     { return TK_ENUM; }
    if w == "pub"      { return TK_PUB; }
    if w == "const"    { return TK_CONST; }
    if w == "as"       { return TK_AS; }
    if w == "break"    { return TK_BREAK; }
    if w == "continue" { return TK_CONTINUE; }
    if w == "true"     { return TK_TRUE; }
    if w == "false"    { return TK_FALSE; }
    if w == "node"     { return TK_NODE; }
    if w == "class"    { return TK_CLASS; }
    if w == "switch"   { return TK_SWITCH; }
    if w == "spawn"    { return TK_SPAWN; }
    // New keywords
    if w == "extern"   { return TK_EXTERN; }
    if w == "asm"      { return TK_ASM; }
    if w == "union"    { return TK_UNION; }
    if w == "volatile" { return TK_VOLATILE; }
    if w == "unsafe"   { return TK_UNSAFE; }
    if w == "move"     { return TK_MOVE; }
    return TK_IDENT;
}

// -----------------------------------------------------------------------
// emit_token — push one token onto the parallel arrays
// -----------------------------------------------------------------------
func emit_token(kind: I32, value: Str, line: I32, col: I32) {
    tok_kinds.push(kind);
    tok_values.push(value);
    tok_lines.push(line);
    tok_cols.push(col);
}

// -----------------------------------------------------------------------
// Include tracking for circular include prevention
// -----------------------------------------------------------------------
let mut include_stack: [Str] = [""];
let mut include_depth: I32 = 0;

// -----------------------------------------------------------------------
// lex — main lexer function
// Takes source string, populates tok_* arrays.
// Returns number of tokens emitted (including EOF).
// -----------------------------------------------------------------------
func lex(src: Str) -> I32 {
    tok_kinds.clear();
    tok_values.clear();
    tok_lines.clear();
    tok_cols.clear();
    let mut i:    I32 = 0;
    let mut line: I32 = 1;
    let mut col:  I32 = 1;
    let n: I32 = src.len();

    while i < n {
        let c: Str = src.substr(i, 1);

        // ── Newline ────────────────────────────────────────────────
        if c == "\n" {
            line = line + 1;
            col  = 1;
            i   += 1;
            continue;
        }

        // ── Other whitespace ───────────────────────────────────────
        if c == " " || c == "\t" || c == "\r" {
            col += 1;
            i   += 1;
            continue;
        }

        // ── Line comment  //  ──────────────────────────────────────
        if c == "/" && i + 1 < n && src.substr(i + 1, 1) == "/" {
            while i < n && src.substr(i, 1) != "\n" {
                i += 1;
            }
            continue;
        }

        // ── Block comment  /* */  ──────────────────────────────────
        if c == "/" && i + 1 < n && src.substr(i + 1, 1) == "*" {
            let start_line: I32 = line;
            let start_col:  I32 = col;
            i += 2; col += 2;
            while i + 1 < n {
                if src.substr(i, 1) == "\n" {
                    line = line + 1; col = 1; i += 1;
                } else {
                    if src.substr(i, 1) == "*" && i + 1 < n && src.substr(i + 1, 1) == "/" {
                        i += 2; col += 2;
                        break;
                    }
                    col += 1; i += 1;
                }
            }
            continue;
        }


        // ── F-string literal  f"..."  ──────────────────────────────
        if c == "f" && i + 1 < n && src.substr(i + 1, 1) == "\"" {
            let tok_col: I32 = col;
            let mut val: Str = "";
            i += 2; col += 2; // skip f"
            let mut depth: I32 = 0;
            while i < n {
                let fsc: Str = src.substr(i, 1);
                if fsc == "\"" && depth == 0 {
                    i += 1; col += 1;
                    break;
                }
                if fsc == "{" { depth = depth + 1; val = val + "{"; i += 1; col += 1; }
                else {
                    if fsc == "}" && depth > 0 { depth = depth - 1; val = val + "}"; i += 1; col += 1; }
                    else {
                        if fsc == "\\" && i + 1 < n {
                            let fesc: Str = src.substr(i + 1, 1);
                            if fesc == "n"  { val = f"{val}\n"; i += 2; col += 2; }
                            else {
                                if fesc == "t"  { val = f"{val}\t"; i += 2; col += 2; }
                                else {
                                    val = f"{val}{fesc}";
                                    i += 2; col += 2;
                                }
                            }
                        } else {
                            if fsc == "\n" { line = line + 1; col = 1; val = f"{val}\n"; i += 1; }
                            else { val = f"{val}{fsc}"; i += 1; col += 1; }
                        }
                    }
                }
            }
            emit_token(TK_FSTR, val, line, tok_col);
            continue;
        }

        // ── String literal  "..."  ─────────────────────────────────
        if c == "\"" {
            let tok_col: I32 = col;
            let mut val: Str = "";
            i += 1; col += 1;
            while i < n && src.substr(i, 1) != "\"" {
                let sc: Str = src.substr(i, 1);
                if sc == "\\" && i + 1 < n {
                    let esc: Str = src.substr(i + 1, 1);
                    if esc == "n"  { val = f"{val}\n"; i += 2; col += 2; }
                    else {
                        if esc == "t"  { val = f"{val}\t"; i += 2; col += 2; }
                        else {
                            if esc == "\\" { val = f"{val}\\"; i += 2; col += 2; }
                            else {
                                if esc == "\"" { val = f"{val}\""; i += 2; col += 2; }
                                else {
                                    val = f"{val}{esc}";
                                    i += 2; col += 2;
                                }
                            }
                        }
                    }
                } else {
                    if sc == "\n" { line = line + 1; col = 1; }
                    val = f"{val}{sc}";
                    i += 1; col += 1;
                }
            }
            if i < n { i += 1; col += 1; } // closing quote
            else { lex_error(line, tok_col, "unterminated string"); }
            emit_token(TK_STR, val, line, tok_col);
            continue;
        }

        // ── Integer / Float literal ────────────────────────────────
        if is_digit(c) {
            let tok_col: I32 = col;
            let mut num: Str = "";
            let mut is_float: Bool = false;
            while i < n && is_digit(src.substr(i, 1)) {
                num = f"{num}{src.substr(i, 1)}";
                i += 1; col += 1;
            }
            // decimal part
            if i < n && src.substr(i, 1) == "." && i + 1 < n && is_digit(src.substr(i + 1, 1)) {
                is_float = true;
                num = f"{num}.";
                i += 1; col += 1;
                while i < n && is_digit(src.substr(i, 1)) {
                    num = f"{num}{src.substr(i, 1)}";
                    i += 1; col += 1;
                }
            }
            if is_float {
                emit_token(TK_FLOAT, num, line, tok_col);
            } else {
                emit_token(TK_INT, num, line, tok_col);
            }
            continue;
        }

        // ── Identifier / keyword ───────────────────────────────────
        if is_alpha(c) {
            let tok_col: I32 = col;
            let mut word: Str = "";
            while i < n && is_alnum(src.substr(i, 1)) {
                word = f"{word}{src.substr(i, 1)}";
                i += 1; col += 1;
            }
            let kind: I32 = keyword_kind(word);
            emit_token(kind, word, line, tok_col);
            continue;
        }

        // ── #include directive ─────────────────────────────────────
        if c == "#" && i + 7 < n && src.substr(i, 8) == "#include" {
            let tok_col: I32 = col;
            i += 8; col += 8;
            // skip whitespace
            while i < n && (src.substr(i, 1) == " " || src.substr(i, 1) == "\t") {
                i += 1; col += 1;
            }
            // read path (quoted or angle)
            let mut path: Str = "";
            if i < n && (src.substr(i, 1) == "\"" || src.substr(i, 1) == "<") {
                let mut close: Str = "\"";
                if src.substr(i, 1) == "<" { close = ">"; }
                i += 1; col += 1;
                while i < n && src.substr(i, 1) != close {
                    path = f"{path}{src.substr(i, 1)}";
                    i += 1; col += 1;
                }
                if i < n { i += 1; col += 1; }
            }
            emit_token(TK_INCLUDE, path, line, tok_col);
            continue;
        }

        // ── Two-character symbols ──────────────────────────────────
        if i + 1 < n {
            let two: Str = src.substr(i, 2);
            if two == "==" { emit_token(TK_EQEQ,       "==", line, col); i += 2; col += 2; continue; }
            if two == "!=" { emit_token(TK_BANGEQ,      "!=", line, col); i += 2; col += 2; continue; }
            if two == "<=" { emit_token(TK_LTEQ,        "<=", line, col); i += 2; col += 2; continue; }
            if two == ">=" { emit_token(TK_GTEQ,        ">=", line, col); i += 2; col += 2; continue; }
            if two == "<<" { emit_token(TK_LTLT,        "<<", line, col); i += 2; col += 2; continue; }
            if two == ">>" { emit_token(TK_GTGT,        ">>", line, col); i += 2; col += 2; continue; }
            if two == "&&" { emit_token(TK_AMPERAMPER,  "&&", line, col); i += 2; col += 2; continue; }
            if two == "||" { emit_token(TK_PIPEPIPE,    "||", line, col); i += 2; col += 2; continue; }
            if two == "->" { emit_token(TK_ARROW,       "->", line, col); i += 2; col += 2; continue; }
            if two == "+=" { emit_token(TK_PLUSEQ,      "+=", line, col); i += 2; col += 2; continue; }
            if two == "-=" { emit_token(TK_MINUSEQ,     "-=", line, col); i += 2; col += 2; continue; }
            if two == "*=" { emit_token(TK_STAREQ,      "*=", line, col); i += 2; col += 2; continue; }
            if two == "/=" { emit_token(TK_SLASHEQ,     "/=", line, col); i += 2; col += 2; continue; }
            if two == "&=" { emit_token(TK_AMPEQ,       "&=", line, col); i += 2; col += 2; continue; }
            if two == "|=" { emit_token(TK_PIPEEQ,      "|=", line, col); i += 2; col += 2; continue; }
            if two == "^=" { emit_token(TK_CARETEQ,     "^=", line, col); i += 2; col += 2; continue; }
            if two == "++" { emit_token(TK_PLUSPLUS,    "++", line, col); i += 2; col += 2; continue; }
            if two == "--" { emit_token(TK_MINUSMINUS,  "--", line, col); i += 2; col += 2; continue; }
            if two == ".." {
                if i + 2 < n && src.substr(i + 2, 1) == "=" {
                    emit_token(TK_DOTDOTEQ, "..=", line, col); i += 3; col += 3; continue;
                }
                emit_token(TK_DOTDOT, "..", line, col); i += 2; col += 2; continue;
            }
            if two == "::" { emit_token(TK_COLONCOLON, "::", line, col); i += 2; col += 2; continue; }
        }

        // ── Single-character symbols ───────────────────────────────
        if c == "+" { emit_token(TK_PLUS,      "+", line, col); i += 1; col += 1; continue; }
        if c == "-" { emit_token(TK_MINUS,     "-", line, col); i += 1; col += 1; continue; }
        if c == "*" { emit_token(TK_STAR,      "*", line, col); i += 1; col += 1; continue; }
        if c == "/" { emit_token(TK_SLASH,     "/", line, col); i += 1; col += 1; continue; }
        if c == "%" { emit_token(TK_PERCENT,   "%", line, col); i += 1; col += 1; continue; }
        if c == "=" { emit_token(TK_EQ,        "=", line, col); i += 1; col += 1; continue; }
        if c == "!" { emit_token(TK_BANG,      "!", line, col); i += 1; col += 1; continue; }
        if c == "<" { emit_token(TK_LT,        "<", line, col); i += 1; col += 1; continue; }
        if c == ">" { emit_token(TK_GT,        ">", line, col); i += 1; col += 1; continue; }
        if c == "(" { emit_token(TK_LPAREN,    "(", line, col); i += 1; col += 1; continue; }
        if c == ")" { emit_token(TK_RPAREN,    ")", line, col); i += 1; col += 1; continue; }
        if c == "{" { emit_token(TK_LBRACE,    "{", line, col); i += 1; col += 1; continue; }
        if c == "}" { emit_token(TK_RBRACE,    "}", line, col); i += 1; col += 1; continue; }
        if c == "[" { emit_token(TK_LBRACKET,  "[", line, col); i += 1; col += 1; continue; }
        if c == "]" { emit_token(TK_RBRACKET,  "]", line, col); i += 1; col += 1; continue; }
        if c == "," { emit_token(TK_COMMA,     ",", line, col); i += 1; col += 1; continue; }
        if c == "." { emit_token(TK_DOT,       ".", line, col); i += 1; col += 1; continue; }
        if c == ":" { emit_token(TK_COLON,     ":", line, col); i += 1; col += 1; continue; }
        if c == ";" { emit_token(TK_SEMICOLON, ";", line, col); i += 1; col += 1; continue; }
        if c == "?" { emit_token(TK_QUESTION,  "?", line, col); i += 1; col += 1; continue; }
        // New: bitwise single-char operators
        if c == "&" { emit_token(TK_AMP,       "&", line, col); i += 1; col += 1; continue; }
        if c == "|" { emit_token(TK_PIPE,      "|", line, col); i += 1; col += 1; continue; }
        if c == "^" { emit_token(TK_CARET,     "^", line, col); i += 1; col += 1; continue; }
        if c == "~" { emit_token(TK_TILDE,     "~", line, col); i += 1; col += 1; continue; }

        // ── Unknown character — report and skip ────────────────────
        lex_error(line, col, f"unexpected character '{c}'");
        i += 1; col += 1;
    }

    emit_token(TK_EOF, "", line, col);
    return tok_kinds.len();
}

// -----------------------------------------------------------------------
// token_kind_name — for debug dumps
// -----------------------------------------------------------------------
func token_kind_name(k: I32) -> Str {
    if k == TK_EOF        { return "EOF"; }
    if k == TK_INT        { return "INT"; }
    if k == TK_FLOAT      { return "FLOAT"; }
    if k == TK_STR        { return "STR"; }
    if k == TK_FSTR       { return "FSTR"; }
    if k == TK_IDENT      { return "IDENT"; }
    if k == TK_FUNC       { return "func"; }
    if k == TK_LET        { return "let"; }
    if k == TK_MUT        { return "mut"; }
    if k == TK_RETURN     { return "return"; }
    if k == TK_IF         { return "if"; }
    if k == TK_ELSE       { return "else"; }
    if k == TK_WHILE      { return "while"; }
    if k == TK_FOR        { return "for"; }
    if k == TK_IN         { return "in"; }
    if k == TK_LOOP       { return "loop"; }
    if k == TK_STRUCT     { return "struct"; }
    if k == TK_ENUM       { return "enum"; }
    if k == TK_PUB        { return "pub"; }
    if k == TK_CONST      { return "const"; }
    if k == TK_AS         { return "as"; }
    if k == TK_BREAK      { return "break"; }
    if k == TK_CONTINUE   { return "continue"; }
    if k == TK_TRUE       { return "true"; }
    if k == TK_FALSE      { return "false"; }
    if k == TK_NODE       { return "node"; }
    if k == TK_CLASS      { return "class"; }
    if k == TK_INCLUDE    { return "#include"; }
    if k == TK_EXTERN     { return "extern"; }
    if k == TK_ASM        { return "asm"; }
    if k == TK_UNION      { return "union"; }
    if k == TK_VOLATILE   { return "volatile"; }
    if k == TK_UNSAFE     { return "unsafe"; }
    if k == TK_MOVE       { return "move"; }
    if k == TK_PLUS       { return "+"; }
    if k == TK_MINUS      { return "-"; }
    if k == TK_STAR       { return "*"; }
    if k == TK_SLASH      { return "/"; }
    if k == TK_PERCENT    { return "%"; }
    if k == TK_EQ         { return "="; }
    if k == TK_EQEQ       { return "=="; }
    if k == TK_BANG       { return "!"; }
    if k == TK_BANGEQ     { return "!="; }
    if k == TK_LT         { return "<"; }
    if k == TK_LTEQ       { return "<="; }
    if k == TK_GT         { return ">"; }
    if k == TK_GTEQ       { return ">="; }
    if k == TK_AMPERAMPER { return "&&"; }
    if k == TK_PIPEPIPE   { return "||"; }
    if k == TK_AMP        { return "&"; }
    if k == TK_PIPE       { return "|"; }
    if k == TK_CARET      { return "^"; }
    if k == TK_TILDE      { return "~"; }
    if k == TK_LTLT       { return "<<"; }
    if k == TK_GTGT       { return ">>"; }
    if k == TK_LPAREN     { return "("; }
    if k == TK_RPAREN     { return ")"; }
    if k == TK_LBRACE     { return "{"; }
    if k == TK_RBRACE     { return "}"; }
    if k == TK_LBRACKET   { return "["; }
    if k == TK_RBRACKET   { return "]"; }
    if k == TK_COMMA      { return ","; }
    if k == TK_DOT        { return "."; }
    if k == TK_DOTDOT     { return ".."; }
    if k == TK_DOTDOTEQ   { return "..="; }
    if k == TK_COLON      { return ":"; }
    if k == TK_COLONCOLON { return "::"; }
    if k == TK_SEMICOLON  { return ";"; }
    if k == TK_ARROW      { return "->"; }
    if k == TK_PLUSEQ     { return "+="; }
    if k == TK_MINUSEQ    { return "-="; }
    if k == TK_STAREQ     { return "*="; }
    if k == TK_SLASHEQ    { return "/="; }
    if k == TK_PLUSPLUS   { return "++"; }
    if k == TK_MINUSMINUS { return "--"; }
    if k == TK_QUESTION   { return "?"; }
    return "?";
}

// -----------------------------------------------------------------------
// dump_tokens — print all lexed tokens for debugging
// -----------------------------------------------------------------------
func dump_tokens() {
    let n: I32 = tok_kinds.len();
    let mut i: I32 = 0;
    while i < n {
        let k: I32    = tok_kinds[i];
        let v: Str    = tok_values[i];
        let ln: I32   = tok_lines[i];
        let cl: I32   = tok_cols[i];
        let name: Str = token_kind_name(k);
        if v.len() > 0 && v != name {
            Print(ln, ":", cl, "\t", name, "\t'", v, "'");
        } else {
            Print(ln, ":", cl, "\t", name);
        }
        i += 1;
    }
}

// -----------------------------------------------------------------------
// IR emit helpers (early declaration, used by later modules)
// -----------------------------------------------------------------------
func next_tmp() -> I32 {
    let t: I32 = tmp_count;
    tmp_count = tmp_count + 1;
    return t;
}

func next_str() -> I32 {
    let s: I32 = str_count;
    str_count = str_count + 1;
    return s;
}

func emit(line: Str) {
    Print(line);
}

func emiti(line: Str) {
    Print("  ", line);
}

func emit_module_header(filename: Str) {
    Print("; Generated by KonScript (self-hosted) v0.1");
    Print("; Source: ", filename);
    Print("target datalayout = \"e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128\"");
    Print("target triple = \"x86_64-pc-linux-gnu\"");
    Print("");
    Print("; --- runtime ---");
    Print("declare i32 @printf(i8* nocapture, ...)");
    Print("declare i8* @malloc(i64)");
    Print("declare void @free(i8*)");
    Print("declare i32 @strlen(i8*)");
    Print("");
}
