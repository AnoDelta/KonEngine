// ---------------------------------------------------------------------------
// konscript.ks — the KonScript compiler, written in KonScript
//
// Stage plan:
//   Stage 0 — C++ compiler (src/main.cpp)       <- current
//   Stage 1 — this file compiled by stage 0      <- IN PROGRESS
//   Stage 2 — this file compiled by stage 1      <- goal (self-hosting)
//
// Progress:
//   [x] Token constants
//   [x] Lexer
//   [ ] Parser
//   [ ] Typechecker
//   [ ] IRGen
//
// Build stage 1:
//   konscript --cpp konscript.ks -o konscript1.cpp
//   clang++ -std=c++17 konscript1.cpp -o konscript1
// Verify:
//   ./konscript1 hello.ks  (should lex hello.ks and print tokens)
// ---------------------------------------------------------------------------

// -----------------------------------------------------------------------
// Token kind constants
// -----------------------------------------------------------------------
const TK_EOF:        I32 = 0;
const TK_INT:        I32 = 1;
const TK_FLOAT:      I32 = 2;
const TK_STR:        I32 = 3;
const TK_IDENT:      I32 = 4;
const TK_FUNC:       I32 = 5;
const TK_LET:        I32 = 6;
const TK_MUT:        I32 = 7;
const TK_RETURN:     I32 = 8;
const TK_IF:         I32 = 9;
const TK_ELSE:       I32 = 10;
const TK_WHILE:      I32 = 11;
const TK_FOR:        I32 = 12;
const TK_IN:         I32 = 13;
const TK_LOOP:       I32 = 14;
const TK_STRUCT:     I32 = 15;
const TK_ENUM:       I32 = 16;
const TK_PUB:        I32 = 17;
const TK_CONST:      I32 = 18;
const TK_AS:         I32 = 19;
const TK_BREAK:      I32 = 20;
const TK_CONTINUE:   I32 = 21;
const TK_TRUE:       I32 = 22;
const TK_FALSE:      I32 = 23;
const TK_NODE:       I32 = 24;
const TK_CLASS:      I32 = 25;
const TK_INCLUDE:    I32 = 26;
const TK_SWITCH:     I32 = 27;
const TK_SPAWN:      I32 = 28;

// Symbols
const TK_PLUS:       I32 = 40;
const TK_MINUS:      I32 = 41;
const TK_STAR:       I32 = 42;
const TK_SLASH:      I32 = 43;
const TK_PERCENT:    I32 = 44;
const TK_EQ:         I32 = 45;
const TK_EQEQ:       I32 = 46;
const TK_BANG:       I32 = 47;
const TK_BANGEQ:     I32 = 48;
const TK_LT:         I32 = 49;
const TK_LTEQ:       I32 = 50;
const TK_GT:         I32 = 51;
const TK_GTEQ:       I32 = 52;
const TK_AMPERAMPER: I32 = 53;
const TK_PIPEPIPE:   I32 = 54;
const TK_LPAREN:     I32 = 55;
const TK_RPAREN:     I32 = 56;
const TK_LBRACE:     I32 = 57;
const TK_RBRACE:     I32 = 58;
const TK_LBRACKET:   I32 = 59;
const TK_RBRACKET:   I32 = 60;
const TK_COMMA:      I32 = 61;
const TK_DOT:        I32 = 62;
const TK_DOTDOT:     I32 = 63;
const TK_COLON:      I32 = 64;
const TK_SEMICOLON:  I32 = 65;
const TK_ARROW:      I32 = 66;
const TK_PLUSEQ:     I32 = 67;
const TK_MINUSEQ:    I32 = 68;
const TK_STAREQ:     I32 = 69;
const TK_SLASHEQ:    I32 = 70;
const TK_PLUSPLUS:   I32 = 71;
const TK_MINUSMINUS: I32 = 72;
const TK_DOTDOTEQ:   I32 = 73;
const TK_QUESTION:   I32 = 74;
const TK_FSTR:       I32 = 75;
const TK_COLONCOLON: I32 = 76;

