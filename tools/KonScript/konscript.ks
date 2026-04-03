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
const NK_FSTR:      I32 = 40; // f-string
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
    let mut ret_type_str: Str = "void";
    if mat(TK_ARROW) { ret_type_str = parse_type(); }

    let body: I32 = parse_block();
    // Store return type in node_str as "name|rettype"
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


// ========================================================================
// TYPECHECKER
// ========================================================================
let mut node_types: [Str] = [""];

let mut sym_names:  [Str] = [""];
let mut sym_types:  [Str] = [""];
let mut sym_scopes: [I32] = [0];
let mut sym_count:  I32   = 0;
let mut cur_scope:  I32   = 0;

let mut fn_names:    [Str] = [""];
let mut fn_rettypes: [Str] = [""];
let mut fn_count:    I32   = 0;


// Extract function name from node_str (format: "name|rettype")
func func_name(s: Str) -> Str {
    let mut i: I32 = 0;
    while i < s.len() {
        if s.substr(i, 1) == "|" { return s.substr(0, i); }
        i = i + 1;
    }
    return s;
}

// Extract function return type from node_str
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
    sym_count = 0; cur_scope = 0;
    fn_names.clear();    fn_names.push("");
    fn_rettypes.clear(); fn_rettypes.push("");
    fn_count = 0;
}

func tc_push_scope() { cur_scope = cur_scope + 1; }

func tc_pop_scope() {
    let mut i: I32 = sym_count;
    while i > 0 {
        if sym_scopes[i] == cur_scope { sym_count = sym_count - 1; }
        i = i - 1;
    }
    cur_scope = cur_scope - 1;
}

func tc_define(name: Str, t: Str) {
    sym_count = sym_count + 1;
    sym_names.push(name);
    sym_types.push(t);
    sym_scopes.push(cur_scope);
}

func tc_lookup(name: Str) -> Str {
    let mut i: I32 = sym_count;
    while i > 0 {
        if sym_names[i] == name { return sym_types[i]; }
        i = i - 1;
    }
    let mut j: I32 = fn_count;
    while j > 0 {
        if fn_names[j] == name { return fn_rettypes[j]; }
        j = j - 1;
    }
    return "?";
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
        node_types[idx] = t; return t;
    }
    if k == NK_UNARY {
        let t: Str = tc_expr(node_a[idx]);
        if node_str[idx] == "!" { node_types[idx] = "Bool"; return "Bool"; }
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
    return "?";
}

func tc_stmt(idx: I32) {
    if idx == 0 { return; }
    let k: I32 = node_kinds[idx];
    if k == NK_LET {
        let t: Str = tc_expr(node_a[idx]);
        tc_define(node_str[idx], t);
        node_types[idx] = t; return;
    }
    if k == NK_CONST_D {
        let t: Str = tc_expr(node_a[idx]);
        tc_define(node_str[idx], t);
        node_types[idx] = t; return;
    }
    if k == NK_RETURN {
        if node_a[idx] != 0 { tc_expr(node_a[idx]); }
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
        let mut s: I32 = node_a[idx];
        while s != 0 { tc_stmt(node_a[s]); s = node_b[s]; }
        tc_pop_scope(); return;
    }
    if k == NK_BREAK || k == NK_CONTINUE { return; }
    tc_expr(idx);
}

func typecheck(prog_idx: I32) {
    reset_tc();
    let mut decl: I32 = node_a[prog_idx];
    while decl != 0 {
        let d: I32 = node_a[decl];
        if node_kinds[d] == NK_FUNC    { tc_def_fn(func_name(node_str[d]), func_ret(node_str[d])); }
        if node_kinds[d] == NK_CONST_D { let t: Str = tc_expr(node_a[d]); tc_define(node_str[d], t); }
        if node_kinds[d] == NK_LET     { let t: Str = tc_expr(node_a[d]); tc_define(node_str[d], t); }
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
    tc_def_fn("_ks_system", "I32");
    tc_def_fn("ir_write_to_file", "Bool");
    tc_def_fn("ir_str_len", "I32");
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
            tc_pop_scope();
        }
        decl = node_b[decl];
    }
}
// -----------------------------------------------------------------------
// main
// -----------------------------------------------------------------------
// ========================================================================
// IRGEN — emit LLVM IR from the AST
// ========================================================================
// Output goes to a string that's written to a .ll file.
// Uses the same parallel-array AST from the parser.

// ── IR output buffer ──────────────────────────────────────────────────────
let mut ir_lines: [Str] = [""];
let mut ir_tmp:   I32   = 0;
let mut ir_str:   I32   = 0;
let mut ir_label: I32   = 0;
let mut ir_alloca: I32  = 0;

// Global string constants: value → label index
let mut gstr_vals:   [Str] = [""];
let mut gstr_labels: [I32] = [0];
let mut gstr_lens:   [I32] = [0];  // string byte length + 1 (for null terminator)
let mut gstr_count:  I32   = 0;


// Global constant value table for IRGen
let mut gconst_names:  [Str] = [""];
let mut gconst_values: [Str] = [""];
let mut gconst_count:  I32   = 0;

func gconst_define(name: Str, val: Str) {
    gconst_count = gconst_count + 1;
    gconst_names.push(name);
    gconst_values.push(val);
}

func gconst_lookup(name: Str) -> Str {
    let mut i: I32 = gconst_count;
    while i > 0 {
        if gconst_names[i] == name { return gconst_values[i]; }
        i = i - 1;
    }
    return "";
}


// Loop label stack for break/continue
let mut loop_cond_labels: [Str] = [""];
let mut loop_end_labels:  [Str] = [""];
let mut loop_depth:       I32   = 0;

func loop_push(cond_l: Str, end_l: Str) {
    loop_depth = loop_depth + 1;
    loop_cond_labels.push(cond_l);
    loop_end_labels.push(end_l);
}

func loop_pop() {
    loop_depth = loop_depth - 1;
}

func loop_cond_top() -> Str {
    if loop_depth > 0 { return loop_cond_labels[loop_depth]; }
    return "loop_cond";
}

func loop_end_top() -> Str {
    if loop_depth > 0 { return loop_end_labels[loop_depth]; }
    return "loop_end";
}

func ir_reset() {
    ir_lines.clear(); ir_lines.push("");
    ir_tmp = 0; ir_str = 0; ir_label = 0; ir_alloca = 0;
    gstr_vals.clear();   gstr_vals.push("");
    gstr_labels.clear(); gstr_labels.push(0);
    gstr_lens.clear();   gstr_lens.push(0);
    gstr_count = 0;
    gconst_names.clear();  gconst_names.push("");
    gconst_values.clear(); gconst_values.push("");
    gconst_count = 0;
    loop_cond_labels.clear(); loop_cond_labels.push("");
    loop_end_labels.clear();  loop_end_labels.push("");
    loop_depth = 0;
}

func ir_emit(line: Str)  { ir_lines.push(line); }
func ir_emiti(line: Str) { ir_lines.push(f"  {line}"); }

func ir_tmp_id() -> I32 {
    let t: I32 = ir_tmp;
    ir_tmp = ir_tmp + 1;
    return t;
}

func ir_label_id() -> I32 {
    let l: I32 = ir_label;
    ir_label = ir_label + 1;
    return l;
}

// ── Global string dedup ───────────────────────────────────────────────────
func ir_str_const(val: Str) -> I32 {
    // Check if already exists
    let mut i: I32 = 1;
    while i <= gstr_count {
        if gstr_vals[i] == val { return gstr_labels[i]; }
        i = i + 1;
    }
    // New string — store length now so we don't need .len() later
    gstr_count = gstr_count + 1;
    let idx: I32 = gstr_count;
    gstr_vals.push(val);
    gstr_labels.push(idx);
    gstr_lens.push(val.len() + 1);
    return idx;
}

func ir_str_len(idx: I32) -> I32 {
    return gstr_lens[idx];
}

// ── LLVM type from KonScript type string ──────────────────────────────────
func ir_type(t: Str) -> Str {
    if t == "I32"  { return "i32"; }
    if t == "I64"  { return "i64"; }
    if t == "F32"  { return "float"; }
    if t == "F64"  { return "double"; }
    if t == "Bool" { return "i1"; }
    if t == "Str"  { return "i8*"; }
    if t == "void" { return "void"; }
    if t == "?"    { return "i32"; }
    // Array types → opaque pointer
    if t.starts("[") { return "i8*"; }
    return "i8*"; // default: opaque pointer
}