// -----------------------------------------------------------------------
// Token storage — parallel arrays, one slot per token
// -----------------------------------------------------------------------
let mut tok_kinds:  [I32] = [0];
let mut tok_values: [Str] = [""];
let mut tok_lines:  [I32] = [0];
let mut tok_cols:   [I32] = [0];

// IR emit counters
let mut tmp_count: I32 = 0;
let mut str_count: I32 = 0;

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
// Single characters are compared as 1-char strings via substr(i,1)
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
// Keyword dispatch — returns token kind for known keywords, TK_IDENT otherwise
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
                    if src.substr(i, 1) == "*" && src.substr(i + 1, 1) == "/" {
                        i += 2; col += 2;
                        break;
                    }
                    col += 1; i += 1;
                }
            }
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
            if two == "&&" { emit_token(TK_AMPERAMPER,  "&&", line, col); i += 2; col += 2; continue; }
            if two == "||" { emit_token(TK_PIPEPIPE,    "||", line, col); i += 2; col += 2; continue; }
            if two == "->" { emit_token(TK_ARROW,       "->", line, col); i += 2; col += 2; continue; }
            if two == "+=" { emit_token(TK_PLUSEQ,      "+=", line, col); i += 2; col += 2; continue; }
            if two == "-=" { emit_token(TK_MINUSEQ,     "-=", line, col); i += 2; col += 2; continue; }
            if two == "*=" { emit_token(TK_STAREQ,      "*=", line, col); i += 2; col += 2; continue; }
            if two == "/=" { emit_token(TK_SLASHEQ,     "/=", line, col); i += 2; col += 2; continue; }
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
// IR emit helpers
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

// ========================================================================
// PARSER
// ========================================================================

// ── Node kind constants ──────────────────────────────────────────────────
const NK_NULL:      I32 = 0;  // null / invalid
const NK_INT:       I32 = 1;  // str=value
const NK_FLOAT:     I32 = 2;  // str=value
const NK_BOOL:      I32 = 3;  // str="true"/"false"
const NK_STR_LIT:   I32 = 4;  // str=value
const NK_IDENT:     I32 = 5;  // str=name
const NK_BINARY:    I32 = 6;  // str=op,  a=left,   b=right
const NK_UNARY:     I32 = 7;  // str=op,  a=operand
const NK_CALL:      I32 = 8;  // a=callee, b=args list head
const NK_MEMBER:    I32 = 9;  // a=object, str=member
const NK_INDEX:     I32 = 10; // a=object, b=index
const NK_ASSIGN:    I32 = 11; // str=op,  a=target, b=value
const NK_LIST:      I32 = 12; // a=item,  b=next (0=end)
const NK_LET:       I32 = 13; // str=name, a=init, b=is_mut
const NK_CONST_D:   I32 = 14; // str=name, a=init
const NK_RETURN:    I32 = 15; // a=value (0=void return)
const NK_IF:        I32 = 16; // a=cond, b=then_block, c=else_block
const NK_WHILE:     I32 = 17; // a=cond, b=body
const NK_FOR_IN:    I32 = 18; // str=var, a=iter, b=body
const NK_LOOP:      I32 = 19; // a=body
const NK_BREAK:     I32 = 20;
const NK_CONTINUE:  I32 = 21;
const NK_BLOCK:     I32 = 22; // a=first NK_LIST (0=empty), b=stmt count
const NK_FUNC:      I32 = 23; // str=name, a=params list, b=body
const NK_PARAM:     I32 = 24; // str="name:type", a=next param
const NK_PROGRAM:   I32 = 25; // a=decls list head
const NK_CAST:      I32 = 26; // a=value, str=type
const NK_ARRAY_LIT: I32 = 27; // a=elements list head
const NK_INCLUDE_D: I32 = 28; // str=path
const NK_NULL_LIT:  I32 = 29;
const NK_STRUCT_D:  I32 = 30; // str=name, a=fields list
const NK_FIELD:     I32 = 31; // str="name:type"