// ── Expression code generation ────────────────────────────────────────────
// Returns {value_str, type_str} packed as "val|type"
func ir_val(v: Str, t: Str) -> Str { return f"{v}|{t}"; }
func ir_get_v(vt: Str) -> Str {
    let mut i: I32 = 0;
    while i < vt.len() {
        if vt.substr(i, 1) == "|" { return vt.substr(0, i); }
        i = i + 1;
    }
    return vt;
}
func ir_get_t(vt: Str) -> Str {
    let mut i: I32 = 0;
    while i < vt.len() {
        if vt.substr(i, 1) == "|" { return vt.substr(i + 1, vt.len() - i - 1); }
        i = i + 1;
    }
    return "i32";
}

// Local variable table for IRGen: name → alloca register
let mut irv_names: [Str] = [""];
let mut irv_regs:  [Str] = [""];
let mut irv_types: [Str] = [""];
let mut irv_count: I32   = 0;
let mut irv_scope: I32   = 0;
let mut irv_scopes:[I32] = [0];

func irv_push_scope() { irv_scope = irv_scope + 1; }
func irv_pop_scope() {
    let mut i: I32 = irv_count;
    while i > 0 {
        if irv_scopes[i] == irv_scope { irv_count = irv_count - 1; }
        i = i - 1;
    }
    irv_scope = irv_scope - 1;
}
func irv_def(name: Str, reg: Str, t: Str) {
    irv_count = irv_count + 1;
    irv_names.push(name);
    irv_regs.push(reg);
    irv_types.push(t);
    irv_scopes.push(irv_scope);
}

// ── Global variable registry for IRGen ───────────────────────────────────────
let mut gvar_names: [Str] = [""];
let mut gvar_regs:  [Str] = [""];
let mut gvar_types: [Str] = [""];
let mut gvar_count: I32   = 0;


// Track which global names are arrays (not strings)
let mut gvar_array_names: [Str] = [""];
let mut gvar_array_count: I32 = 0;

func gvar_mark_array(name: Str) {
    gvar_array_count = gvar_array_count + 1;
    gvar_array_names.push(name);
}

func gvar_is_array(name: Str) -> Bool {
    let mut i: I32 = gvar_array_count;
    while i > 0 {
        if gvar_array_names[i] == name { return true; }
        i = i - 1;
    }
    return false;
}

func gvar_define(name: Str, reg: Str, typ: Str) {
    gvar_count = gvar_count + 1;
    gvar_names.push(name);
    gvar_regs.push(reg);
    gvar_types.push(typ);
}

func gvar_lookup_reg(name: Str) -> Str {
    let mut i: I32 = gvar_count;
    while i > 0 {
        if gvar_names[i] == name { return gvar_regs[i]; }
        i = i - 1;
    }
    return "";
}

func gvar_lookup_type(name: Str) -> Str {
    let mut i: I32 = gvar_count;
    while i > 0 {
        if gvar_names[i] == name { return gvar_types[i]; }
        i = i - 1;
    }
    return "i8*";
}

func irv_lookup_reg(name: Str) -> Str {
    let mut i: I32 = irv_count;
    while i > 0 {
        if irv_names[i] == name { return irv_regs[i]; }
        i = i - 1;
    }
    // Fall back to global variable registry
    return gvar_lookup_reg(name);
}
func irv_lookup_type(name: Str) -> Str {
    let mut i: I32 = irv_count;
    while i > 0 {
        if irv_names[i] == name { return irv_types[i]; }
        i = i - 1;
    }
    // Fall back to global variable registry
    let gt: Str = gvar_lookup_type(name);
    if gt.len() > 0 { return gt; }
    return "i32";
}
func irv_reset() {
    irv_names.clear();  irv_names.push("");
    irv_regs.clear();   irv_regs.push("");
    irv_types.clear();  irv_types.push("");
    irv_scopes.clear(); irv_scopes.push(0);
    irv_count = 0; irv_scope = 0;
}

// Current function return type
let mut ir_ret_type: Str = "void";



// Build "type val" arg string for IR call (no f-strings)
func ir_arg_str(atype: Str, aval: Str) -> Str {
    let sp: Str = " ";
    return atype + sp + aval;
}

// Append arg to arglist (no f-strings - works at IRGen compile time)
func ir_arglist_append(lst: Str, atype: Str, aval: Str) -> Str {
    let sp: Str = " ";
    let piece: Str = atype + sp + aval;
    if lst.len() == 0 { return piece; }
    let comma: Str = ", ";
    return lst + comma + piece;
}

func ir_gen_expr(idx: I32) -> Str {
    if idx == 0 { return ir_val("0", "i32"); }
    let k: I32  = node_kinds[idx];
    let nt: Str = node_types[idx];
    let lt: Str = ir_type(nt);

    if k == NK_INT {
        return ir_val(node_str[idx], "i32");
    }

    if k == NK_FLOAT {
        return ir_val(node_str[idx], "float");
    }

    if k == NK_BOOL {
        if node_str[idx] == "true"  { return ir_val("1", "i1"); }
        return ir_val("0", "i1");
    }

    if k == NK_NULL_LIT {
        return ir_val("null", "i8*");
    }


    if k == NK_FSTR {
        // F-string: emit runtime string building via _ks_str_concat
        // Parse template: split on { and }, emit concat for each piece
        let raw: Str = node_str[idx];
        let n_raw: I32 = raw.len();
        let mut i_raw: I32 = 0;
        // Start with empty string
        let empty_si: I32 = ir_str_const("");
        let t_empty: I32 = ir_tmp_id();
        ir_emiti(f"%t{t_empty} = getelementptr inbounds [1 x i8], [1 x i8]* @str.{empty_si}, i32 0, i32 0");
        let mut cur_reg: Str = f"%t{t_empty}";
        while i_raw < n_raw {
            // Find next {
            let mut j_raw: I32 = i_raw;
            while j_raw < n_raw && raw.substr(j_raw, 1) != "{" {
                j_raw = j_raw + 1;
            }
            // Emit literal segment i_raw..j_raw
            if j_raw > i_raw {
                let lit: Str = raw.substr(i_raw, j_raw - i_raw);
                let lit_si: I32 = ir_str_const(lit);
                let lit_len: I32 = lit.len() + 1;
                let t_lit: I32 = ir_tmp_id();
                ir_emiti(f"%t{t_lit} = getelementptr inbounds [{lit_len} x i8], [{lit_len} x i8]* @str.{lit_si}, i32 0, i32 0");
                let t_cat: I32 = ir_tmp_id();
                ir_emiti(f"%t{t_cat} = call i8* @_ks_str_concat(i8* {cur_reg}, i8* %t{t_lit})");
                cur_reg = f"%t{t_cat}";
            }
            if j_raw >= n_raw { i_raw = n_raw; }
            else {
                // Found {, find matching }
                j_raw = j_raw + 1;
                let mut k_raw: I32 = j_raw;
                while k_raw < n_raw && raw.substr(k_raw, 1) != "}" {
                    k_raw = k_raw + 1;
                }
                let varname: Str = raw.substr(j_raw, k_raw - j_raw);
                // Look up variable - emit load and convert
                let var_reg: Str = irv_lookup_reg(varname);
                if var_reg.len() > 0 {
                    let var_type: Str = irv_lookup_type(varname);
                    let t_load: I32 = ir_tmp_id();
                    ir_emiti(f"%t{t_load} = load {var_type}, {var_type}* {var_reg}");
                    if var_type == "i8*" {
                        let t_cat2: I32 = ir_tmp_id();
                        ir_emiti(f"%t{t_cat2} = call i8* @_ks_str_concat(i8* {cur_reg}, i8* %t{t_load})");
                        cur_reg = f"%t{t_cat2}";
                    } else {
                        let t_conv: I32 = ir_tmp_id();
                        ir_emiti(f"%t{t_conv} = call i8* @_ks_int_to_str(i32 %t{t_load})");
                        let t_cat2: I32 = ir_tmp_id();
                        ir_emiti(f"%t{t_cat2} = call i8* @_ks_str_concat(i8* {cur_reg}, i8* %t{t_conv})");
                        cur_reg = f"%t{t_cat2}";
                    }
                }
                i_raw = k_raw + 1;
            }
        }
        return ir_val(cur_reg, "i8*");
    }
    if k == NK_STR_LIT {
        let sidx: I32 = ir_str_const(node_str[idx]);
        let slen: I32 = ir_str_len(sidx);
        let t: I32 = ir_tmp_id();
        ir_emiti(f"%t{t} = getelementptr inbounds [{slen} x i8], [{slen} x i8]* @str.{sidx}, i32 0, i32 0");
        return ir_val(f"%t{t}", "i8*");
    }

    if k == NK_IDENT {
        let name: Str = node_str[idx];
        let reg: Str  = irv_lookup_reg(name);
        if reg.len() > 0 {
            let vt: Str = irv_lookup_type(name);
            let t: I32 = ir_tmp_id();
            ir_emiti(f"%t{t} = load {vt}, {vt}* {reg}");
            return ir_val(f"%t{t}", vt);
        }
        // Check global const table
        let cv: Str = gconst_lookup(name);
        if cv.len() > 0 { return ir_val(cv, "i32"); }
        // Unknown — emit 0
        return ir_val("0", "i32");
    }

    if k == NK_BINARY {
        let lvt: Str  = ir_gen_expr(node_a[idx]);
        let rvt: Str  = ir_gen_expr(node_b[idx]);
        let lv: Str   = ir_get_v(lvt);
        let rv: Str   = ir_get_v(rvt);
        let llt: Str  = ir_get_t(lvt);
        let op: Str   = node_str[idx];
        let t: I32    = ir_tmp_id();
        if op == "+" {
            let rlt: Str = ir_get_t(rvt);
            if llt == "i8*" || llt.ends("*") || rlt == "i8*" || rlt.ends("*") {
                let mut lv8: Str = lv;
                let mut rv8: Str = rv;
                if llt != "i8*" {
                    let ct: I32 = ir_tmp_id();
                    ir_emiti(f"%t{ct} = inttoptr {llt} {lv} to i8*");
                    lv8 = f"%t{ct}";
                }
                if rlt != "i8*" {
                    let ct: I32 = ir_tmp_id();
                    ir_emiti(f"%t{ct} = inttoptr {rlt} {rv} to i8*");
                    rv8 = f"%t{ct}";
                }
                ir_emiti(f"%t{t} = call i8* @_ks_str_concat(i8* {lv8}, i8* {rv8})");
                return ir_val(f"%t{t}", "i8*");
            }
            ir_emiti(f"%t{t} = add {llt} {lv}, {rv}");
            return ir_val(f"%t{t}", llt);
        }
        if op == "-"  { ir_emiti(f"%t{t} = sub {llt} {lv}, {rv}");  return ir_val(f"%t{t}", llt); }
        if op == "*"  { ir_emiti(f"%t{t} = mul {llt} {lv}, {rv}");  return ir_val(f"%t{t}", llt); }
        if op == "/"  { ir_emiti(f"%t{t} = sdiv {llt} {lv}, {rv}"); return ir_val(f"%t{t}", llt); }
        if op == "%"  { ir_emiti(f"%t{t} = srem {llt} {lv}, {rv}"); return ir_val(f"%t{t}", llt); }
        // Comparisons: normalize both sides to same type
        if op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=" {
            // Coerce pointers to i64 for comparison
            let mut clv: Str = lv;
            let mut crv: Str = rv;
            let mut clt: Str = llt;
            let rlt: Str = ir_get_t(rvt);
            // String comparison via strcmp
            if (llt == "i8*" || llt.ends("*")) && (rlt == "i8*" || rlt.ends("*")) {
                let cmp_t: I32 = ir_tmp_id();
                ir_emiti(f"%t{cmp_t} = call i32 @_ks_str_compare(i8* {lv}, i8* {rv})");
                let zero_t: I32 = ir_tmp_id();
                ir_emiti(f"%t{zero_t} = add i32 0, 0");
                if op == "==" { ir_emiti(f"%t{t} = icmp eq i32 %t{cmp_t}, 0"); return ir_val(f"%t{t}", "i1"); }
                if op == "!=" { ir_emiti(f"%t{t} = icmp ne i32 %t{cmp_t}, 0"); return ir_val(f"%t{t}", "i1"); }
                if op == "<"  { ir_emiti(f"%t{t} = icmp slt i32 %t{cmp_t}, 0"); return ir_val(f"%t{t}", "i1"); }
                if op == ">"  { ir_emiti(f"%t{t} = icmp sgt i32 %t{cmp_t}, 0"); return ir_val(f"%t{t}", "i1"); }
                if op == "<=" { ir_emiti(f"%t{t} = icmp sle i32 %t{cmp_t}, 0"); return ir_val(f"%t{t}", "i1"); }
                if op == ">=" { ir_emiti(f"%t{t} = icmp sge i32 %t{cmp_t}, 0"); return ir_val(f"%t{t}", "i1"); }
            }
            if llt == "i8*" || llt.ends("*") {
                let ct: I32 = ir_tmp_id();
                ir_emiti(f"%t{ct} = ptrtoint i8* {lv} to i64");
                clv = f"%t{ct}"; clt = "i64";
            }
            if rlt == "i8*" || rlt.ends("*") {
                let ct: I32 = ir_tmp_id();
                ir_emiti(f"%t{ct} = ptrtoint i8* {rv} to i64");
                crv = f"%t{ct}";
                // Also widen lv to i64 if it isn't already
                if clt != "i64" {
                    let ct2: I32 = ir_tmp_id();
                    if clt == "i1" { ir_emiti(f"%t{ct2} = zext i1 {clv} to i64"); }
                    else { ir_emiti(f"%t{ct2} = sext {clt} {clv} to i64"); }
                    clv = f"%t{ct2}";
                    clt = "i64";
                }
            }
            // Widen i1 to i32
            if clt == "i1" { clt = "i32"; let ct: I32 = ir_tmp_id(); ir_emiti(f"%t{ct} = zext i1 {clv} to i32"); clv = f"%t{ct}"; }
            // If one side is i64 and other is i32, widen i32 to i64
            let rlt2: Str = ir_get_t(rvt);
            if clt == "i64" && (rlt2 == "i32" || rlt2 == "i1") {
                let ct: I32 = ir_tmp_id();
                if rlt2 == "i1" { ir_emiti(f"%t{ct} = zext i1 {crv} to i64"); }
                else { ir_emiti(f"%t{ct} = sext i32 {crv} to i64"); }
                crv = f"%t{ct}";
            }
            if clt == "i32" && rlt2 == "i64" {
                let ct: I32 = ir_tmp_id();
                ir_emiti(f"%t{ct} = sext i32 {clv} to i64");
                clv = f"%t{ct}"; clt = "i64";
            }
            if op == "==" { ir_emiti(f"%t{t} = icmp eq {clt} {clv}, {crv}");  return ir_val(f"%t{t}", "i1"); }
            if op == "!=" { ir_emiti(f"%t{t} = icmp ne {clt} {clv}, {crv}");  return ir_val(f"%t{t}", "i1"); }
            if op == "<"  { ir_emiti(f"%t{t} = icmp slt {clt} {clv}, {crv}"); return ir_val(f"%t{t}", "i1"); }
            if op == ">"  { ir_emiti(f"%t{t} = icmp sgt {clt} {clv}, {crv}"); return ir_val(f"%t{t}", "i1"); }
            if op == "<=" { ir_emiti(f"%t{t} = icmp sle {clt} {clv}, {crv}"); return ir_val(f"%t{t}", "i1"); }
            if op == ">=" { ir_emiti(f"%t{t} = icmp sge {clt} {clv}, {crv}"); return ir_val(f"%t{t}", "i1"); }
        }
        if op == "&&" || op == "||" {
            // Coerce both sides to i1
            let mut alv: Str = lv;
            let mut arv: Str = rv;
            if llt != "i1" {
                let ct: I32 = ir_tmp_id();
                if llt == "i8*" || llt.ends("*") {
                    ir_emiti(f"%t{ct} = icmp ne {llt} {lv}, null");
                } else {
                    ir_emiti(f"%t{ct} = icmp ne {llt} {lv}, 0");
                }
                alv = f"%t{ct}";
            }
            let rlt2: Str = ir_get_t(rvt);
            if rlt2 != "i1" {
                let ct: I32 = ir_tmp_id();
                if rlt2 == "i8*" || rlt2.ends("*") {
                    ir_emiti(f"%t{ct} = icmp ne {rlt2} {rv}, null");
                } else {
                    ir_emiti(f"%t{ct} = icmp ne {rlt2} {rv}, 0");
                }
                arv = f"%t{ct}";
            }
            if op == "&&" { ir_emiti(f"%t{t} = and i1 {alv}, {arv}"); }
            if op == "||" { ir_emiti(f"%t{t} = or i1 {alv}, {arv}"); }
            return ir_val(f"%t{t}", "i1");
        }
        return ir_val(f"%t{t}", llt);
    }

    if k == NK_UNARY {
        let avt: Str = ir_gen_expr(node_a[idx]);
        let av: Str  = ir_get_v(avt);
        let alt: Str = ir_get_t(avt);
        let t: I32   = ir_tmp_id();
        if node_str[idx] == "-" {
            ir_emiti(f"%t{t} = sub {alt} 0, {av}");
            return ir_val(f"%t{t}", alt);
        }
        if node_str[idx] == "!" {
            let mut bv: Str = av;
            if alt != "i1" {
                let ct: I32 = ir_tmp_id();
                if alt == "i8*" || alt.ends("*") {
                    ir_emiti(f"%t{ct} = icmp ne {alt} {av}, null");
                } else {
                    ir_emiti(f"%t{ct} = icmp ne {alt} {av}, 0");
                }
                bv = f"%t{ct}";
            }
            ir_emiti(f"%t{t} = xor i1 {bv}, true");
            return ir_val(f"%t{t}", "i1");
        }
        return avt;
    }

    if k == NK_ASSIGN {
        let assign_op: Str = node_str[idx];
        let rvt: Str = ir_gen_expr(node_b[idx]);
        let rv: Str  = ir_get_v(rvt);
        let rlt: Str = ir_get_t(rvt);
        // Target: ident or index
        let tgt: I32 = node_a[idx];
        if node_kinds[tgt] == NK_IDENT {
            let reg: Str = irv_lookup_reg(node_str[tgt]);
            if reg.len() > 0 {
                let mut store_val: Str = rv;
                let mut store_type: Str = rlt;
                if assign_op != "=" {
                    let cur_type: Str = irv_lookup_type(node_str[tgt]);
                    let cur_t: I32 = ir_tmp_id();
                    ir_emiti(f"%t{cur_t} = load {cur_type}, {cur_type}* {reg}");
                    let res_t: I32 = ir_tmp_id();
                    if assign_op == "+=" { ir_emiti(f"%t{res_t} = add {cur_type} %t{cur_t}, {rv}"); }
                    if assign_op == "-=" { ir_emiti(f"%t{res_t} = sub {cur_type} %t{cur_t}, {rv}"); }
                    if assign_op == "*=" { ir_emiti(f"%t{res_t} = mul {cur_type} %t{cur_t}, {rv}"); }
                    if assign_op == "/=" { ir_emiti(f"%t{res_t} = sdiv {cur_type} %t{cur_t}, {rv}"); }
                    store_val = f"%t{res_t}";
                    store_type = cur_type;
                }
                ir_emiti(f"store {store_type} {store_val}, {store_type}* {reg}");
            }
        }
        if node_kinds[tgt] == NK_INDEX {
            let ptr_vt: Str = ir_gen_expr(node_a[tgt]);
            let ptr_v: Str  = ir_get_v(ptr_vt);
            let plt: Str    = ir_get_t(ptr_vt);
            let idx_vt: Str = ir_gen_expr(node_b[tgt]);
            let idx_v: Str  = ir_get_v(idx_vt);
            if ptr_v != "0" && ptr_v != "null" {
                let tp: I32 = ir_tmp_id();
                let tp2: I32 = ir_tmp_id();
                // Bitcast pointer to rlt* if needed
                let mut ptr_cast: Str = ptr_v;
                if plt != f"{rlt}*" && plt != rlt {
                    ir_emiti(f"%t{tp} = bitcast {plt} {ptr_v} to {rlt}*");
                    ptr_cast = f"%t{tp}";
                }
                ir_emiti(f"%t{tp2} = getelementptr {rlt}, {rlt}* {ptr_cast}, i32 {idx_v}");
                ir_emiti(f"store {rlt} {rv}, {rlt}* %t{tp2}");
            }
        }
        return rvt;
    }

    if k == NK_CALL {
        let callee: I32 = node_a[idx];
        // Build arglist inline (avoid array method calls)
        let mut arglist: Str = "";
        let mut arg: I32 = node_b[idx];
        let mut first_arg: Bool = true;
        while arg != 0 {
            let avt: Str = ir_gen_expr(node_a[arg]);
            let astr: Str = f"{ir_get_t(avt)} {ir_get_v(avt)}";
            if !first_arg { arglist = f"{arglist}, {astr}"; }
            if first_arg  { arglist = astr; first_arg = false; }
            arg = node_b[arg];
        }
        let t: I32 = ir_tmp_id();
        // Callee name
        if node_kinds[callee] == NK_IDENT {
            let fname: Str = node_str[callee];
            // Print: emit one printf per arg
            if fname == "Print" {
                let mut arg: I32 = node_b[idx];
                while arg != 0 {
                    let avt: Str = ir_gen_expr(node_a[arg]);
                    let av: Str  = ir_get_v(avt);
                    let alt: Str = ir_get_t(avt);
                    let pt: I32  = ir_tmp_id();
                    if alt == "i8*" {
                        let fmt: I32 = ir_str_const("%s");
                        ir_emiti(f"%t{pt} = getelementptr inbounds [3 x i8], [3 x i8]* @str.{fmt}, i32 0, i32 0");
                        let pt2: I32 = ir_tmp_id();
                        ir_emiti(f"%t{pt2} = call i32 (i8*, ...) @printf(i8* %t{pt}, i8* {av})");
                    } else {
                        let fmt: I32 = ir_str_const("%d");
                        ir_emiti(f"%t{pt} = getelementptr inbounds [3 x i8], [3 x i8]* @str.{fmt}, i32 0, i32 0");
                        let pt2: I32 = ir_tmp_id();
                        ir_emiti(f"%t{pt2} = call i32 (i8*, ...) @printf(i8* %t{pt}, i32 {av})");
                    }
                    arg = node_b[arg];
                }
                // Print newline
                let nlt: I32 = ir_str_const("\n");
                let pt3: I32 = ir_tmp_id();
                ir_emiti(f"%t{pt3} = getelementptr inbounds [2 x i8], [2 x i8]* @str.{nlt}, i32 0, i32 0");
                let pt4: I32 = ir_tmp_id();
                ir_emiti(f"%t{pt4} = call i32 (i8*, ...) @printf(i8* %t{pt3})");
                return ir_val("0", "void");
            }
            let fret_raw: Str = tc_fn_ret(fname);
            let fret: Str = ir_type(fret_raw);
            // Only emit as void if explicitly void
            if fret == "void" {
                ir_emiti(f"call void @{fname}({arglist})");
                return ir_val("0", "void");
            }
            // For unknown (?) return type, emit as i32 with result
            ir_emiti(f"%t{t} = call {fret} @{fname}({arglist})");
            return ir_val(f"%t{t}", fret);
        }
        // Method calls — dispatch by method name
        if node_kinds[callee] == NK_MEMBER {
            let method: Str = node_str[callee];
            // Build method arglist FIRST (needed for namespace methods)
            let mut m_arglist: Str = "";
            let mut pre_arg: I32 = node_b[idx];
            while pre_arg != 0 {
                let pre_avt: Str  = ir_gen_expr(node_a[pre_arg]);
                let pre_at: Str = ir_get_t(pre_avt);
                let pre_av: Str  = ir_get_v(pre_avt);
                m_arglist = ir_arglist_append(m_arglist, pre_at, pre_av);
                pre_arg = node_b[pre_arg];
            }
            let obj_vt: Str = ir_gen_expr(node_a[callee]);
            let obj_v_raw: Str = ir_get_v(obj_vt);
            let obj_lt: Str    = ir_get_t(obj_vt);
            // Guard: if object is null/0, check for namespace first
            if obj_v_raw == "0" || obj_v_raw == "null" {
                // Check for namespace methods (File.read etc)
                let callee_ident_kind: I32 = node_kinds[node_a[callee]];
                if callee_ident_kind == NK_IDENT {
                    let ns_name: Str = node_str[node_a[callee]];
                    if ns_name == "File" {
                        if method == "read" {
                            ir_emiti(f"%t{t} = call i8* @_ks_file_read({m_arglist})");
                            return ir_val(f"%t{t}", "i8*");
                        }
                        if method == "write" {
                            ir_emiti(f"%t{t} = call i8* @_ks_file_write({m_arglist})");
                            return ir_val(f"%t{t}", "i8*");
                        }
                    }
                }
                ir_emiti(f"%t{t} = add i32 0, 0  ; method on null obj");
                return ir_val(f"%t{t}", "i32");
            }
            if false {
                ir_emiti(f"%t{t} = add i32 0, 0  ; method on null obj");
                return ir_val(f"%t{t}", "i32");
            }
            // Cast to i8* if needed for string/array methods
            let mut obj_v: Str = obj_v_raw;
            if obj_lt != "i8*" {
                let ct: I32 = ir_tmp_id();
                ir_emiti(f"%t{ct} = inttoptr {obj_lt} {obj_v_raw} to i8*");
                obj_v = f"%t{ct}";
            }
            // Build method arglist
            // arglist already built above
            // Dispatch
            if method == "len" {
                // Use _ks_array_len for arrays, _ks_str_len for strings
                // Check IRGen type of object - arrays have type starting with "["
                let mut obj_irtype: Str = "Str";
                if node_kinds[node_a[callee]] == NK_IDENT {
                    let obj_name2: Str = node_str[node_a[callee]];
                    if gvar_is_array(obj_name2) { obj_irtype = "["; }
                    if node_types[node_a[callee]].starts("[") { obj_irtype = "["; }
                }
                if obj_irtype == "[" {
                    ir_emiti(f"%t{t} = call i32 @_ks_array_len(i8* {obj_v})");
                } else {
                    ir_emiti(f"%t{t} = call i32 @_ks_str_len(i8* {obj_v})");
                }
                return ir_val(f"%t{t}", "i32");
            }
            if method == "push" {
                if obj_v != "0" && obj_v != "null" {
                    let push_t2b: I32 = ir_tmp_id();
                    if m_arglist.starts("i8*") {
                        let push_vb: Str = m_arglist.substr(4, m_arglist.len() - 4);
                        ir_emiti(f"call void @_ks_array_push(i8* {obj_v}, i8* {push_vb})");
                    } else {
                        if m_arglist.starts("i1 ") {
                            let push_vb: Str = m_arglist.substr(3, m_arglist.len() - 3);
                            let push_t3b: I32 = ir_tmp_id();
                            ir_emiti(f"%t{push_t3b} = zext i1 {push_vb} to i32");
                            ir_emiti(f"%t{push_t2b} = inttoptr i32 %t{push_t3b} to i8*");
                            ir_emiti(f"call void @_ks_array_push(i8* {obj_v}, i8* %t{push_t2b})");
                        } else {
                            let push_vb: Str = m_arglist.substr(4, m_arglist.len() - 4);
                            ir_emiti(f"%t{push_t2b} = inttoptr i32 {push_vb} to i8*");
                            ir_emiti(f"call void @_ks_array_push(i8* {obj_v}, i8* %t{push_t2b})");
                        }
                    }
                }
                return ir_val("0", "void");
            }
            if method == "clear" {
                if obj_v != "0" && obj_v != "null" {
                    ir_emiti(f"call void @_ks_array_clear(i8* {obj_v})");
                }
                return ir_val("0", "void");
            }
            if method == "substr" {
                ir_emiti(f"%t{t} = call i8* @_ks_str_substr(i8* {obj_v}, {m_arglist})");
                return ir_val(f"%t{t}", "i8*");
            }
            if method == "trim" {
                ir_emiti(f"%t{t} = call i8* @_ks_str_trim(i8* {obj_v})");
                return ir_val(f"%t{t}", "i8*");
            }
            if method == "starts" {
                ir_emiti(f"%t{t} = call i1 @_ks_str_starts(i8* {obj_v}, {m_arglist})");
                return ir_val(f"%t{t}", "i1");
            }
            if method == "ends" {
                ir_emiti(f"%t{t} = call i1 @_ks_str_ends(i8* {obj_v}, {m_arglist})");
                return ir_val(f"%t{t}", "i1");
            }
            if method == "contains" {
                ir_emiti(f"%t{t} = call i1 @_ks_str_contains(i8* {obj_v}, {m_arglist})");
                return ir_val(f"%t{t}", "i1");
            }
            if method == "replace" {
                ir_emiti(f"%t{t} = call i8* @_ks_str_replace(i8* {obj_v}, {m_arglist})");
                return ir_val(f"%t{t}", "i8*");
            }
            if method == "read" {
                ir_emiti(f"%t{t} = call i8* @_ks_file_read(i8* {m_arglist})");
                return ir_val(f"%t{t}", "i8*");
            }
            if method == "write" {
                ir_emiti(f"%t{t} = call i32 @_ks_file_write(i8* {obj_v}, {m_arglist})");
                return ir_val(f"%t{t}", "i32");
            }
        }
        // Unknown method — placeholder
        ir_emiti(f"%t{t} = add i32 0, 0  ; method call placeholder");
        return ir_val(f"%t{t}", "i32");
    }

    if k == NK_INDEX {
        let pvt: Str = ir_gen_expr(node_a[idx]);
        let ivt: Str = ir_gen_expr(node_b[idx]);
        let pv: Str  = ir_get_v(pvt);
        let iv: Str  = ir_get_v(ivt);
        let elem_t: Str = ir_type(nt);
        let t: I32 = ir_tmp_id();
        let t2: I32 = ir_tmp_id();
        // Guard: if pointer is 0 (unknown), return 0
        if pv == "0" || pv == "null" {
            if elem_t == "i8*" || elem_t.ends("*") {
                ir_emiti(f"%t{t} = inttoptr i32 0 to {elem_t}  ; index on null ptr");
            } else {
                ir_emiti(f"%t{t} = add {elem_t} 0, 0  ; index on null ptr");
            }
            return ir_val(f"%t{t}", elem_t);
        }
        // Use _ks_array_get for KsArray structs
        if pv == "0" || pv == "null" {
            ir_emiti(f"%t{t} = add {elem_t} 0, 0  ; index on null ptr");
            return ir_val(f"%t{t}", elem_t);
        }
        // Ensure index is i32 (may be i8* if from another array_get)
        let mut iv_i32: Str = iv;
        let ivt2: Str = ir_get_t(ivt);
        if ivt2 != "i32" && ivt2 != "i1" {
            let ti1: I32 = ir_tmp_id();
            let ti2: I32 = ir_tmp_id();
            ir_emiti(f"%t{ti1} = ptrtoint {ivt2} {iv} to i64");
            ir_emiti(f"%t{ti2} = trunc i64 %t{ti1} to i32");
            iv_i32 = f"%t{ti2}";
        }
        ir_emiti(f"%t{t} = call i8* @_ks_array_get(i8* {pv}, i32 {iv_i32})");
        if elem_t == "i8*" { return ir_val(f"%t{t}", "i8*"); }
        ir_emiti(f"%t{t2} = ptrtoint i8* %t{t} to i64");
        let t3: I32 = ir_tmp_id();
        ir_emiti(f"%t{t3} = trunc i64 %t{t2} to {elem_t}");
        return ir_val(f"%t{t3}", elem_t);
    }

    if k == NK_CAST {
        let avt: Str = ir_gen_expr(node_a[idx]);
        let av: Str  = ir_get_v(avt);
        let from_t: Str = ir_get_t(avt);
        let to_t: Str   = ir_type(node_str[idx]);
        let t: I32 = ir_tmp_id();
        if from_t == to_t { return avt; }
        ir_emiti(f"%t{t} = bitcast {from_t} {av} to {to_t}");
        return ir_val(f"%t{t}", to_t);
    }


    if k == NK_MEMBER {
        let obj_vt2: Str = ir_gen_expr(node_a[idx]);
        let obj_v2: Str  = ir_get_v(obj_vt2);
        let obj_lt2: Str = ir_get_t(obj_vt2);
        let member2: Str = node_str[idx];
        // Cast to i8* if needed
        let mut obj_ptr: Str = obj_v2;
        if obj_lt2 != "i8*" && obj_v2 != "0" {
            let ct2: I32 = ir_tmp_id();
            ir_emiti(f"%t{ct2} = inttoptr {obj_lt2} {obj_v2} to i8*");
            obj_ptr = f"%t{ct2}";
        }
        let t2: I32 = ir_tmp_id();
        if member2 == "ok" {
            if obj_v2 == "0" { ir_emiti(f"%t{t2} = add i32 0, 0"); return ir_val(f"%t{t2}", "i1"); }
            ir_emiti(f"%t{t2} = call i32 @_ks_result_ok(i8* {obj_ptr})");
            let t3: I32 = ir_tmp_id();
            ir_emiti(f"%t{t3} = trunc i32 %t{t2} to i1");
            return ir_val(f"%t{t3}", "i1");
        }
        if member2 == "value" {
            if obj_v2 == "0" { ir_emiti(f"%t{t2} = inttoptr i32 0 to i8*"); return ir_val(f"%t{t2}", "i8*"); }
            ir_emiti(f"%t{t2} = call i8* @_ks_result_value(i8* {obj_ptr})");
            return ir_val(f"%t{t2}", "i8*");
        }
        if member2 == "error" {
            if obj_v2 == "0" { ir_emiti(f"%t{t2} = inttoptr i32 0 to i8*"); return ir_val(f"%t{t2}", "i8*"); }
            ir_emiti(f"%t{t2} = call i8* @_ks_result_error(i8* {obj_ptr})");
            return ir_val(f"%t{t2}", "i8*");
        }
        // Unknown member — placeholder
        ir_emiti(f"%t{t2} = add i32 0, 0  ; unknown member {member2}");
        return ir_val(f"%t{t2}", "i32");
    }
    // fallback
    let t: I32 = ir_tmp_id();
    ir_emiti(f"%t{t} = add i32 0, 0  ; unhandled nk={k}");
    return ir_val(f"%t{t}", "i32");
}