// ── AST storage — parallel arrays, index 0 is the null node ─────────────
let mut node_kinds: [I32] = [0];
let mut node_a:     [I32] = [0];
let mut node_b:     [I32] = [0];
let mut node_c:     [I32] = [0];  // third child (else, extra)
let mut node_str:   [Str] = [""];
let mut node_line:  [I32] = [0];

// ── Parser state ─────────────────────────────────────────────────────────
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

// Build a NK_LIST chain from a list of node indices
// list_append(head, tail, item) -> new_tail
// Use as: head=0; tail=0; then for each item call list_append
func list_node(item: I32, next: I32) -> I32 {
    return alloc_node(NK_LIST, item, next, 0, "");
}

// ── Type annotation — returns type string e.g. "I32", "[Str]", "I32?" ────
func parse_type() -> Str {
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

// ── Expression parsing (recursive descent, precedence climbing) ───────────

// Forward declarations (KonScript supports forward refs via typechecker prepass)

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
    // Bool literals
    if chk(TK_TRUE)  { adv(); return alloc_node(NK_BOOL, 0, 0, 0, "true"); }
    if chk(TK_FALSE) { adv(); return alloc_node(NK_BOOL, 0, 0, 0, "false"); }
    // Null / None
    if chk(TK_IDENT) && pk_val() == "null" { adv(); return alloc_node(NK_NULL_LIT, 0, 0, 0, "null"); }
    if chk(TK_IDENT) && pk_val() == "None" { adv(); return alloc_node(NK_NULL_LIT, 0, 0, 0, "None"); }

    // Identifier
    if chk(TK_IDENT) {
        let name: Str = pk_val(); adv();
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
    adv(); // skip bad token
    return 0;
}

func parse_postfix() -> I32 {
    let mut nd: I32 = parse_primary();
    loop {
        // Function call: foo(a, b)
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
        // Member access: a.b
        if mat(TK_DOT) {
            let member: Str = eat(TK_IDENT, "member name");
            nd = alloc_node(NK_MEMBER, nd, 0, 0, member);
            continue;
        }
        // Index: a[i]
        if mat(TK_LBRACKET) {
            let idx: I32 = parse_or();
            eat(TK_RBRACKET, "']'");
            nd = alloc_node(NK_INDEX, nd, idx, 0, "");
            continue;
        }
        // Type cast: x as T
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

func parse_cmp() -> I32 {
    let mut left: I32 = parse_add();
    while chk(TK_LT) || chk(TK_LTEQ) || chk(TK_GT) || chk(TK_GTEQ) {
        let op: Str = pk_val(); adv();
        let right: I32 = parse_add();
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

func parse_and() -> I32 {
    let mut left: I32 = parse_eq();
    while chk(TK_AMPERAMPER) {
        adv();
        let right: I32 = parse_eq();
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
    // Assignment operators
    if chk(TK_EQ) || chk(TK_PLUSEQ) || chk(TK_MINUSEQ) || chk(TK_STAREQ) || chk(TK_SLASHEQ) {
        let op: Str = pk_val(); adv();
        let right: I32 = parse_or();
        return alloc_node(NK_ASSIGN, left, right, 0, op);
    }
    return left;
}

// ── Statement parser ──────────────────────────────────────────────────────
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
        // Optional type annotation
        if mat(TK_COLON) { parse_type(); } // consume and discard for now
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

    // block
    if chk(TK_LBRACE) { return parse_block(); }

    // Expression statement (call, assignment, etc.)
    let expr: I32 = parse_expr();
    mat(TK_SEMICOLON);
    return expr;
}

// ── Top-level declarations ────────────────────────────────────────────────
func parse_func() -> I32 {
    eat(TK_FUNC, "'func'");
    let name: Str = eat(TK_IDENT, "function name");
    eat(TK_LPAREN, "'('");

    // Parameters
    let mut phead: I32 = 0;
    let mut ptail: I32 = 0;
    let mut pcnt: I32 = 0;
    while !chk(TK_RPAREN) && pk() != TK_EOF {
        if pcnt > 0 { eat(TK_COMMA, "','"); }
        let pname: Str = eat(TK_IDENT, "parameter name");
        eat(TK_COLON, "':'");
        let ptype: Str = parse_type();
        let pnode: I32 = alloc_node(NK_PARAM, 0, 0, 0, f"{pname}:{ptype}");
        let ln: I32 = list_node(pnode, 0);
        if phead == 0 { phead = ln; ptail = ln; }
        else { node_b[ptail] = ln; ptail = ln; }
        pcnt = pcnt + 1;
    }
    eat(TK_RPAREN, "')'");

    // Return type
    if mat(TK_ARROW) { parse_type(); } // consume, discard for now

    let body: I32 = parse_block();
    return alloc_node(NK_FUNC, phead, body, pcnt, name);
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

func parse_top_level() -> I32 {
    // Skip pub
    if chk(TK_PUB) { adv(); }
    if chk(TK_FUNC)   { return parse_func(); }
    if chk(TK_STRUCT) { return parse_struct(); }
    if chk(TK_CONST)  { return parse_stmt(); }
    if chk(TK_LET)    { return parse_stmt(); }
    if chk(TK_INCLUDE) {
        let path: Str = pk_val(); adv();
        return alloc_node(NK_INCLUDE_D, 0, 0, 0, path);
    }
    // Unknown — skip
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

    // Recurse for common patterns
    if k == NK_FUNC {
        // params
        let mut p: I32 = node_a[idx];
        while p != 0 {
            dump_node(node_a[p], indent + 1);
            p = node_b[p];
        }
        dump_node(node_b[idx], indent + 1);
    } else {
        if k == NK_BLOCK {
            let mut p: I32 = node_a[idx];
            while p != 0 {
                dump_node(node_a[p], indent + 1);
                p = node_b[p];
            }
        } else {
            if k == NK_PROGRAM {
                let mut p: I32 = node_a[idx];
                while p != 0 {
                    dump_node(node_a[p], indent + 1);
                    p = node_b[p];
                }
            } else {
                if node_a[idx] != 0 { dump_node(node_a[idx], indent + 1); }
                if node_b[idx] != 0 { dump_node(node_b[idx], indent + 1); }
                if node_c[idx] != 0 { dump_node(node_c[idx], indent + 1); }
            }
        }
    }
}

// -----------------------------------------------------------------------
// main
//
// Stage 1: read a .ks file, lex + parse it, dump the AST summary.
// -----------------------------------------------------------------------
func main() -> I32 {
    Print("KonScript self-hosted compiler v0.1 — stage 1 (lexer + parser)");
    Print("");

    let path_result: Result<Str> = File.read(".ks_input");
    if !path_result.ok {
        Print("Usage: echo 'path/to/file.ks' > .ks_input && ./konscript1");
        return 1;
    }

    let input_path: Str = path_result.value.trim();
    if input_path.len() == 0 {
        Print("error: .ks_input is empty");
        return 1;
    }

    let src_result: Result<Str> = File.read(input_path);
    if !src_result.ok {
        Print("error: cannot read '", input_path, "': ", src_result.error);
        return 1;
    }

    let src: Str = src_result.value;

    // Stage 1: Lex
    Print("Lexing:  ", input_path);
    let ntoks: I32 = lex(src);
    Print("Tokens:  ", ntoks);

    // Stage 2: Parse
    Print("Parsing...");
    let prog: I32 = parse(ntoks);
    let decl_count: I32 = node_b[prog];
    Print("Nodes:   ", node_kinds.len());
    Print("Decls:   ", decl_count);
    Print("");

    // Dump top-level declarations
    let mut p: I32 = node_a[prog];
    while p != 0 {
        dump_node(node_a[p], 0);
        p = node_b[p];
    }

    return 0;
}