func ir_gen_stmt(idx: I32) {
    if idx == 0 { return; }
    let k: I32 = node_kinds[idx];

    if k == NK_LET || k == NK_CONST_D {
        let name: Str = node_str[idx];
        // Try pre-scanned register first, emit inline if missing
        let mut reg: Str = irv_lookup_reg(name);
        let nt: Str   = node_types[idx];
        let lt: Str   = ir_type(nt);
        if reg.len() == 0 {
            let uid: I32 = ir_alloca;
            ir_alloca = ir_alloca + 1;
            reg = f"%{name}.{uid}";
            ir_emiti(f"{reg} = alloca {lt}");
            irv_def(name, reg, lt);
        }
        if node_a[idx] != 0 {
            let vt: Str = ir_gen_expr(node_a[idx]);
            let v: Str  = ir_get_v(vt);
            let vlt: Str = ir_get_t(vt);
            ir_emiti(f"store {vlt} {v}, {vlt}* {reg}");
        }
        return;
    }

    if k == NK_RETURN {
        if node_a[idx] == 0 {
            // Use function return type for bare return
            if ir_ret_type == "void" { ir_emiti("ret void"); }
            if ir_ret_type == "i32"  { ir_emiti("ret i32 0"); }
            if ir_ret_type == "i64"  { ir_emiti("ret i64 0"); }
            if ir_ret_type == "i1"   { ir_emiti("ret i1 false"); }
            if ir_ret_type == "i8*"  { ir_emiti("ret i8* null"); }
            if ir_ret_type == "?"    { ir_emiti("ret i32 0"); }
        } else {
            let vt: Str = ir_gen_expr(node_a[idx]);
            let v: Str  = ir_get_v(vt);
            let lt: Str = ir_get_t(vt);
            // Coerce to function return type if needed
            let mut rv: Str = v;
            let mut rlt: Str = lt;
            if ir_ret_type != lt && ir_ret_type != "void" && ir_ret_type != "?" {
                let ct: I32 = ir_tmp_id();
                if ir_ret_type == "i8*" && (lt == "i32" || lt == "i64" || lt == "i1") {
                    ir_emiti(f"%t{ct} = inttoptr {lt} {v} to i8*");
                    rv = f"%t{ct}"; rlt = "i8*";
                }
                if (ir_ret_type == "i32" || ir_ret_type == "i64") && (lt == "i8*" || lt.ends("*")) {
                    ir_emiti(f"%t{ct} = ptrtoint {lt} {v} to {ir_ret_type}");
                    rv = f"%t{ct}"; rlt = ir_ret_type;
                }
                if ir_ret_type == "i1" && lt == "i32" {
                    ir_emiti(f"%t{ct} = trunc i32 {v} to i1");
                    rv = f"%t{ct}"; rlt = "i1";
                }
                if ir_ret_type == "i32" && lt == "i1" {
                    ir_emiti(f"%t{ct} = zext i1 {v} to i32");
                    rv = f"%t{ct}"; rlt = "i32";
                }
            }
            ir_emiti(f"ret {rlt} {rv}");
        }
        let dl: I32 = ir_label_id();
        ir_emit(f"dead_{dl}:");
        return;
    }

    if k == NK_IF {
        let cond_vt: Str = ir_gen_expr(node_a[idx]);
        let cond_v: Str  = ir_get_v(cond_vt);
        let cond_lt: Str = ir_get_t(cond_vt);
        let lbl: I32     = ir_label_id();
        let then_l: Str  = f"then_{lbl}";
        let else_l: Str  = f"else_{lbl}";
        let end_l: Str   = f"end_{lbl}";
        // Coerce to i1 if needed
        let mut cond_i1: Str = cond_v;
        if cond_lt != "i1" {
            let ct: I32 = ir_tmp_id();
            if cond_lt == "i8*" || cond_lt.ends("*") {
                ir_emiti(f"%t{ct} = icmp ne {cond_lt} {cond_v}, null");
            } else {
                ir_emiti(f"%t{ct} = icmp ne {cond_lt} {cond_v}, 0");
            }
            cond_i1 = f"%t{ct}";
        }
        if node_c[idx] != 0 {
            ir_emiti(f"br i1 {cond_i1}, label %{then_l}, label %{else_l}");
        } else {
            ir_emiti(f"br i1 {cond_i1}, label %{then_l}, label %{end_l}");
        }
        ir_emit(f"{then_l}:");
        irv_push_scope();
        ir_gen_stmt(node_b[idx]);
        irv_pop_scope();
        ir_emiti(f"br label %{end_l}");
        if node_c[idx] != 0 {
            ir_emit(f"{else_l}:");
            irv_push_scope();
            ir_gen_stmt(node_c[idx]);
            irv_pop_scope();
            ir_emiti(f"br label %{end_l}");
        }
        ir_emit(f"{end_l}:");
        return;
    }

    if k == NK_WHILE {
        let lbl: I32    = ir_label_id();
        let cond_l: Str = f"cond_{lbl}";
        let body_l: Str = f"body_{lbl}";
        let end_l: Str  = f"endw_{lbl}";
        loop_push(cond_l, end_l);
        ir_emiti(f"br label %{cond_l}");
        ir_emit(f"{cond_l}:");
        let cvt: Str = ir_gen_expr(node_a[idx]);
        let cv: Str  = ir_get_v(cvt);
        let clt: Str = ir_get_t(cvt);
        let mut ci1: Str = cv;
        if clt != "i1" {
            let ct: I32 = ir_tmp_id();
            if clt == "i8*" || clt.ends("*") {
                ir_emiti(f"%t{ct} = icmp ne {clt} {cv}, null");
            } else {
                ir_emiti(f"%t{ct} = icmp ne {clt} {cv}, 0");
            }
            ci1 = f"%t{ct}";
        }
        ir_emiti(f"br i1 {ci1}, label %{body_l}, label %{end_l}");
        ir_emit(f"{body_l}:");
        irv_push_scope();
        ir_gen_stmt(node_b[idx]);
        irv_pop_scope();
        ir_emiti(f"br label %{cond_l}");
        loop_pop();
        ir_emit(f"{end_l}:");
        return;
    }

    if k == NK_LOOP {
        let lbl: I32    = ir_label_id();
        let body_l: Str = f"loop_{lbl}";
        let end_l: Str  = f"endl_{lbl}";
        loop_push(body_l, end_l);
        ir_emiti(f"br label %{body_l}");
        ir_emit(f"{body_l}:");
        irv_push_scope();
        ir_gen_stmt(node_a[idx]);
        irv_pop_scope();
        ir_emiti(f"br label %{body_l}");
        loop_pop();
        ir_emit(f"{end_l}:");
        return;
    }

    if k == NK_BREAK    { ir_emiti(f"br label %{loop_end_top()}");  return; }
    if k == NK_CONTINUE { ir_emiti(f"br label %{loop_cond_top()}"); return; }

    if k == NK_BLOCK {
        irv_push_scope();
        let mut s: I32 = node_a[idx];
        while s != 0 {
            ir_gen_stmt(node_a[s]);
            s = node_b[s];
        }
        irv_pop_scope();
        return;
    }

    // Expression statement
    ir_gen_expr(idx);
}


// Pre-scan AST to emit all allocas in entry block (fixes dominance issues)
func ir_pre_alloca(idx: I32) {
    if idx == 0 { return; }
    let k: I32 = node_kinds[idx];
    if k == NK_LET || k == NK_CONST_D {
        let name: Str = node_str[idx];
        let nt: Str   = node_types[idx];
        let lt: Str   = ir_type(nt);
        let uid: I32  = ir_alloca;
        ir_alloca = ir_alloca + 1;
        let reg: Str  = f"%{name}.{uid}";
        ir_emiti(f"{reg} = alloca {lt}");
        irv_def(name, reg, lt);
        return;
    }
    if k == NK_BLOCK {
        let mut s: I32 = node_a[idx];
        while s != 0 {
            ir_pre_alloca(node_a[s]);
            s = node_b[s];
        }
        return;
    }
    if k == NK_IF {
        ir_pre_alloca(node_b[idx]);
        if node_c[idx] != 0 { ir_pre_alloca(node_c[idx]); }
        return;
    }
    if k == NK_WHILE || k == NK_LOOP || k == NK_FOR_IN {
        ir_pre_alloca(node_b[idx]);
        return;
    }
}

func ir_gen_func(idx: I32) {
    irv_reset();
    // Reset loop stack per function
    loop_cond_labels.clear(); loop_cond_labels.push("");
    loop_end_labels.clear();  loop_end_labels.push("");
    loop_depth = 0;
    let name: Str = func_name(node_str[idx]);
    // Build param list
    let mut params: Str = "";
    let mut pm: I32 = node_a[idx];
    let mut first: Bool = true;
    while pm != 0 {
        let pn: I32  = node_a[pm];
        let ps: Str  = node_str[pn];
        // Parse "name:type"
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
            ci = ci + 1;
        }
        let plt: Str = ir_type(ptype);
        if !first { params = f"{params}, "; }
        params = f"{params}{plt} %{pname}.arg";
        first = false;
        pm = node_b[pm];
    }
    // Return type from fn_rettypes
    let ret: Str   = tc_fn_ret(name);
    let retlt: Str = ir_type(ret);
    if name == "main" {
        ir_emit(f"define i32 @main({params}) " + "{");
    } else {
        ir_emit(f"define {retlt} @{name}({params}) " + "{");
    }
    // Alloca for params
    pm = node_a[idx];
    while pm != 0 {
        let pn: I32  = node_a[pm];
        let ps: Str  = node_str[pn];
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
            ci = ci + 1;
        }
        let plt: Str = ir_type(ptype);
        let puid: I32 = ir_alloca;
        ir_alloca = ir_alloca + 1;
        ir_emiti(f"%{pname}.{puid} = alloca {plt}");
        ir_emiti(f"store {plt} %{pname}.arg, {plt}* %{pname}.{puid}");
        irv_def(pname, f"%{pname}.{puid}", plt);
        pm = node_b[pm];
    }
    ir_ret_type = retlt;
    // Call global initializer in main to set up global arrays
    if name == "main" && gvar_array_count > 0 {
        ir_emiti("call void @__ks_global_init()");
    }
    // Pre-scan: emit all local allocas in entry block
    ir_pre_alloca(node_b[idx]);
    ir_gen_stmt(node_b[idx]);
    // Default return (catches functions without explicit ret)
    if retlt == "void"  { ir_emiti("ret void"); }
    if retlt == "i32"   { ir_emiti("ret i32 0"); }
    if retlt == "i1"    { ir_emiti("ret i1 false"); }
    if retlt == "i8*"   { ir_emiti("ret i8* null"); }
    if retlt == "i64"   { ir_emiti("ret i64 0"); }
    ir_emit("}");
    ir_emit("");
}

// Escape string for LLVM IR c"..." syntax
// Uses hex codes for chars that break the c"..." syntax
func ir_escape_str(s: Str) -> Str {
    let mut out: Str = "";
    let mut i: I32 = 0;
    while i < s.len() {
        let c: Str = s.substr(i, 1);
        if c == "\"" { out = f"{out}\\22"; }
        else {
            if c == "\\" { out = f"{out}\\5C"; }
            else {
                if c == "\n" { out = f"{out}\\0A"; }
                else {
                    if c == "\t" { out = f"{out}\\09"; }
                    else { out = f"{out}{c}"; }
                }
            }
        }
        i = i + 1;
    }
    return out;
}


func irgen(prog_idx: I32) {
    ir_reset();

    // Emit header
    ir_emit("; KonScript self-hosted IR");
    ir_emit("target datalayout = \"e-m:e-i64:64-f80:128-n8:16:32:64-S128\"");
    ir_emit("target triple = \"x86_64-pc-linux-gnu\"");
    ir_emit("");
    ir_emit("; runtime declarations");
    ir_emit("declare i32 @printf(i8* nocapture readonly, ...)");
    ir_emit("declare i8* @malloc(i64)");
    ir_emit("declare void @free(i8*)");
    ir_emit("declare i32 @strlen(i8*)");
    ir_emit("declare i32 @Print(...)");
    ir_emit("declare i8* @_ks_str_concat(i8*, i8*)");
    ir_emit("declare i32 @_ks_result_ok(i8*)");
    ir_emit("declare i8* @_ks_result_value(i8*)");
    ir_emit("declare i8* @_ks_result_error(i8*)");
    ir_emit("declare i8* @_ks_int_to_str(i32)");
    ir_emit("declare i32 @_ks_str_len(i8*)");
    ir_emit("declare i32 @_ks_str_compare(i8*, i8*)");
    ir_emit("declare i1 @_ks_str_ends(i8*, i8*)");
    ir_emit("declare i1 @_ks_str_starts(i8*, i8*)");
    ir_emit("declare i1 @_ks_str_contains(i8*, i8*)");
    ir_emit("declare i8* @_ks_str_trim(i8*)");
    ir_emit("declare i8* @_ks_str_substr(i8*, i32, i32)");
    ir_emit("declare i8* @_ks_str_replace(i8*, i8*, i8*)");
    ir_emit("declare i8* @_ks_file_read(i8*)");
    ir_emit("declare i8* @_ks_file_write(i8*, i8*)");
    ir_emit("declare i32 @_ks_file_exists(i8*)");
    ir_emit("declare i8* @_ks_file_delete(i8*)");
    ir_emit("declare i8* @_ks_file_append(i8*, i8*)");
    // Array runtime
    ir_emit("declare i8* @_ks_array_new(i32)");
    ir_emit("declare void @_ks_array_push(i8*, i8*)");
    ir_emit("declare i8* @_ks_array_get(i8*, i32)");
    ir_emit("declare i32 @_ks_array_len(i8*)");
    ir_emit("declare void @_ks_array_clear(i8*)");
    ir_emit("declare i8* @_ks_array_pop(i8*)");
    ir_emit("declare i32 @_ks_array_has(i8*, i8*)");
    ir_emit("declare i8* @_ks_array_clone(i8*)");
    ir_emit("declare void @_ks_array_free(i8*)");
    // String extras
    ir_emit("declare i8* @_ks_str_split(i8*, i8*)");
    ir_emit("declare i32 @_ks_str_toInt(i8*)");
    ir_emit("declare float @_ks_str_toFloat(i8*)");
    ir_emit("declare i8* @_ks_str_charAt(i8*, i32)");
    ir_emit("declare i32 @_ks_str_toCharCode(i8*)");
    ir_emit("declare i8* @_ks_str_fromCharCode(i32)");
    ir_emit("declare i32 @_ks_str_isAlpha(i8*)");
    ir_emit("declare i32 @_ks_str_isDigit(i8*)");
    ir_emit("declare i8* @_ks_str_upper(i8*)");
    ir_emit("declare i8* @_ks_str_lower(i8*)");
    ir_emit("declare i32 @_ks_str_isEmpty(i8*)");
    // HashMap runtime
    ir_emit("declare i8* @_ks_hashmap_new()");
    ir_emit("declare void @_ks_hashmap_set(i8*, i8*, i8*)");
    ir_emit("declare i8* @_ks_hashmap_get(i8*, i8*)");
    ir_emit("declare i32 @_ks_hashmap_has(i8*, i8*)");
    ir_emit("declare i32 @_ks_hashmap_len(i8*)");
    // Closure runtime
    ir_emit("declare i8* @_ks_closure_new(i8*, i8*)");
    ir_emit("declare i8* @_ks_closure_fn(i8*)");
    ir_emit("declare i8* @_ks_closure_env(i8*)");
    ir_emit("declare void @_ks_closure_free(i8*)");
    // Shell execution
    ir_emit("declare i32 @_ks_system(i8*)");
    // Pre-register return types for self-referential functions
    tc_def_fn("ir_escape_str", "Str");
    tc_def_fn("ir_get_v", "Str");
    tc_def_fn("ir_get_t", "Str");
    tc_def_fn("ir_val", "Str");
    tc_def_fn("func_name", "Str");
    tc_def_fn("func_ret", "Str");
    tc_def_fn("ir_type", "Str");
    tc_def_fn("tc_fn_ret", "Str");
    tc_def_fn("loop_cond_top", "Str");
    tc_def_fn("loop_end_top", "Str");
    tc_def_fn("ir_to_string", "Str");
    tc_def_fn("ir_arglist_append", "Str");
    tc_def_fn("ir_arg_str", "Str");
    tc_def_fn("ir_arg_str", "Str");
    tc_def_fn("ir_tmp_id", "I32");
    tc_def_fn("ir_label_id", "I32");
    tc_def_fn("ir_str_const", "I32");
    // void functions
    tc_def_fn("ir_gen_func", "void");
    tc_def_fn("ir_gen_stmt", "void");
    tc_def_fn("ir_emit", "void");
    tc_def_fn("ir_emiti", "void");
    tc_def_fn("ir_reset", "void");
    tc_def_fn("irv_reset", "void");
    tc_def_fn("irv_push_scope", "void");
    tc_def_fn("irv_pop_scope", "void");
    tc_def_fn("irv_def", "void");
    tc_def_fn("loop_push", "void");
    tc_def_fn("loop_pop", "void");
    tc_def_fn("gconst_define", "void");
    tc_def_fn("tc_def_fn", "void");
    tc_def_fn("tc_define", "void");
    tc_def_fn("tc_push_scope", "void");
    tc_def_fn("tc_pop_scope", "void");
    tc_def_fn("reset_tc", "void");
    tc_def_fn("typecheck", "void");
    tc_def_fn("irgen", "void");
    // Str-returning lookup functions
    tc_def_fn("irv_lookup_reg", "Str");
    tc_def_fn("irv_lookup_type", "Str");
    tc_def_fn("tc_lookup", "Str");
    tc_def_fn("gconst_lookup", "Str");
    ir_emit("");

    // First pass: register global constants and function sigs
    let mut decl: I32 = node_a[prog_idx];
    while decl != 0 {
        let d: I32 = node_a[decl];
        if node_kinds[d] == NK_CONST_D {
            // Evaluate integer constants for the const table
            let init: I32 = node_a[d];
            if init != 0 && node_kinds[init] == NK_INT {
                gconst_define(node_str[d], node_str[init]);
            }
            if init != 0 && node_kinds[init] == NK_BOOL {
                if node_str[init] == "true"  { gconst_define(node_str[d], "1"); }
                if node_str[init] == "false" { gconst_define(node_str[d], "0"); }
            }
        }
        if node_kinds[d] == NK_LET {
            // Emit LLVM global variable - infer type from initializer
            let gname: Str = node_str[d];
            let ginit: I32 = node_a[d];
            let mut gtype: Str = "i8*";
            if ginit != 0 {
                let ik: I32 = node_kinds[ginit];
                if ik == NK_INT  { gtype = "i32"; }
                if ik == NK_BOOL { gtype = "i1"; }
                if ik == NK_FLOAT { gtype = "float"; }
            }
            if gtype == "i32" || gtype == "i1" {
                ir_emit(f"@{gname} = global i32 0");
                gvar_define(gname, f"@{gname}", "i32");
            } else {
                ir_emit(f"@{gname} = global i8* null");
                gvar_define(gname, f"@{gname}", "i8*");
                gvar_mark_array(gname);
            }
        }
        decl = node_b[decl];
    }

    // Emit __ks_global_init to initialize global arrays
    if gvar_array_count > 0 {
        ir_emit("define void @__ks_global_init() {");
        ir_emit("entry:");
        let mut gi: I32 = 1;
        while gi <= gvar_array_count {
            let aname: Str = gvar_array_names[gi];
            let t: I32 = ir_tmp_id();
            ir_emiti(f"%t{t} = call i8* @_ks_array_new(i32 8)");
            ir_emiti(f"store i8* %t{t}, i8** @{aname}");
            gi = gi + 1;
        }
        ir_emiti("ret void");
        ir_emit("}");
        ir_emit("");
    }

    // Second pass: emit functions
    decl = node_a[prog_idx];
    while decl != 0 {
        let d: I32 = node_a[decl];
        if node_kinds[d] == NK_FUNC {
            ir_gen_func(d);
        }
        decl = node_b[decl];
    }

    // Emit global string constants
    let mut si: I32 = 1;
    while si <= gstr_count {
        let sv: Str = gstr_vals[si];
        let sl: I32 = sv.len() + 1;
        let escaped: Str = ir_escape_str(sv);
        ir_emit(f"@str.{si} = private unnamed_addr constant [{sl} x i8] c\"{escaped}\\00\"");
        si = si + 1;
    }
}


func ir_to_string() -> Str {
    // Build output using string concat (not f-strings) to avoid buffer limits
    let mut out: Str = "";
    let mut i: I32 = 1;
    while i < ir_lines.len() {
        out = out + ir_lines[i] + "\n";
        i = i + 1;
    }
    return out;
}

// Write IR directly to file line-by-line (avoids giant string)
func ir_write_to_file(path: Str) -> Bool {
    // Write first line
    let first_result: Result<Str> = File.write(path, "");
    if !first_result.ok { return false; }
    let mut i: I32 = 1;
    while i < ir_lines.len() {
        let line: Str = ir_lines[i] + "\n";
        let r: Result<Str> = File.append(path, line);
        if !r.ok { return false; }
        i = i + 1;
    }
    return true;
}

func main() -> I32 {
    Print("KonScript self-hosted compiler v0.1 — stage 1 (full pipeline)");
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

    Print("Lexing...");
    let ntoks: I32 = lex(src);
    Print("Tokens:   ", ntoks);

    Print("Parsing...");
    let prog: I32 = parse(ntoks);
    Print("Nodes:    ", node_kinds.len());

    Print("Typechecking...");
    typecheck(prog);
    Print("Functions:", fn_count);

    Print("Generating IR...");
    irgen(prog);

    let out_path: Str = "/tmp/konscript_out.ll";

    // Write IR line-by-line to avoid f-string buffer limits
    let wrote_ok: Bool = ir_write_to_file(out_path);
    if !wrote_ok {
        Print("error: cannot write IR to ", out_path);
        return 1;
    }

    Print("IR lines: ", ir_lines.len());
    Print("Written:  ", out_path);

    // ── Compile IR to native binary ─────────────────────────────────────
    let obj_path: Str = "/tmp/konscript_out.o";
    let rt_obj: Str   = "/tmp/_ks_runtime.o";
    let bin_path: Str = "/tmp/konscript_output";

    // Step 1: llc — compile .ll to .o
    Print("Compiling IR...");
    let llc_cmd: Str = "llc -filetype=obj -relocation-model=pic -o " + obj_path + " " + out_path;
    let llc_ret: I32 = _ks_system(llc_cmd);
    if llc_ret != 0 {
        Print("error: llc failed (exit ", llc_ret, ")");
        return 1;
    }

    // Step 2: compile runtime
    Print("Compiling runtime...");
    let rt_cmd: Str = "clang -c -fPIC _ks_runtime.c -o " + rt_obj;
    let rt_ret: I32 = _ks_system(rt_cmd);
    if rt_ret != 0 {
        Print("error: runtime compilation failed (exit ", rt_ret, ")");
        return 1;
    }

    // Step 3: link
    Print("Linking...");
    let link_cmd: Str = "clang " + obj_path + " " + rt_obj + " -o " + bin_path + " -lm";
    let link_ret: I32 = _ks_system(link_cmd);
    if link_ret != 0 {
        Print("error: linking failed (exit ", link_ret, ")");
        return 1;
    }

    Print("");
    Print("  ✓ ", bin_path);
    Print("");
    return 0;
}

