// ---------------------------------------------------------------------------
// konscript.ks — the KonScript compiler, written in KonScript
//
// Self-hosting bootstrap:
//   Stage 0 — C++ bootstrap compiler (src/main.cpp)
//   Stage 1 — compiled by Stage 0 via LLVM IR (no CLI, uses .ks_input)
//   Stage 2 — compiled by Stage 1 via C++ codegen (has CLI args)
//   Stage 3 — compiled by Stage 2 (self-hosted, verified)
//   Stage 4 — compiled by Stage 3 (byte-identical to Stage 3)
//
//   The installed binary is Stage 4. Stages 2+ produce identical binaries.
//
// Implemented:
//   [x] Token constants (TK_*)
//   [x] Lexer (lex)
//   [x] Parser (parse, parallel arrays)
//   [x] Typechecker (typecheck, scope-based inference)
//   [x] C++ Codegen (cg_generate, cg_write_to_file)
//   [x] CLI flags (-o, -I, -L, -l, --cpp, --no-stdlib, --help)
//   [x] Engine game compilation (node/scene/lifecycle)
//   [x] FFI support (extern "C", inline asm)
//   [x] Self-hosting (4-stage bootstrap, byte-identical)
//   [x] Progress bars (pure KonScript, ANSI/Unicode)
//   [x] Timing (_ks_time_ms, format_ms)
//
// Build:
//   ./build.sh          # 5-stage bootstrap
//   sudo ./install.sh   # install Stage 4 system-wide
//
// Self-compile:
//   konscript konscript.ks -o konscript2   # produces identical binary
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
const TK_EXTERN:     I32 = 29;
const TK_ASM:        I32 = 30;

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
const TK_AMP:       I32 = 77;

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
    if w == "extern"   { return TK_EXTERN; }
    if w == "asm"      { return TK_ASM; }
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
        if c == " " || c == "\t" {
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
                            if fesc == "n"  { val = val + "\n"; i += 2; col += 2; }
                            else {
                                if fesc == "t"  { val = val + "\t"; i += 2; col += 2; }
                                else {
                                    val = val + fesc;
                                    i += 2; col += 2;
                                }
                            }
                        } else {
                            if fsc == "\n" { line = line + 1; col = 1; val = val + "\n"; i += 1; }
                            else { val = val + fsc; i += 1; col += 1; }
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
                    if esc == "n"  { val = val + "\n"; i += 2; col += 2; }
                    else {
                        if esc == "t"  { val = val + "\t"; i += 2; col += 2; }
                        else {
                            if esc == "\\" { val = val + "\\"; i += 2; col += 2; }
                            else {
                                if esc == "\"" { val = val + "\""; i += 2; col += 2; }
                                else {
                                    val = val + esc;
                                    i += 2; col += 2;
                                }
                            }
                        }
                    }
                } else {
                    if sc == "\n" { line = line + 1; col = 1; }
                    val = val + sc;
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
                num = num + src.substr(i, 1);
                i += 1; col += 1;
            }
            // decimal part
            if i < n && src.substr(i, 1) == "." && i + 1 < n && is_digit(src.substr(i + 1, 1)) {
                is_float = true;
                num = num + ".";
                i += 1; col += 1;
                while i < n && is_digit(src.substr(i, 1)) {
                    num = num + src.substr(i, 1);
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
                word = word + src.substr(i, 1);
                i += 1; col += 1;
            }
            let kind: I32 = keyword_kind(word);
            emit_token(kind, word, line, tok_col);
            continue;
        }

        // ── # comment (legacy) — skip to end of line ────────────────
        if c == "#" && !(i + 7 < n && src.substr(i, 8) == "#include") {
            while i < n && src.substr(i, 1) != "\n" { i += 1; col += 1; }
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
                    path = path + src.substr(i, 1);
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
        if c == "&" { emit_token(TK_AMP,       "&", line, col); i += 1; col += 1; continue; }

        // ── Unknown character — report and skip ────────────────────
        lex_error(line, col, "unexpected character '" + c + "'");
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
const NK_EXTERN:    I32 = 32;
const NK_ASM:       I32 = 33;
const NK_UNION_D:   I32 = 34;
const NK_REF:       I32 = 35;
const NK_REF_MUT:   I32 = 36;
const NK_DEREF:     I32 = 37;
const NK_FUNC_EXPR: I32 = 38;
const NK_NODE:      I32 = 39;  // str="Name|Base", a=fields/methods list, b=body
const NK_TERNARY:   I32 = 41;  // a=condition, b=trueVal, c=falseVal

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
func pk_at(offset: I32) -> I32 { return tok_kinds[pos + offset]; }
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
        parse_error(pk_ln(), pk_col(), "expected " + msg + ", got '" + pk_val() + "'");
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
        let mut t: Str = "[" + inner + "]";
        if mat(TK_QUESTION) { t = t + "?"; }
        return t;
    }
    // Tuple type: (A, B, ...)
    if chk(TK_LPAREN) {
        adv();
        let mut t: Str = "(";
        let mut first: Bool = true;
        while !chk(TK_RPAREN) && pk() != TK_EOF {
            if !first { eat(TK_COMMA, "','"); t = t + ","; }
            t = t + parse_type();
            first = false;
        }
        eat(TK_RPAREN, "')'");
        if mat(TK_QUESTION) { t = t + "?"; }
        return t + ")";
    }
    // Named type (possibly generic: Result<T>, HashMap<K,V>)
    let name: Str = eat(TK_IDENT, "type name");
    let mut t: Str = name;
    if mat(TK_LT) {
        t = t + "<";
        let mut first: Bool = true;
        while !chk(TK_GT) && pk() != TK_EOF {
            if !first { eat(TK_COMMA, "','"); t = t + ","; }
            t = t + parse_type();
            first = false;
        }
        eat(TK_GT, "'>'");
        t = t + ">";
    }
    if mat(TK_QUESTION) { t = t + "?"; }
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

    // C++-style lambda: []() { } or [&]() { }
    // Check if [ is followed by ] or &]
    if chk(TK_LBRACKET) {
        let mut is_lambda: Bool = false;
        if pk_at(1) == TK_RBRACKET { is_lambda = true; }
        if pk_at(1) == TK_AMP && pk_at(2) == TK_RBRACKET { is_lambda = true; }

        if is_lambda {
            adv();  // consume [
            if chk(TK_AMP) { adv(); }  // consume & if present
            eat(TK_RBRACKET, "']'");
            // Parse like func() { } closure
            eat(TK_LPAREN, "'('");
            let mut params: Str = "";
            let mut pcnt: I32 = 0;
            while !chk(TK_RPAREN) && pk() != TK_EOF {
                if pcnt > 0 { eat(TK_COMMA, "','"); }
                let pname: Str = eat(TK_IDENT, "param name");
                eat(TK_COLON, "':'");
                let ptype: Str = parse_type();
                if pcnt > 0 { params = params + ","; }
                params = params + pname + ":" + ptype;
                pcnt = pcnt + 1;
            }
            eat(TK_RPAREN, "')'");
            let mut ret_type: Str = "";
            if mat(TK_ARROW) { ret_type = parse_type(); }
            let body: I32 = parse_block();
            let pnode: I32 = alloc_node(NK_PARAM, 0, 0, 0, params);
            return alloc_node(NK_FUNC_EXPR, pnode, body, 0, ret_type);
        }
    }

    // Array literal [a, b, c]
    if mat(TK_LBRACKET) {
        let mut head: I32 = 0;
        let mut tail: I32 = 0;
        let mut cnt: I32 = 0;
        while !chk(TK_RBRACKET) && pk() != TK_EOF {
            if cnt > 0 { eat(TK_COMMA, "','"); }
            let elem: I32 = parse_ternary();
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
        let inner: I32 = parse_ternary();
        eat(TK_RPAREN, "')'");
        return inner;
    }

    parse_error(ln, cl, "unexpected token '" + pk_val() + "' in expression");
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
                let arg: I32 = parse_ternary();
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
            let idx: I32 = parse_ternary();
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

func parse_ternary() -> I32 {
    let mut cond: I32 = parse_or();
    if chk(TK_QUESTION) {
        adv();
        let true_val: I32 = parse_or();
        eat(TK_COLON, "':'");
        let false_val: I32 = parse_ternary();
        cond = alloc_node(NK_TERNARY, cond, true_val, false_val, "");
    }
    return cond;
}

func parse_expr() -> I32 {
    let left: I32 = parse_ternary();
    // Assignment operators
    if chk(TK_EQ) || chk(TK_PLUSEQ) || chk(TK_MINUSEQ) || chk(TK_STAREQ) || chk(TK_SLASHEQ) {
        let op: Str = pk_val(); adv();
        let right: I32 = parse_ternary();
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
        // Optional type annotation — store as "name:Type" in node_str
        let mut full_name: Str = name;
        if mat(TK_COLON) {
            let type_ann: Str = parse_type();
            full_name = name + ":" + type_ann;
        }
        let mut init: I32 = 0;
        if mat(TK_EQ) { init = parse_expr(); }
        mat(TK_SEMICOLON);
        return alloc_node(NK_LET, init, is_mut, 0, full_name);
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

    // asm statement
    if chk(TK_ASM) { adv(); return parse_asm_stmt(); }

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
        let pnode: I32 = alloc_node(NK_PARAM, 0, 0, 0, pname + ":" + ptype);
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
    return alloc_node(NK_FUNC, phead, body, pcnt, name + "|" + ret_type_str);
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
        let fn2: I32 = alloc_node(NK_FIELD, 0, 0, 0, fname + ":" + ftype);
        let ln2: I32 = list_node(fn2, 0);
        if fhead == 0 { fhead = ln2; ftail = ln2; }
        else { node_b[ftail] = ln2; ftail = ln2; }
        fcnt = fcnt + 1;
    }
    eat(TK_RBRACE, "'}'");
    return alloc_node(NK_STRUCT_D, fhead, fcnt, 0, name);
}

// node Name : Base { fields, methods }
func parse_node() -> I32 {
    eat(TK_NODE, "'node'");
    let name: Str = eat(TK_IDENT, "node name");
    // Optional base type
    let mut base: Str = "Node2D";
    if mat(TK_COLON) {
        base = eat(TK_IDENT, "base type");
    }
    eat(TK_LBRACE, "'{'");
    // Parse members: fields (let) and methods (func)
    let mut mhead: I32 = 0;
    let mut mtail: I32 = 0;
    let mut mcnt: I32 = 0;
    while !chk(TK_RBRACE) && pk() != TK_EOF {
        let mut is_pub: Bool = false;
        if chk(TK_PUB) { adv(); is_pub = true; }
        let mut member: I32 = 0;
        if chk(TK_FUNC) {
            member = parse_func();
        }
        if chk(TK_LET) {
            member = parse_stmt();
        }
        if chk(TK_CONST) {
            member = parse_stmt();
        }
        if member != 0 {
            let ln: I32 = list_node(member, 0);
            if mhead == 0 { mhead = ln; mtail = ln; }
            else { node_b[mtail] = ln; mtail = ln; }
            mcnt = mcnt + 1;
        }
    }
    eat(TK_RBRACE, "'}'");
    return alloc_node(NK_NODE, mhead, mcnt, 0, name + "|" + base);
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
        // Check for ... (parsed as TK_DOTDOT + TK_DOT, or three TK_DOTs)
        if chk(TK_DOTDOT) {
            adv(); // consume ..
            if chk(TK_DOT) { adv(); } // consume third .
            is_variadic = true;
            break;
        }
        if chk(TK_DOT) && pk_val() == "." {
            adv(); adv(); adv(); // consume . . .
            is_variadic = true;
            break;
        }
        let pname: Str = eat(TK_IDENT, "parameter");
        let mut ptype: Str = pname;
        if mat(TK_COLON) {
            ptype = parse_type();
        }
        if !first { params = params + "," + ptype; }
        else { params = ptype; }
        first = false;
    }
    eat(TK_RPAREN, "')'");
    let mut ret: Str = "void";
    if mat(TK_ARROW) { ret = parse_type(); }
    mat(TK_SEMICOLON);
    let mut variadic_str: Str = "";
    if is_variadic { variadic_str = "..."; }
    return alloc_node(NK_EXTERN, 0, 0, 0, name + "|" + linkage + "|" + ret + "|" + params + "|" + variadic_str);
}

// asm("template" : outputs : inputs : clobbers);
func parse_asm_stmt() -> I32 {
    // Already consumed 'asm'
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
    return alloc_node(NK_ASM, 0, 0, 0, tmpl + "|" + outputs + "|" + inputs + "|" + clobbers);
}

func parse_top_level() -> I32 {
    // Skip pub
    if chk(TK_PUB) { adv(); }
    if chk(TK_FUNC)   { return parse_func(); }
    if chk(TK_STRUCT) { return parse_struct(); }
    if chk(TK_NODE)   { return parse_node(); }
    if chk(TK_CONST)  { return parse_stmt(); }
    if chk(TK_LET)    { return parse_stmt(); }
    if chk(TK_EXTERN) { adv(); return parse_extern(); }
    if chk(TK_INCLUDE) {
        let path: Str = pk_val(); adv();
        return alloc_node(NK_INCLUDE_D, 0, 0, 0, path);
    }
    // Unknown — skip
    parse_error(pk_ln(), pk_col(), "unexpected token '" + pk_val() + "' at top level");
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
    if k == NK_TERNARY   { return "ternary"; }
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
    while i < indent { pad = pad + "  "; i = i + 1; }

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
    if k == NK_TERNARY {
        tc_expr(node_a[idx]);
        let tt: Str = tc_expr(node_b[idx]);
        tc_expr(node_c[idx]);
        node_types[idx] = tt; return tt;
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
            // File methods return Result<Str>
            if method == "read" || method == "write" || method == "append" ||
               method == "delete" { ret = "Result<Str>"; }
            if method == "exists" { ret = "Bool"; }
            if method == "lines" { ret = "[Str]"; }
            // HashMap methods
            if method == "get" { ret = "?"; }
            if method == "set" { ret = "void"; }
            if method == "remove" { ret = "void"; }
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
        let t: Str = "[" + elem_t + "]";
        node_types[idx] = t; return t;
    }
    return "?";
}

func tc_stmt(idx: I32) {
    if idx == 0 { return; }
    let k: I32 = node_kinds[idx];
    if k == NK_LET {
        let mut t: Str = tc_expr(node_a[idx]);
        // If explicit type annotation exists and inferred type is unknown, use annotation
        let ann: Str = get_type_ann(node_str[idx]);
        if ann.len() > 0 && (t == "?" || t == "") { t = ann; }
        tc_define(strip_type_ann(node_str[idx]), t);
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
        let mut elem_t: Str = "I32";
        if iter_t.len() > 2 && iter_t.starts("[") {
            elem_t = iter_t.substr(1, iter_t.len() - 2);
        }
        // Range expressions iterate over I32
        if iter_t == "?" { elem_t = "I32"; }
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
        if node_kinds[d] == NK_CONST_D { let t: Str = tc_expr(node_a[d]); tc_define(strip_type_ann(node_str[d]), t); }
        if node_kinds[d] == NK_LET     { let t: Str = tc_expr(node_a[d]); tc_define(strip_type_ann(node_str[d]), t); }
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
    // Parser functions
    tc_def_fn("eat", "Str");
    tc_def_fn("pk_val", "Str");
    tc_def_fn("parse_type", "Str");
    tc_def_fn("parse_top_level", "I32");
    tc_def_fn("parse_func", "I32");
    tc_def_fn("parse_struct", "I32");
    tc_def_fn("parse_node", "I32");
    tc_def_fn("parse_extern", "I32");
    tc_def_fn("parse_asm_stmt", "I32");
    tc_def_fn("cg_gen_extern", "void");
    tc_def_fn("cg_gen_asm", "void");
    tc_def_fn("parse_stmt", "I32");
    tc_def_fn("parse_block", "I32");
    tc_def_fn("parse_expr", "I32");
    tc_def_fn("parse_or", "I32");
    tc_def_fn("parse_and", "I32");
    tc_def_fn("parse_eq", "I32");
    tc_def_fn("parse_cmp", "I32");
    tc_def_fn("parse_add", "I32");
    tc_def_fn("parse_mul", "I32");
    tc_def_fn("parse_unary", "I32");
    tc_def_fn("parse_postfix", "I32");
    tc_def_fn("parse_primary", "I32");
    tc_def_fn("parse", "I32");
    tc_def_fn("pk", "I32");
    tc_def_fn("pk_ln", "I32");
    tc_def_fn("pk_col", "I32");
    tc_def_fn("mat", "Bool");
    tc_def_fn("chk", "Bool");
    tc_def_fn("keyword_kind", "I32");
    tc_def_fn("alloc_node", "I32");
    tc_def_fn("list_node", "I32");
    tc_def_fn("adv", "void");
    tc_def_fn("reset_ast", "void");
    tc_def_fn("emit_token", "void");
    tc_def_fn("lex_error", "void");
    tc_def_fn("parse_error", "void");
    tc_def_fn("dump_tokens", "void");
    tc_def_fn("dump_node", "void");
    // Lexer functions
    tc_def_fn("lex", "I32");
    tc_def_fn("is_digit", "Bool");
    tc_def_fn("is_alpha", "Bool");
    tc_def_fn("is_alnum", "Bool");
    tc_def_fn("is_hex", "Bool");
    tc_def_fn("token_kind_name", "Str");
    tc_def_fn("node_kind_name", "Str");
    // IRGen helpers
    tc_def_fn("gvar_define", "void");
    tc_def_fn("gvar_mark_array", "void");
    tc_def_fn("gvar_set_init", "void");
    tc_def_fn("gvar_is_array", "Bool");
    tc_def_fn("gvar_lookup_reg", "Str");
    tc_def_fn("gvar_lookup_type", "Str");
    tc_def_fn("ir_escape_str", "Str");
    tc_def_fn("ir_str_len", "I32");
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
    tc_def_fn("_ks_time_ms", "F64");
    tc_def_fn("_ks_self_dir", "Str");
    tc_def_fn("_ks_argc", "I32");
    tc_def_fn("_ks_get_argv", "Str");
    tc_def_fn("_ks_init_args", "void");
    tc_def_fn("pad_name", "Str");
    tc_def_fn("format_ms", "Str");
    tc_def_fn("stage_doing", "void");
    tc_def_fn("stage_ok", "void");
    tc_def_fn("print_success", "void");
    tc_def_fn("cg_type_is_ptr", "Bool");
    tc_def_fn("cg_register_field", "void");
    tc_def_fn("cg_lookup_field_type", "Str");
    tc_def_fn("strip_type_ann", "Str");
    tc_def_fn("get_type_ann", "Str");
    tc_def_fn("binary_dir", "Str");
    tc_def_fn("cg_emit_fwd_decls", "void");
    tc_def_fn("cg_emit_toplevel", "void");
    tc_def_fn("print_usage", "void");
    tc_def_fn("_ks_int_to_str", "Str");
    // C++ codegen functions
    tc_def_fn("cg_escape_str", "Str");
    tc_def_fn("cg_generate", "Str");
    tc_def_fn("cg_write_to_file", "Bool");
    tc_def_fn("cg_gen_expr", "Str");
    tc_def_fn("cg_type", "Str");
    tc_def_fn("cg_is_node_type", "Bool");
    tc_def_fn("cg_is_ptr", "Bool");
    tc_def_fn("cg_reset", "void");
    tc_def_fn("cg_emit", "void");
    tc_def_fn("cg_emit_raw", "void");
    tc_def_fn("cg_indent_inc", "void");
    tc_def_fn("cg_indent_dec", "void");
    tc_def_fn("cg_mark_ptr", "void");
    tc_def_fn("cg_gen_stmt", "void");
    tc_def_fn("cg_gen_func", "void");
    tc_def_fn("cg_gen_struct", "void");
    tc_def_fn("cg_gen_node", "void");
    tc_def_fn("cg_register_node", "void");
    tc_def_fn("cg_is_engine_node", "Bool");
    tc_def_fn("cg_is_user_node", "Bool");
    tc_def_fn("cg_is_ptr_type", "Bool");
    tc_def_fn("cg_is_engine_val_type", "Bool");
    tc_def_fn("cg_lifecycle_sig", "Str");
    tc_def_fn("cg_engine_func", "Str");
    tc_def_fn("cg_scene_method", "Str");
    tc_def_fn("gvar_set_init", "void");
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
                        if !found { pname = pname + ch; }
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
    if t == "I8"   { return "i8"; }
    if t == "I16"  { return "i16"; }
    if t == "U8"   { return "i8"; }
    if t == "U16"  { return "i16"; }
    if t == "U32"  { return "i32"; }
    if t == "U64"  { return "i64"; }
    if t == "F32"  { return "float"; }
    if t == "F64"  { return "double"; }
    if t == "Bool" { return "i1"; }
    if t == "Str"  { return "i8*"; }
    if t == "void" { return "void"; }
    if t == "?"    { return "i32"; }
    // Array types → opaque pointer
    if t.starts("[") { return "i8*"; }
    // Result, HashMap, etc. → opaque pointer
    if t.starts("Result") { return "i8*"; }
    if t.starts("HashMap") { return "i8*"; }
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
let mut gvar_array_inits: [I32] = [0];  // initializer AST node index
let mut gvar_array_count: I32 = 0;

func gvar_mark_array(name: Str) {
    gvar_array_count = gvar_array_count + 1;
    gvar_array_names.push(name);
    gvar_array_inits.push(0);
}

func gvar_set_init(name: Str, init_node: I32) {
    let mut i: I32 = gvar_array_count;
    while i > 0 {
        if gvar_array_names[i] == name {
            gvar_array_inits[i] = init_node;
            return;
        }
        i = i - 1;
    }
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

    if k == NK_TERNARY {
        // Ternary in LLVM IR: use select or phi — emit as select for simplicity
        let cvt: Str = ir_gen_expr(node_a[idx]);
        let tvt: Str = ir_gen_expr(node_b[idx]);
        let fvt: Str = ir_gen_expr(node_c[idx]);
        let cv: Str  = ir_get_v(cvt);
        let tv: Str  = ir_get_v(tvt);
        let fv: Str  = ir_get_v(fvt);
        let tt: Str  = ir_get_t(tvt);
        let t: I32   = ir_tmp_id();
        ir_emit("  %t" + ToString(t) + " = select i1 " + cv + ", " + tt + " " + tv + ", " + tt + " " + fv);
        return ir_val("%t" + ToString(t), tt);
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
            // Check if target is a KsArray (global array or typed as array)
            let mut is_ks_array: Bool = false;
            if node_kinds[node_a[tgt]] == NK_IDENT {
                let arr_name: Str = node_str[node_a[tgt]];
                if gvar_is_array(arr_name) { is_ks_array = true; }
                if node_types[node_a[tgt]].starts("[") { is_ks_array = true; }
            }
            if is_ks_array {
                // Use _ks_array_set for KsArray runtime arrays
                let mut val_ptr: Str = rv;
                if rlt != "i8*" {
                    let cvt: I32 = ir_tmp_id();
                    ir_emiti(f"%t{cvt} = inttoptr {rlt} {rv} to i8*");
                    val_ptr = f"%t{cvt}";
                }
                // Ensure index is i32
                let mut idx_i32: Str = idx_v;
                if ir_get_t(idx_vt) != "i32" {
                    let cvt2: I32 = ir_tmp_id();
                    ir_emiti(f"%t{cvt2} = trunc i64 {idx_v} to i32");
                    idx_i32 = f"%t{cvt2}";
                }
                ir_emiti(f"call void @_ks_array_set(i8* {ptr_v}, i32 {idx_i32}, i8* {val_ptr})");
            } else {
                // Raw pointer index assignment (for non-KsArray pointers)
                if ptr_v != "0" && ptr_v != "null" {
                    let tp: I32 = ir_tmp_id();
                    let tp2: I32 = ir_tmp_id();
                    let mut ptr_cast: Str = ptr_v;
                    if plt != f"{rlt}*" && plt != rlt {
                        ir_emiti(f"%t{tp} = bitcast {plt} {ptr_v} to {rlt}*");
                        ptr_cast = f"%t{tp}";
                    }
                    ir_emiti(f"%t{tp2} = getelementptr {rlt}, {rlt}* {ptr_cast}, i32 {idx_v}");
                    ir_emiti(f"store {rlt} {rv}, {rlt}* %t{tp2}");
                }
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
                ir_emiti(f"%t{t} = call i8* @_ks_file_write({m_arglist})");
                return ir_val(f"%t{t}", "i8*");
            }
            if method == "exists" {
                ir_emiti(f"%t{t} = call i32 @_ks_file_exists({m_arglist})");
                return ir_val(f"%t{t}", "i32");
            }
            if method == "append" {
                ir_emiti(f"%t{t} = call i8* @_ks_file_append({m_arglist})");
                return ir_val(f"%t{t}", "i8*");
            }
            if method == "delete" {
                ir_emiti(f"%t{t} = call i8* @_ks_file_delete({m_arglist})");
                return ir_val(f"%t{t}", "i8*");
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
            // Zero-initialize
            if lt == "i8*" { ir_emiti(f"store i8* null, i8** {reg}"); }
            if lt == "i32" { ir_emiti(f"store i32 0, i32* {reg}"); }
            if lt == "i1"  { ir_emiti(f"store i1 0, i1* {reg}"); }
            if lt == "float" { ir_emiti(f"store float 0.0, float* {reg}"); }
            if lt == "double" { ir_emiti(f"store double 0.0, double* {reg}"); }
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
        // Zero-initialize to prevent uninitialized memory bugs
        if lt == "i8*" {
            ir_emiti(f"store i8* null, i8** {reg}");
        }
        if lt == "i32" {
            ir_emiti(f"store i32 0, i32* {reg}");
        }
        if lt == "i1" {
            ir_emiti(f"store i1 0, i1* {reg}");
        }
        if lt == "float" {
            ir_emiti(f"store float 0.0, float* {reg}");
        }
        if lt == "double" {
            ir_emiti(f"store double 0.0, double* {reg}");
        }
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
                if !found { pname = pname + ch; }
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
    ir_emit("entry:");
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
                if !found { pname = pname + ch; }
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
    ir_emit("declare void @_ks_array_set(i8*, i32, i8*)");
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
    ir_emit("declare double @_ks_time_ms()");
    ir_emit("declare i32 @_ks_argc()");
    ir_emit("declare i8* @_ks_get_argv(i32)");
    ir_emit("declare void @_ks_init_args(i32, i8**)");
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
                if ginit != 0 { gvar_set_init(gname, ginit); }
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
            // Push initial values from array literal
            let init_node: I32 = gvar_array_inits[gi];
            if init_node != 0 && node_kinds[init_node] == NK_ARRAY_LIT {
                let mut elem: I32 = node_a[init_node];
                while elem != 0 {
                    let ev: I32 = node_a[elem];
                    if node_kinds[ev] == NK_INT {
                        let et: I32 = ir_tmp_id();
                        ir_emiti(f"%t{et} = inttoptr i32 {node_str[ev]} to i8*");
                        let lt: I32 = ir_tmp_id();
                        ir_emiti(f"%t{lt} = load i8*, i8** @{aname}");
                        ir_emiti(f"call void @_ks_array_push(i8* %t{lt}, i8* %t{et})");
                    }
                    if node_kinds[ev] == NK_STR_LIT {
                        let sidx: I32 = ir_str_const(node_str[ev]);
                        let slen: I32 = ir_str_len(sidx);
                        let et: I32 = ir_tmp_id();
                        ir_emiti(f"%t{et} = getelementptr inbounds [{slen} x i8], [{slen} x i8]* @str.{sidx}, i32 0, i32 0");
                        let lt: I32 = ir_tmp_id();
                        ir_emiti(f"%t{lt} = load i8*, i8** @{aname}");
                        ir_emiti(f"call void @_ks_array_push(i8* %t{lt}, i8* %t{et})");
                    }
                    if node_kinds[ev] == NK_BOOL {
                        let mut bv: Str = "0";
                        if node_str[ev] == "true" { bv = "1"; }
                        let et: I32 = ir_tmp_id();
                        ir_emiti(f"%t{et} = inttoptr i32 {bv} to i8*");
                        let lt: I32 = ir_tmp_id();
                        ir_emiti(f"%t{lt} = load i8*, i8** @{aname}");
                        ir_emiti(f"call void @_ks_array_push(i8* %t{lt}, i8* %t{et})");
                    }
                    elem = node_b[elem];
                }
            }
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


// ===== C++ string escaping helper =====
func cg_escape_str(s: Str) -> Str {
    let mut out: Str = "";
    let mut i: I32 = 0;
    while i < s.len() {
        let cc: I32 = s.charAt(i).toCharCode();
        if cc == 10 { out = out + "\\n"; }
        else {
            if cc == 9 { out = out + "\\t"; }
            else {
                if cc == 13 { out = out + "\\r"; }
                else {
                    if cc == 92 { out = out + "\\\\"; }
                    else {
                        if cc == 34 { out = out + "\\\""; }
                        else { out = out + s.substr(i, 1); }
                    }
                }
            }
        }
        i = i + 1;
    }
    return out;
}

// ===== C++ CODEGEN (from ks_codegen.ks) =====

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
let mut included_progs: [I32] = [0];   // AST indices of included .ks programs

// Type of the last expression generated by cg_gen_expr.
// Lets callers know whether the result is a pointer, string, etc.
// for correct accessor (. vs ->) and type propagation through chains.
let mut cg_last_type: Str = "";

// Field type registry: stores "ClassName.fieldName" → type for node/class fields.
// Used to resolve types during chained member access (e.g. outer.col.width).
let mut cg_field_types: [Str] = [""];   // entries: "Class.field=Type"
let mut cg_field_count: I32 = 0;

func cg_register_field(class_name: Str, field_name: Str, field_type: Str) {
    cg_field_count = cg_field_count + 1;
    cg_field_types.push(class_name + "." + field_name + "=" + field_type);
}

func cg_lookup_field_type(class_name: Str, field_name: Str) -> Str {
    let key: Str = class_name + "." + field_name + "=";
    let mut i: I32 = 1;
    while i <= cg_field_count {
        if cg_field_types[i].starts(key) {
            return cg_field_types[i].substr(key.len(), cg_field_types[i].len() - key.len());
        }
        i = i + 1;
    }
    return "";
}

func cg_reset() {
    cg_lines.clear(); cg_lines.push("");
    cg_indent = 0;
    cg_ptr_vars.clear(); cg_ptr_vars.push("");
    cg_ptr_count = 0;
    cg_is_engine = false;
    cg_last_type = "";
    cg_field_types.clear(); cg_field_types.push("");
    cg_field_count = 0;
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

// Strip type annotation from "name:Type" → returns just "name"
func strip_type_ann(s: Str) -> Str {
    let mut i: I32 = 0;
    while i < s.len() {
        if s.substr(i, 1) == ":" { return s.substr(0, i); }
        i = i + 1;
    }
    return s;
}

// Extract type annotation from "name:Type" → returns "Type" or ""
func get_type_ann(s: Str) -> Str {
    let mut i: I32 = 0;
    while i < s.len() {
        if s.substr(i, 1) == ":" { return s.substr(i + 1, s.len() - i - 1); }
        i = i + 1;
    }
    return "";
}

func cg_is_ptr(name: Str) -> Bool {
    let mut i: I32 = 1;
    while i <= cg_ptr_count {
        if cg_ptr_vars[i] == name { return true; }
        i = i + 1;
    }
    return false;
}

// Check if a TYPE string represents a pointer (engine node types are always pointers)
func cg_type_is_ptr(t: Str) -> Bool {
    if cg_is_engine_node(t) || cg_is_user_node(t) { return true; }
    if t.ends("*") { return true; }
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
    if t == "Vec2" { return "Vector2"; }
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
    if t == "CameraNode2D" { return true; }
    return false;
}

// ── Expression codegen ───────────────────────────────────────────────────
func cg_gen_expr(idx: I32) -> Str {
    if idx == 0 { return ""; }
    let k: I32 = node_kinds[idx];

    if k == NK_INT   { cg_last_type = "I32"; return node_str[idx]; }
    if k == NK_FLOAT { cg_last_type = "F64"; return node_str[idx] + "f"; }
    if k == NK_BOOL  {
        cg_last_type = "Bool";
        if node_str[idx] == "true" { return "true"; }
        return "false";
    }
    if k == NK_STR_LIT {
        cg_last_type = "Str";
        return "\"" + cg_escape_str(node_str[idx]) + "\"";
    }
    if k == NK_NULL_LIT { cg_last_type = "ptr"; return "nullptr"; }

    if k == NK_FSTR {
        // F-strings: parse template and emit C++ string concatenation
        let raw: Str = node_str[idx];
        let n_raw: I32 = raw.len();
        let mut i_f: I32 = 0;
        // For C++ codegen: build f-string as std::string concatenation
        let mut result: Str = "std::string()";
        while i_f < n_raw {
            let mut j_f: I32 = i_f;
            while j_f < n_raw && raw.substr(j_f, 1) != "{" {
                j_f = j_f + 1;
            }
            if j_f > i_f {
                let lit: Str = cg_escape_str(raw.substr(i_f, j_f - i_f));
                result = result + " + std::string(\"" + lit + "\")";
            }
            if j_f >= n_raw { i_f = n_raw; }
            else {
                j_f = j_f + 1;
                let mut k_f: I32 = j_f;
                while k_f < n_raw && raw.substr(k_f, 1) != "}" {
                    k_f = k_f + 1;
                }
                let expr_name: Str = raw.substr(j_f, k_f - j_f);
                // Result member access: .ok, .value, .error are methods → need ()
                if expr_name.ends(".ok") || expr_name.ends(".value") || expr_name.ends(".error") {
                    result = result + " + _ks_tostr(" + expr_name + "())";
                } else {
                    result = result + " + _ks_tostr(" + expr_name + ")";
                }
                i_f = k_f + 1;
            }
        }
        return "(" + result + ")";
    }

    if k == NK_IDENT {
        let iname: Str = node_str[idx];
        // Set type from typechecker or pointer tracking
        let nt: Str = node_types[idx];
        if nt.len() > 0 && nt != "?" { cg_last_type = nt; }
        else if cg_is_ptr(iname) { cg_last_type = "ptr"; }
        else { cg_last_type = ""; }
        return iname;
    }

    if k == NK_TERNARY {
        let cond: Str = cg_gen_expr(node_a[idx]);
        let true_val: Str = cg_gen_expr(node_b[idx]);
        let false_val: Str = cg_gen_expr(node_c[idx]);
        return "(" + cond + " ? " + true_val + " : " + false_val + ")";
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
                // Use variadic Print template
                let mut parg: I32 = node_b[idx];
                let mut print_args: Str = "";
                let mut first_p: Bool = true;
                while parg != 0 {
                    let pexpr: Str = cg_gen_expr(node_a[parg]);
                    if !first_p { print_args = print_args + ", "; }
                    print_args = print_args + pexpr;
                    first_p = false;
                    parg = node_b[parg];
                }
                return "Print(" + print_args + ")";
            }
            // ToString
            if fname == "ToString" { return "std::to_string(" + args + ")"; }
            if fname == "Vec2" { cg_last_type = "Vec2"; return "Vector2{" + args + "}"; }
            if fname == "Camera2D" { cg_last_type = "Camera2D"; return "Camera2D{" + args + "}"; }
            if fname == "Scene" { cg_last_type = "Scene"; return "Scene()"; }
            if fname == "Tilemap" { cg_last_type = "Tilemap"; return "Tilemap(" + args + ")"; }
            if fname == "TileGrid" { cg_last_type = "TileGrid"; return "TileGrid{" + args + "}"; }
            if fname == "IsometricGrid" { cg_last_type = "IsometricGrid"; return "IsometricGrid{" + args + "}"; }
            if fname == "IsoTilemap" { cg_last_type = "IsoTilemap"; return "IsoTilemap(" + args + ")"; }
            if fname == "_ks_system"  { return "_ks_run(" + args + ")"; }
            if fname == "_ks_int_to_str"  { return "std::string(_ks_int_to_str(" + args + "))"; }
            if fname == "_ks_self_dir"   { return "std::string(_ks_self_dir())"; }
            // Check engine function mapping table
            let mapped: Str = cg_engine_func(fname);
            if mapped.len() > 0 {
                return mapped + "(" + args + ")";
            }
            return fname + "(" + args + ")";
        }
        if node_kinds[callee] == NK_MEMBER {
            let obj: Str = cg_gen_expr(node_a[callee]);
            let obj_type: Str = cg_last_type;
            let method: Str = node_str[callee];
            let accessor: Str = ".";
            // Static class methods: Namespace.method → Namespace::method
            if obj == "AssetManager" { return "AssetManager::" + method + "(" + args + ")"; }
            if obj == "Random" { return "Random::" + method + "(" + args + ")"; }
            if obj == "UI" {
                let mut ui_method: Str = method;
                if method == "Draw" { ui_method = "DrawAll"; }
                if method == "WantsInput" { ui_method = "WantsInput"; }
                return "UI" + ui_method + "(" + args + ")";
            }
            if obj == "Timer" {
                return "Timer" + method + "(" + args + ")";
            }
            // Collection methods — use .size() for vectors, _ks_len for strings
            if method == "len"     { return "(int)" + obj + ".size()"; }
            if method == "isEmpty" { return obj + ".empty()"; }
            if method == "push"    { return obj + ".push_back(" + args + ")"; }
            if method == "pop"     { return "([&](){ auto _v = " + obj + ".back(); " + obj + ".pop_back(); return _v; }())"; }
            if method == "clear"   { return obj + ".clear()"; }
            // String methods
            if method == "trim"     { return "_ks_trim(" + obj + ")"; }
            if method == "upper"    { return "_ks_upper(" + obj + ")"; }
            if method == "lower"    { return "_ks_lower(" + obj + ")"; }
            if method == "contains" { return "_ks_contains(" + obj + ", " + args + ")"; }
            if method == "starts"   { return "_ks_starts(" + obj + ", " + args + ")"; }
            if method == "ends"     { return "_ks_ends(" + obj + ", " + args + ")"; }
            if method == "replace"  { return "_ks_replace(" + obj + ", " + args + ")"; }
            if method == "substr"   { return "_ks_substr(" + obj + ", " + args + ")"; }
            if method == "split"    { return "_ks_split(" + obj + ", " + args + ")"; }
            if method == "toInt"    { return "_ks_toInt(" + obj + ")"; }
            if method == "toFloat"  { return "_ks_toFloat(" + obj + ")"; }
            if method == "charAt"   { return "_ks_charAt(" + obj + ", " + args + ")"; }
            if method == "toCharCode" { return "(int)(" + obj + "[0])"; }
            if method == "toFloat"  { return "_ks_toFloat(" + obj + ")"; }
            // HashMap methods
            if method == "set"    { return obj + "[" + args + "]"; }
            if method == "get"    { return obj + ".count(" + args + ") ? std::make_optional(" + obj + "[" + args + "]) : std::nullopt"; }
            if method == "has"    { return obj + ".count(" + args + ") > 0"; }
            if method == "remove" { return obj + ".erase(" + args + ")"; }
            // Scene methods (modular mapping)
            let scene_mapped: Str = cg_scene_method(method);
            if scene_mapped.len() > 0 {
                return obj + "." + scene_mapped + "(" + args + ")";
            }
            // File namespace methods
            if method == "read"   { return "_ks_fread(" + args + ")"; }
            if method == "write"  { return "_ks_fwrite(" + args + ")"; }
            if method == "exists" { return "_ks_fexists(" + args + ")"; }
            if method == "delete" { return "_ks_fwrite(" + args + ")"; }
            if method == "append" { return "_ks_fappend(" + args + ")"; }
            if method == "lines"  { return "_ks_file_lines(_C(" + args + "))"; }
            // Node/Scene factory: .add(Type, "name") → .Add<Type>("name") or ->AddChild<Type>("name")
            if method == "add" {
                // Split args: first arg is the type, rest are function args
                // args looks like: "Collider2D, \"col\""
                // Need: AddChild<Collider2D>("col")
                let mut add_type: Str = args;
                let mut add_args: Str = "";
                let mut ci: I32 = 0;
                while ci < args.len() {
                    if args.substr(ci, 1) == "," {
                        add_type = args.substr(0, ci);
                        add_args = args.substr(ci + 2, args.len() - ci - 2);
                        ci = args.len();
                    }
                    ci = ci + 1;
                }
                cg_last_type = add_type;  // .add() returns a pointer to the type
                if obj == "this" {
                    return "this->AddChild<" + add_type + ">(" + add_args + ")";
                }
                return obj + ".Add<" + add_type + ">(" + add_args + ")";
            }
            // Tilemap methods
            if obj_type == "Tilemap" || obj_type == "IsoTilemap" {
                if method == "Set" { return obj + ".Set(" + args + ")"; }
                if method == "Get" { cg_last_type = "I32"; return obj + ".Get(" + args + ")"; }
                if method == "InBounds" { cg_last_type = "Bool"; return obj + ".InBounds(" + args + ")"; }
                if method == "WorldToTile" { cg_last_type = "TileCoord"; return obj + ".WorldToTile(" + args + ")"; }
                if method == "TileToWorld" { cg_last_type = "Vec2"; return obj + ".TileToWorld(" + args + ")"; }
                if method == "Fill" { return obj + ".Fill(" + args + ")"; }
                if method == "Clear" { return obj + ".Clear()"; }
                if method == "Draw" { return obj + ".Draw(" + args + ")"; }
            }
            if obj_type == "TileGrid" || obj_type == "IsometricGrid" {
                if method == "DrawGrid" { return obj + ".DrawGrid(" + args + ")"; }
                if method == "Snap" { cg_last_type = "Vec2"; return obj + ".Snap(" + args + ")"; }
                if method == "CellAt" { cg_last_type = "TileCoord"; return obj + ".CellAt(" + args + ")"; }
            }
            // Node methods: use -> for pointer types, . for values
            let mut call_is_ptr: Bool = cg_is_ptr(obj) || cg_type_is_ptr(obj_type);
            let mut call_acc: Str = ".";
            if call_is_ptr { call_acc = "->"; }
            return obj + call_acc + method + "(" + args + ")";
        }
        return "/* unknown call */";
    }

    if k == NK_MEMBER {
        let obj: Str = cg_gen_expr(node_a[idx]);
        let obj_type: Str = cg_last_type;
        let member: Str = node_str[idx];

        // Namespace mappings: Key.A → Key::Code::A, Mouse.Left → Mouse::Button::Left
        // AssetManager.method → AssetManager::method (static calls handled in NK_CALL too)
        if obj == "Key" { cg_last_type = "I32"; return "Key::Code::" + member; }
        if obj == "Mouse" { cg_last_type = "I32"; return "Mouse::Button::" + member; }
        if obj == "Gamepad" { cg_last_type = "I32"; return "Gamepad::" + member; }
        if obj == "Color" {
            cg_last_type = "Color";
            // Map Color::Red → RED etc
            if member == "Red" { return "RED"; }
            if member == "Green" { return "GREEN"; }
            if member == "Blue" { return "BLUE"; }
            if member == "White" { return "WHITE"; }
            if member == "Black" { return "BLACK"; }
            if member == "Yellow" { return "YELLOW"; }
            if member == "Cyan" { return "CYAN"; }
            if member == "Magenta" { return "MAGENTA"; }
            if member == "Orange" { return "ORANGE"; }
            if member == "Gray" { return "GRAY"; }
            return member;
        }

        // Determine accessor: -> for pointers, . for values
        let mut is_ptr: Bool = cg_is_ptr(obj) || cg_type_is_ptr(obj_type);
        let mut acc: Str = ".";
        if is_ptr { acc = "->"; }
        // Result members — wrapper methods
        if member == "ok"    { cg_last_type = "Bool"; return obj + acc + "ok()"; }
        if member == "value" { cg_last_type = "Str";  return obj + acc + "value()"; }
        if member == "error" { cg_last_type = "Str";  return obj + acc + "error()"; }
        // Propagate type: look up the member's type if known
        // For engine node fields: x/y/width/height/scaleX/scaleY/rotation → F64
        // For struct fields: could look up in struct table (future)
        if member == "x" || member == "y" || member == "width" || member == "height" ||
           member == "scaleX" || member == "scaleY" || member == "rotation" ||
           member == "originX" || member == "originY" || member == "speed" ||
           member == "volume" || member == "zoom" { cg_last_type = "F64"; }
        else if member == "name" || member == "text" { cg_last_type = "Str"; }
        else if member == "active" || member == "visible" || member == "current" ||
                member == "looping" || member == "solid" || member == "staticBody" { cg_last_type = "Bool"; }
        else {
            // Look up member's type from the field registry (for chained access)
            let field_t: Str = cg_lookup_field_type(obj_type, member);
            if field_t.len() > 0 {
                cg_last_type = field_t;
            } else if cg_is_engine_node(member) || cg_is_user_node(member) {
                cg_last_type = member;
            } else if cg_is_ptr(member) {
                cg_last_type = "ptr";
            } else {
                cg_last_type = node_types[idx];
            }
        }
        return obj + acc + member;
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
        let mut ret_cpp: Str = "void";
        if ret_str.len() > 0 { ret_cpp = cg_type(ret_str); }
        // Parse params from the NK_PARAM node's string: "name:type,name:type"
        let mut params_cpp: Str = "";
        let pm: I32 = node_a[idx];
        if pm != 0 {
            let pstr: Str = node_str[pm];
            if pstr.len() > 0 {
                // Split by comma
                let mut pi: I32 = 0;
                let mut seg_start: I32 = 0;
                let mut first: Bool = true;
                while pi <= pstr.len() {
                    if pi == pstr.len() || pstr.substr(pi, 1) == "," {
                        let seg: Str = pstr.substr(seg_start, pi - seg_start);
                        // Split by colon: "name:type"
                        let mut ci: I32 = 0;
                        while ci < seg.len() {
                            if seg.substr(ci, 1) == ":" {
                                let pname: Str = seg.substr(0, ci);
                                let ptype: Str = seg.substr(ci + 1, seg.len() - ci - 1);
                                if !first { params_cpp = params_cpp + ", "; }
                                params_cpp = params_cpp + cg_type(ptype) + " " + pname;
                                first = false;
                                ci = seg.len();
                            }
                            ci = ci + 1;
                        }
                        seg_start = pi + 1;
                    }
                    pi = pi + 1;
                }
            }
        }
        // Generate body inline — emit lambda header, body stmts go into main buffer
        let body_idx: I32 = node_b[idx];
        let mut header: Str = "[&](" + params_cpp + ")";
        if ret_str.len() > 0 { header = header + " -> " + ret_cpp; }
        // For inline lambdas (e.g. Timer callbacks), we generate body stmts directly
        // We collect them via a separate buffer approach
        let saved_len: I32 = cg_lines.len();
        if body_idx != 0 {
            cg_gen_stmt(body_idx);
        }
        // Collect any lines generated by the body
        let mut body_code: Str = "";
        let mut bi: I32 = saved_len;
        while bi < cg_lines.len() {
            body_code = body_code + cg_lines[bi] + "\n";
            bi = bi + 1;
        }
        // Remove body lines from main buffer (they'll be inlined in the expression)
        while cg_lines.len() > saved_len {
            cg_lines.pop();
        }
        return header + " {\n" + body_code + "}";
    }

    return "/* expr " + node_str[idx] + " */";
}

// ── Statement codegen ────────────────────────────────────────────────────
func cg_gen_stmt(idx: I32) {
    if idx == 0 { return; }
    let k: I32 = node_kinds[idx];

    if k == NK_LET {
        let name: Str = strip_type_ann(node_str[idx]);
        let type_ann: Str = get_type_ann(node_str[idx]);
        let init: I32 = node_a[idx];
        if init != 0 {
            let val: Str = cg_gen_expr(init);
            let init_k: I32 = node_kinds[init];
            if init_k == NK_ARRAY_LIT {
                let mut elem_type: Str = "int32_t";
                let first_elem: I32 = node_a[init];
                if first_elem != 0 {
                    let ek: I32 = node_kinds[node_a[first_elem]];
                    if ek == NK_STR_LIT { elem_type = "std::string"; }
                    if ek == NK_BOOL    { elem_type = "bool"; }
                    if ek == NK_FLOAT   { elem_type = "float"; }
                }
                cg_emit("std::vector<" + elem_type + "> " + name + " = " + val + ";");
            } else {
                // Use explicit std::string for string-typed vars
                let vtype: Str = node_types[idx];
                if vtype == "Str" || init_k == NK_STR_LIT || init_k == NK_FSTR {
                    cg_emit("std::string " + name + " = " + val + ";");
                } else if cg_is_ptr_type(type_ann) {
                    // User/engine node type annotation → emit pointer declaration
                    cg_emit(type_ann + "* " + name + " = " + val + ";");
                    cg_mark_ptr(name);
                } else if cg_is_engine_val_type(type_ann) {
                    // Engine value type annotation → emit explicit type
                    cg_emit(type_ann + " " + name + " = " + val + ";");
                } else {
                    cg_emit("auto " + name + " = " + val + ";");
                    // Track pointer vars (scene.Add, AddChild return pointers)
                    if val.contains(".Add<") || val.contains("->AddChild<") {
                        cg_mark_ptr(name);
                    }
                }
            }
        } else {
            let vtype: Str = node_types[idx];
            if vtype == "Str" || type_ann == "Str" {
                cg_emit("std::string " + name + " = \"\";");
            } else if cg_is_ptr_type(type_ann) {
                // User/engine node type annotation → emit pointer declaration
                cg_emit(type_ann + "* " + name + " = nullptr;");
                cg_mark_ptr(name);
            } else {
                cg_emit("auto " + name + " = 0;");
            }
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
        // Push directly to cg_lines to bypass any cg_emit string issues
        if node_a[idx] != 0 {
            let val: Str = cg_gen_expr(node_a[idx]);
            cg_lines.push("    return " + val + ";");
        } else {
            cg_lines.push("    return;");
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
    if k == NK_ASM      { cg_gen_asm(idx); return; }

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

    // Special case: main gets argc/argv and initializes runtime args
    if name == "main" {
        cg_emit("int main(int argc, char** argv) {");
        cg_indent_inc();
        cg_emit("_ks_init_args(argc, argv);");
    } else {
        cg_emit(ret_cpp + " " + name + "(" + params + ") {");
        cg_indent_inc();
    }
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
// ── Engine node type check ───────────────────────────────────────────────
// Returns true if a type name is a built-in engine node (always a pointer)
func cg_is_engine_node(t: Str) -> Bool {
    if t == "Node2D" || t == "Sprite2D" || t == "Collider2D" { return true; }
    if t == "AnimatedSprite2D" || t == "AnimationPlayer" { return true; }
    if t == "CameraNode2D" { return true; }
    if t == "Node" { return true; }
    return false;
}

// Check if a type is a user-defined node (tracked during codegen)
let mut cg_user_nodes: [Str] = [""];
let mut cg_user_node_count: I32 = 0;

func cg_register_node(name: Str) {
    cg_user_node_count = cg_user_node_count + 1;
    cg_user_nodes.push(name);
}

func cg_is_user_node(name: Str) -> Bool {
    let mut i: I32 = 1;
    while i <= cg_user_node_count {
        if cg_user_nodes[i] == name { return true; }
        i = i + 1;
    }
    return false;
}

// Returns true if a type is an engine value type (not a pointer)
func cg_is_engine_val_type(t: Str) -> Bool {
    if t == "Camera2D" || t == "Scene" || t == "Color" { return true; }
    if t == "Tilemap" || t == "TileGrid" || t == "TileCoord" { return true; }
    if t == "IsometricGrid" || t == "IsoTilemap" { return true; }
    if t == "Vector2" || t == "Vec2" { return true; }
    if t == "Sound" || t == "Music" || t == "Texture" || t == "Font" { return true; }
    if t == "Rectangle" || t == "Circle" { return true; }
    return false;
}

// Returns true if a type should be a pointer in C++
func cg_is_ptr_type(t: Str) -> Bool {
    return cg_is_engine_node(t) || cg_is_user_node(t);
}

// ── Lifecycle method signature map ───────────────────────────────────────
func cg_lifecycle_sig(method_name: Str) -> Str {
    if method_name == "Ready"            { return "void Ready() override"; }
    if method_name == "Update"           { return "void Update(float dt) override"; }
    if method_name == "Draw"             { return "void Draw() override"; }
    if method_name == "OnCollisionEnter" { return "void OnCollisionEnter(Collider2D* other) override"; }
    if method_name == "OnCollisionExit"  { return "void OnCollisionExit(Collider2D* other) override"; }
    return "";
}

// ── Engine API function mapping ──────────────────────────────────────────
// Maps KonScript function names to C++ equivalents
// Add new engine functions here — one line each
func cg_engine_func(name: Str) -> Str {
    // Input - keyboard
    if name == "KeyDown"       { return "IsKeyDown"; }
    if name == "KeyPressed"    { return "IsKeyPressed"; }
    if name == "KeyReleased"   { return "IsKeyReleased"; }
    // Input - mouse
    if name == "MouseDown"     { return "IsMouseButtonDown"; }
    if name == "MousePressed"  { return "IsMouseButtonPressed"; }
    if name == "MouseReleased" { return "IsMouseButtonReleased"; }
    // Math
    if name == "Sqrt"    { return "sqrtf"; }
    if name == "Abs"     { return "fabsf"; }
    if name == "Floor"   { return "floorf"; }
    if name == "Ceil"    { return "ceilf"; }
    if name == "Round"   { return "roundf"; }
    if name == "Sin"     { return "sinf"; }
    if name == "Cos"     { return "cosf"; }
    if name == "Tan"     { return "tanf"; }
    if name == "Atan2"   { return "atan2f"; }
    if name == "Min"     { return "std::min"; }
    if name == "Max"     { return "std::max"; }
    // Everything else passes through (InitWindow, SetTargetFPS, etc.)
    return "";
}

// ── Scene method mapping ─────────────────────────────────────────────────
func cg_scene_method(method: Str) -> Str {
    if method == "update" { return "Update"; }
    if method == "draw"   { return "Draw"; }
    if method == "scan"   { return "Scan"; }
    if method == "remove" { return "Remove"; }
    if method == "get"    { return "GetNode"; }
    return "";
}

// ── Extern declaration codegen ────────────────────────────────────────────
// Emits: extern "C" { RetType name(params...); }
// str format: "name|linkage|ret|params|variadic"
func cg_gen_extern(idx: I32) {
    let full: Str = node_str[idx];
    // Parse fields separated by |
    let mut parts: [Str] = [""];
    parts.clear();
    let mut start: I32 = 0;
    let mut ei: I32 = 0;
    while ei <= full.len() {
        if ei == full.len() || full.substr(ei, 1) == "|" {
            parts.push(full.substr(start, ei - start));
            start = ei + 1;
        }
        ei = ei + 1;
    }
    // parts[0]=name, [1]=linkage, [2]=ret, [3]=params, [4]=variadic
    let mut ename: Str = "";
    let mut elinkage: Str = "C";
    let mut eret: Str = "void";
    let mut eparams: Str = "";
    let mut evariadic: Str = "";
    if parts.len() > 0 { ename = parts[0]; }
    if parts.len() > 1 { elinkage = parts[1]; }
    if parts.len() > 2 { eret = parts[2]; }
    if parts.len() > 3 { eparams = parts[3]; }
    if parts.len() > 4 { evariadic = parts[4]; }

    // Build C++ parameter list — use C types for extern (not std::string)
    let mut cpp_params: Str = "";
    if eparams.len() > 0 {
        let param_parts: [Str] = eparams.split(",");
        let mut pi: I32 = 0;
        while pi < param_parts.len() {
            if pi > 0 { cpp_params = cpp_params + ", "; }
            let pt: Str = param_parts[pi];
            // For extern "C", use C-compatible types
            if pt == "Str" { cpp_params = cpp_params + "const char*"; }
            else { cpp_params = cpp_params + cg_type(pt); }
            pi = pi + 1;
        }
    }
    if evariadic == "..." {
        if cpp_params.len() > 0 { cpp_params = cpp_params + ", ..."; }
        else { cpp_params = "..."; }
    }

    // Use C-compatible return type
    let mut cret: Str = cg_type(eret);
    if eret == "Str" { cret = "const char*"; }
    cg_emit_raw("extern \"" + elinkage + "\" " + cret + " " + ename + "(" + cpp_params + ");");
}

// ── Inline assembly codegen ──────────────────────────────────────────────
// Emits: __asm__ volatile("template" : outputs : inputs : clobbers);
// str format: "template|outputs|inputs|clobbers"
func cg_gen_asm(idx: I32) {
    let full: Str = node_str[idx];
    let mut parts: [Str] = [""];
    parts.clear();
    let mut start: I32 = 0;
    let mut ai: I32 = 0;
    while ai <= full.len() {
        if ai == full.len() || full.substr(ai, 1) == "|" {
            parts.push(full.substr(start, ai - start));
            start = ai + 1;
        }
        ai = ai + 1;
    }
    let mut tmpl: Str = "";
    let mut outputs: Str = "";
    let mut inputs: Str = "";
    let mut clobbers: Str = "";
    if parts.len() > 0 { tmpl = parts[0]; }
    if parts.len() > 1 { outputs = parts[1]; }
    if parts.len() > 2 { inputs = parts[2]; }
    if parts.len() > 3 { clobbers = parts[3]; }

    let mut asm_line: Str = "__asm__ volatile(\"" + cg_escape_str(tmpl) + "\"";
    if outputs.len() > 0 || inputs.len() > 0 || clobbers.len() > 0 {
        asm_line = asm_line + " : \"" + outputs + "\"";
        if inputs.len() > 0 || clobbers.len() > 0 {
            asm_line = asm_line + " : \"" + inputs + "\"";
            if clobbers.len() > 0 {
                asm_line = asm_line + " : \"" + clobbers + "\"";
            }
        }
    }
    asm_line = asm_line + ");";
    cg_emit(asm_line);
}

// ── Node codegen ─────────────────────────────────────────────────────────
func cg_gen_node(idx: I32) {
    // Parse name|base from node_str
    let full: Str = node_str[idx];
    let mut name: Str = "";
    let mut base: Str = "Node2D";
    let mut pi: I32 = 0;
    while pi < full.len() {
        if full.substr(pi, 1) == "|" {
            name = full.substr(0, pi);
            base = full.substr(pi + 1, full.len() - pi - 1);
            pi = full.len();
        }
        pi = pi + 1;
    }
    if name.len() == 0 { name = full; }

    // Register as user node type
    cg_register_node(name);

    // Class declaration
    cg_emit("class " + name + " : public " + base + " {");
    cg_emit("public:");
    cg_indent_inc();

    // Collect fields and methods
    let mut ctor_inits: [Str] = [""];
    let mut m: I32 = node_a[idx];
    while m != 0 {
        let member: I32 = node_a[m];
        let mk: I32 = node_kinds[member];

        if mk == NK_LET {
            // Field declaration — node_str may be "name" or "name:Type"
            let fname: Str = strip_type_ann(node_str[member]);
            let type_ann: Str = get_type_ann(node_str[member]);
            let finit: I32 = node_a[member];
            // Resolve type: prefer explicit annotation, then node_types, then infer
            let mut ft: Str = node_types[member];
            if type_ann.len() > 0 { ft = type_ann; }
            let mut ftype: Str = "int32_t";
            if ft == "F64" || ft == "F32" || ft == "Float" { ftype = "float"; }
            if ft == "Str" { ftype = "std::string"; }
            if ft == "Bool" { ftype = "bool"; }
            if ft == "I32" || ft == "I64" { ftype = cg_type(ft); }
            if ft == "Vec2" || ft == "Vector2" { ftype = "Vector2"; }
            if ft == "Scene" { ftype = "Scene"; }
            if cg_is_ptr_type(ft) { ftype = ft + "*"; cg_mark_ptr(fname); }
            // Register field type for chained access resolution
            cg_register_field(name, fname, ft);
            if finit != 0 {
                let fval: Str = cg_gen_expr(finit);
                let fk: I32 = node_kinds[finit];
                // If type still default, infer from init expression
                if ftype == "int32_t" {
                    if fk == NK_FLOAT { ftype = "float"; }
                    if fk == NK_BOOL  { ftype = "bool"; }
                    if fk == NK_STR_LIT { ftype = "std::string"; }
                    if fk == NK_NULL_LIT { ftype = "void*"; }
                }
                // Check if init is trivial or needs ctor
                if fk == NK_INT || fk == NK_FLOAT || fk == NK_BOOL || fk == NK_STR_LIT || fk == NK_NULL_LIT {
                    cg_emit(ftype + " " + fname + " = " + fval + ";");
                } else {
                    // Non-trivial: declare field, defer init to ctor
                    if ftype.ends("*") {
                        cg_emit(ftype + " " + fname + " = nullptr;");
                    } else {
                        cg_emit(ftype + " " + fname + " = {};");
                    }
                    ctor_inits.push(fname + " = " + fval + ";");
                }
            } else {
                if ftype.ends("*") {
                    cg_emit(ftype + " " + fname + " = nullptr;");
                } else {
                    cg_emit(ftype + " " + fname + " = {};");
                }
            }
        }

        if mk == NK_CONST_D {
            // Const field inside node
            let fname: Str = node_str[member];
            let finit: I32 = node_a[member];
            if finit != 0 {
                let fval: Str = cg_gen_expr(finit);
                let ft: Str = node_types[member];
                let mut ftype: Str = "auto";
                if ft == "F64" || ft == "F32" { ftype = "float"; }
                if ft == "I32" { ftype = "int32_t"; }
                if ft == "Str" { ftype = "std::string"; }
                if ft == "Bool" { ftype = "bool"; }
                cg_emit("static constexpr " + ftype + " " + fname + " = " + fval + ";");
            }
        }

        if mk == NK_FUNC {
            // Method
            let mname: Str = func_name(node_str[member]);
            let mret: Str = func_ret(node_str[member]);
            let lifecycle: Str = cg_lifecycle_sig(mname);
            if lifecycle.len() > 0 {
                // Lifecycle method with override
                // Register pointer params for collision callbacks
                if mname == "OnCollisionEnter" || mname == "OnCollisionExit" {
                    cg_mark_ptr("other");
                }
                cg_emit(lifecycle + " {");
                // Call super for Ready/Update so base body types initialize properly
                // (e.g. StaticBody2D::Ready marks colliders as solid+static)
                if mname == "Ready" { cg_emit("    " + base + "::Ready();"); }
                if mname == "Update" { cg_emit("    " + base + "::Update(dt);"); }
            } else {
                // Regular method
                let mut mparams: Str = "";
                let mut mp: I32 = node_a[member];
                let mut mfirst: Bool = true;
                while mp != 0 {
                    let mpn: I32 = node_a[mp];
                    let mps: Str = node_str[mpn];
                    let mut mci: I32 = 0;
                    let mut mpname: Str = "";
                    let mut mptype: Str = "";
                    while mci < mps.len() {
                        if mps.substr(mci, 1) == ":" {
                            mpname = mps.substr(0, mci);
                            mptype = mps.substr(mci + 1, mps.len() - mci - 1);
                            mci = mps.len();
                        }
                        mci = mci + 1;
                    }
                    if !mfirst { mparams = mparams + ", "; }
                    mparams = mparams + cg_type(mptype) + " " + mpname;
                    mfirst = false;
                    mp = node_b[mp];
                }
                cg_emit(cg_type(mret) + " " + mname + "(" + mparams + ") {");
            }
            cg_indent_inc();
            cg_gen_stmt(node_b[member]);
            cg_indent_dec();
            cg_emit("}");
            cg_emit("");
        }

        m = node_b[m];
    }

    // Constructor
    cg_indent_dec();
    cg_emit("");
    cg_emit("    " + name + "(const std::string& _name) : " + base + "(_name) {");
    let mut ci: I32 = 1;
    while ci < ctor_inits.len() {
        cg_emit("        " + ctor_inits[ci]);
        ci = ci + 1;
    }
    cg_emit("    }");
    cg_emit("};");
    cg_emit("");
}

// ── Main codegen entry point ─────────────────────────────────────────────
func cg_emit_fwd_decls(pidx: I32) {
    let mut fwd: I32 = node_a[pidx];
    while fwd != 0 {
        let fd: I32 = node_a[fwd];
        if node_kinds[fd] == NK_FUNC {
            let full_fn: Str = node_str[fd];
            let fn_name: Str = func_name(full_fn);
            let fn_ret: Str = func_ret(full_fn);
            let fn_ret_cpp: Str = cg_type(fn_ret);
            let mut fwd_params: Str = "";
            let mut fpm: I32 = node_a[fd];
            let mut fwd_first: Bool = true;
            while fpm != 0 {
                let fpn: I32 = node_a[fpm];
                let fps: Str = node_str[fpn];
                let mut fci: I32 = 0;
                let mut fptype: Str = "";
                while fci < fps.len() {
                    if fps.substr(fci, 1) == ":" {
                        fptype = fps.substr(fci + 1, fps.len() - fci - 1);
                        fci = fps.len();
                    }
                    fci = fci + 1;
                }
                if !fwd_first { fwd_params = fwd_params + ", "; }
                fwd_params = fwd_params + cg_type(fptype);
                fwd_first = false;
                fpm = node_b[fpm];
            }
            if fn_name == "main" {
                cg_emit("int main(int argc, char** argv);");
            } else {
                cg_emit(fn_ret_cpp + " " + fn_name + "(" + fwd_params + ");");
            }
        }
        fwd = node_b[fwd];
    }
}

func cg_emit_toplevel(pidx: I32) {
    let mut decl: I32 = node_a[pidx];
    while decl != 0 {
        let d: I32 = node_a[decl];
        if node_kinds[d] == NK_FUNC {
            let fn_name: Str = func_name(node_str[d]);
            if fn_name != "main" { cg_gen_func(d); }
        }
        if node_kinds[d] == NK_STRUCT_D { cg_gen_struct(d); }
        if node_kinds[d] == NK_NODE { cg_gen_node(d); }
        if node_kinds[d] == NK_CONST_D {
            let val: Str = cg_gen_expr(node_a[d]);
            cg_emit("constexpr auto " + node_str[d] + " = " + val + ";");
        }
        if node_kinds[d] == NK_EXTERN { cg_gen_extern(d); }
        decl = node_b[decl];
    }
}

func cg_generate(prog_idx: I32) -> Str {
    cg_reset();

    // Pre-scan: detect #include <engine> and register node types
    let mut pre: I32 = node_a[prog_idx];
    while pre != 0 {
        let pd: I32 = node_a[pre];
        if node_kinds[pd] == NK_INCLUDE_D {
            if node_str[pd] == "engine" { cg_is_engine = true; }
        }
        if node_kinds[pd] == NK_NODE {
            // Register user node name for pointer tracking
            let nstr: Str = node_str[pd];
            let mut npi: I32 = 0;
            let mut nname: Str = nstr;
            while npi < nstr.len() {
                if nstr.substr(npi, 1) == "|" {
                    nname = nstr.substr(0, npi);
                    npi = nstr.len();
                }
                npi = npi + 1;
            }
            cg_register_node(nname);
        }
        pre = node_b[pre];
    }

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
    cg_emit_raw("// Runtime C functions");
    cg_emit_raw("extern \"C\" {");
    cg_emit_raw("void* _ks_file_read(const char*);");
    cg_emit_raw("void* _ks_file_write(const char*, const char*);");
    cg_emit_raw("void* _ks_file_append(const char*, const char*);");
    cg_emit_raw("int _ks_file_exists(const char*);");
    cg_emit_raw("void* _ks_file_delete(const char*);");
    cg_emit_raw("int _ks_str_len(const char*);");
    cg_emit_raw("char* _ks_str_substr(const char*, int, int);");
    cg_emit_raw("char* _ks_str_trim(const char*);");
    cg_emit_raw("char* _ks_str_upper(const char*);");
    cg_emit_raw("char* _ks_str_lower(const char*);");
    cg_emit_raw("char* _ks_str_replace(const char*, const char*, const char*);");
    cg_emit_raw("int _ks_str_contains(const char*, const char*);");
    cg_emit_raw("int _ks_str_starts(const char*, const char*);");
    cg_emit_raw("int _ks_str_ends(const char*, const char*);");
    cg_emit_raw("char* _ks_str_concat(const char*, const char*);");
    cg_emit_raw("int _ks_str_compare(const char*, const char*);");
    cg_emit_raw("char* _ks_str_charAt(const char*, int);");
    cg_emit_raw("int _ks_str_toInt(const char*);");
    cg_emit_raw("float _ks_str_toFloat(const char*);");
    cg_emit_raw("void* _ks_str_split(const char*, const char*);");
    cg_emit_raw("extern \"C\" char* _ks_int_to_str(int);");
    cg_emit_raw("int _ks_str_isEmpty(const char*);");
    cg_emit_raw("int _ks_str_isAlpha(const char*);");
    cg_emit_raw("int _ks_str_isDigit(const char*);");
    cg_emit_raw("int _ks_str_toCharCode(const char*);");
    cg_emit_raw("char* _ks_str_fromCharCode(int);");
    cg_emit_raw("void* _ks_array_new(int);");
    cg_emit_raw("void _ks_array_push(void*, void*);");
    cg_emit_raw("void* _ks_array_get(void*, int);");
    cg_emit_raw("int _ks_array_len(void*);");
    cg_emit_raw("void _ks_array_clear(void*);");
    cg_emit_raw("void* _ks_array_pop(void*);");
    cg_emit_raw("int _ks_array_has(void*, void*);");
    cg_emit_raw("void _ks_array_set(void*, int, void*);");
    cg_emit_raw("void* _ks_array_clone(void*);");
    cg_emit_raw("void _ks_array_free(void*);");
    cg_emit_raw("void* _ks_hashmap_new();");
    cg_emit_raw("void _ks_hashmap_set(void*, const char*, void*);");
    cg_emit_raw("void* _ks_hashmap_get(void*, const char*);");
    cg_emit_raw("int _ks_hashmap_has(void*, const char*);");
    cg_emit_raw("int _ks_hashmap_len(void*);");
    cg_emit_raw("int _ks_result_ok(void*);");
    cg_emit_raw("char* _ks_result_value(void*);");
    cg_emit_raw("char* _ks_result_error(void*);");
    cg_emit_raw("int _ks_system(const char*);");
    cg_emit_raw("void _ks_init_args(int, char**);");
    cg_emit_raw("int _ks_argc();");
    cg_emit_raw("char* _ks_get_argv(int);");
    cg_emit_raw("double _ks_time_ms();");
    cg_emit_raw("char* _ks_self_dir();");
    cg_emit_raw("}");
    cg_emit_raw("");
    // C++ wrappers that accept std::string
    cg_emit_raw("// C++ wrappers for std::string bridge");
    cg_emit_raw("inline std::string _S(const char* s) { return s ? s : \"\"; }");
    cg_emit_raw("inline const char* _C(const std::string& s) { return s.c_str(); }");
    cg_emit_raw("inline std::string _ks_substr(const std::string& s, int p, int l) { return _S(_ks_str_substr(_C(s), p, l)); }");
    cg_emit_raw("inline std::string _ks_trim(const std::string& s) { return _S(_ks_str_trim(_C(s))); }");
    cg_emit_raw("inline std::string _ks_upper(const std::string& s) { return _S(_ks_str_upper(_C(s))); }");
    cg_emit_raw("inline std::string _ks_lower(const std::string& s) { return _S(_ks_str_lower(_C(s))); }");
    cg_emit_raw("inline std::string _ks_replace(const std::string& s, const std::string& f, const std::string& t) { return _S(_ks_str_replace(_C(s), _C(f), _C(t))); }");
    cg_emit_raw("inline bool _ks_contains(const std::string& s, const std::string& sub) { return _ks_str_contains(_C(s), _C(sub)); }");
    cg_emit_raw("inline bool _ks_starts(const std::string& s, const std::string& p) { return _ks_str_starts(_C(s), _C(p)); }");
    cg_emit_raw("inline bool _ks_ends(const std::string& s, const std::string& e) { return _ks_str_ends(_C(s), _C(e)); }");
    cg_emit_raw("inline int _ks_len(const std::string& s) { return (int)s.size(); }");
    cg_emit_raw("inline bool _ks_isEmpty(const std::string& s) { return s.empty(); }");
    cg_emit_raw("inline std::string _ks_charAt(const std::string& s, int i) { return _S(_ks_str_charAt(_C(s), i)); }");
    cg_emit_raw("inline int _ks_toInt(const std::string& s) { return _ks_str_toInt(_C(s)); }");
    cg_emit_raw("inline float _ks_toFloat(const std::string& s) { return _ks_str_toFloat(_C(s)); }");
    cg_emit_raw("inline int _ks_compare(const std::string& a, const std::string& b) { return _ks_str_compare(_C(a), _C(b)); }");
    cg_emit_raw("inline std::vector<std::string> _ks_split(const std::string& s, const std::string& d) { auto* arr = _ks_str_split(_C(s), _C(d)); std::vector<std::string> v; for(int i=0;i<_ks_array_len(arr);i++) v.push_back(_S((const char*)_ks_array_get(arr,i))); return v; }");
    cg_emit_raw("// Result wrapper");
    cg_emit_raw("struct _KsResultW { void* _r; bool ok() { return _ks_result_ok(_r); } std::string value() { return _S(_ks_result_value(_r)); } std::string error() { return _S(_ks_result_error(_r)); } };");
    cg_emit_raw("inline _KsResultW _ks_fread(const std::string& p) { return {_ks_file_read(_C(p))}; }");
    cg_emit_raw("inline _KsResultW _ks_fwrite(const std::string& p, const std::string& c) { return {_ks_file_write(_C(p), _C(c))}; }");
    cg_emit_raw("inline _KsResultW _ks_fappend(const std::string& p, const std::string& c) { return {_ks_file_append(_C(p), _C(c))}; }");
    cg_emit_raw("inline bool _ks_fexists(const std::string& p) { return _ks_file_exists(_C(p)); }");
    cg_emit_raw("inline int _ks_run(const std::string& cmd) { return _ks_system(_C(cmd)); }");
    cg_emit_raw("// Print helper");
    cg_emit_raw("template<typename... Args> void Print(Args... args) { ((std::cout << args), ...); std::cout << std::endl; }");
    cg_emit_raw("// Universal to-string conversion");
    cg_emit_raw("inline std::string _ks_tostr(const std::string& s) { return s; }");
    cg_emit_raw("inline std::string _ks_tostr(const char* s) { return s ? s : \"\"; }");
    cg_emit_raw("inline std::string _ks_tostr(int v) { return std::to_string(v); }");
    cg_emit_raw("inline std::string _ks_tostr(long v) { return std::to_string(v); }");
    cg_emit_raw("inline std::string _ks_tostr(float v) { return std::to_string(v); }");
    cg_emit_raw("inline std::string _ks_tostr(double v) { return std::to_string(v); }");
    cg_emit_raw("inline std::string _ks_tostr(bool v) { return v ? \"true\" : \"false\"; }");
    cg_emit_raw("inline std::string _ks_tostr(const _KsResultW& r) { if (!r._r) return \"Result(null)\"; return ((_KsResultW&)r).ok() ? (\"Result::Ok(\" + ((_KsResultW&)r).value() + \")\") : (\"Result::Err(\" + ((_KsResultW&)r).error() + \")\"); }");
    cg_emit_raw("");

    // Forward declarations for all functions
    cg_emit_fwd_decls(prog_idx);
    cg_emit("");

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
        if node_kinds[d] == NK_NODE {
            cg_gen_node(d);
        }
        if node_kinds[d] == NK_CONST_D {
            let gname: Str = strip_type_ann(node_str[d]);
            let val: Str = cg_gen_expr(node_a[d]);
            cg_emit("constexpr auto " + gname + " = " + val + ";");
        }
        if node_kinds[d] == NK_LET {
            let gname: Str = strip_type_ann(node_str[d]);
            let ginit: I32 = node_a[d];
            let val: Str = cg_gen_expr(ginit);
            // Check if initializer is an array literal
            if ginit != 0 && node_kinds[ginit] == NK_ARRAY_LIT {
                // Infer element type from first element
                let mut elem_type: Str = "int32_t";
                let first_elem: I32 = node_a[ginit];
                if first_elem != 0 {
                    let ek: I32 = node_kinds[node_a[first_elem]];
                    if ek == NK_STR_LIT { elem_type = "std::string"; }
                    if ek == NK_BOOL    { elem_type = "bool"; }
                    if ek == NK_FLOAT   { elem_type = "float"; }
                }
                cg_emit("std::vector<" + elem_type + "> " + gname + " = " + val + ";");
            } else {
                // Use std::string for string literal initializers
                if ginit != 0 && (node_kinds[ginit] == NK_STR_LIT || node_kinds[ginit] == NK_FSTR) {
                    cg_emit("std::string " + gname + " = " + val + ";");
                } else {
                    cg_emit("auto " + gname + " = " + val + ";");
                }
            }
        }
        if node_kinds[d] == NK_INCLUDE_D {
            let path: Str = node_str[d];
            if path == "engine" {
                cg_is_engine = true;
            } else {
                // Pass through C/C++ includes: #include "file.h" or #include <header>
                if path.ends(".h") || path.ends(".hpp") || path.ends(".hh") {
                    cg_emit_raw("#include \"" + path + "\"");
                }
            }
        }
        // Extern declarations: extern "C" func name(params) -> ret;
        if node_kinds[d] == NK_EXTERN {
            cg_gen_extern(d);
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


// ===== END CODEGEN =====

// ── Usage help ───────────────────────────────────────────────────────────
// ── Progress bar helpers (pure KonScript) ────────────────────────────────
// Uses _ks_system("printf ...") for ANSI escape codes and Unicode blocks.
// Each call is kept small to avoid string concat issues in the LLVM path.
// Progress bar block characters are inlined in the functions below

func pad_name(name: Str, width: I32) -> Str {
    let mut s: Str = name;
    while s.len() < width { s = s + " "; }
    return s;
}

func format_ms(ms: F64) -> Str {
    // Convert to tenths of a millisecond via integer math
    let mut tenths: I32 = (ms * 10.0) as I32;
    if tenths < 0 { tenths = 0; }
    if tenths < 10000 {
        // Under 1 second → show as N.Nms
        let whole: I32 = tenths / 10;
        let frac: I32 = tenths - whole * 10;
        return _ks_int_to_str(whole) + "." + _ks_int_to_str(frac) + "ms";
    }
    // 1 second or more → show as N.Ns
    let ms_i: I32 = tenths / 10;
    let secs: I32 = ms_i / 1000;
    let sfrac: I32 = (ms_i - secs * 1000) / 100;
    return _ks_int_to_str(secs) + "." + _ks_int_to_str(sfrac) + "s";
}

func stage_doing(step: I32, total: I32, name: Str) {
    let hdr: Str = "[" + _ks_int_to_str(step) + "/" + _ks_int_to_str(total) + "]";
    let n: Str = pad_name(name, 16);
    // Multiple small calls — avoids heap pressure in LLVM-compiled binaries
    _ks_system("printf '\\033[2m" + hdr + "\\033[0m '");
    _ks_system("printf '\\033[1m" + n + "\\033[0m '");
    _ks_system("printf '\\033[33m\\xe2\\x96\\x91\\xe2\\x96\\x91\\xe2\\x96\\x91\\xe2\\x96\\x91\\xe2\\x96\\x91\\xe2\\x96\\x91\\xe2\\x96\\x91\\xe2\\x96\\x91\\xe2\\x96\\x91\\xe2\\x96\\x91\\xe2\\x96\\x91\\xe2\\x96\\x91\\xe2\\x96\\x91\\xe2\\x96\\x91\\xe2\\x96\\x91\\xe2\\x96\\x91\\xe2\\x96\\x91\\xe2\\x96\\x91\\xe2\\x96\\x91\\xe2\\x96\\x91\\033[0m'");
    _ks_system("printf '  ...\\r'");
}

func stage_ok(step: I32, total: I32, name: Str, ms: F64) {
    let hdr: Str = "[" + _ks_int_to_str(step) + "/" + _ks_int_to_str(total) + "]";
    let n: Str = pad_name(name, 16);
    let ts: Str = format_ms(ms);
    _ks_system("printf '\\r\\033[2m" + hdr + "\\033[0m '");
    _ks_system("printf '\\033[1m" + n + "\\033[0m '");
    _ks_system("printf '\\033[32m\\xe2\\x96\\x93\\xe2\\x96\\x93\\xe2\\x96\\x93\\xe2\\x96\\x93\\xe2\\x96\\x93\\xe2\\x96\\x93\\xe2\\x96\\x93\\xe2\\x96\\x93\\xe2\\x96\\x93\\xe2\\x96\\x93\\xe2\\x96\\x93\\xe2\\x96\\x93\\xe2\\x96\\x93\\xe2\\x96\\x93\\xe2\\x96\\x93\\xe2\\x96\\x93\\xe2\\x96\\x93\\xe2\\x96\\x93\\xe2\\x96\\x93\\xe2\\x96\\x93\\033[0m'");
    _ks_system("printf '  \\033[2m" + ts + "\\033[0m\\n'");
}

let mut cg_target_label: Str = "linux64";

func print_success(path: Str) {
    _ks_system("printf '\\n  \\033[1m\\033[32m\\xe2\\x9c\\x93\\033[0m '");
    _ks_system("printf '\\033[1m" + path + "\\033[0m  '");
    _ks_system("printf '\\033[2m[" + cg_target_label + "]\\033[0m\\n\\n'");
}

func binary_dir() -> Str {
    let exe: Str = _ks_get_argv(0);
    let mut last_slash: I32 = -1;
    let mut i: I32 = 0;
    while i < exe.len() {
        if exe.substr(i, 1) == "/" { last_slash = i; }
        i = i + 1;
    }
    if last_slash >= 0 { return exe.substr(0, last_slash); }
    return ".";
}

func print_usage() {
    Print("KonScript compiler v0.1 - self-hosted");
    Print("");
    Print("Usage: konscript [options] <file.ks>");
    Print("");
    Print("Options:");
    Print("  -o <file>        Output file name (default: input stem)");
    Print("  -I<dir>          Add include search directory");
    Print("  -L<dir>          Add library search directory");
    Print("  -l<lib>          Link library (e.g. -lSDL2)");
    Print("  --cpp            Output C++ source only (don't compile)");
    Print("  --no-stdlib      Don't link KonScript runtime (for OS dev)");
    Print("  --pack           Enable KonPak asset pack support (-DKON_USE_PACK)");
    Print("  --help, -h       Show this help");
    Print("");
    Print("Examples:");
    Print("  konscript game.ks                        Compile to native binary");
    Print("  konscript game.ks -o mygame              Custom output name");
    Print("  konscript --cpp game.ks -o game.cpp      Emit C++ only");
    Print("  konscript app.ks -I/usr/include/SDL2 -lSDL2");
    Print("  konscript kernel.ks --no-stdlib          Bare-metal (OS dev)");
    Print("  konscript app.ks -I/usr/include/qt6 -lQt6Widgets -lQt6Core");
}

func main() -> I32 {
    // ── Parse command-line arguments ─────────────────────────────────────
    let arg_count: I32 = _ks_argc();

    let mut input_file: Str = "";
    let mut output_file: Str = "";
    let mut extra_includes: [Str] = [""];
    let mut extra_libdirs: [Str] = [""];
    let mut extra_libs: [Str] = [""];
    let mut cpp_only: Bool = false;
    let mut no_stdlib: Bool = false;
    let mut pack_support: Bool = false;
    let mut target_platform: Str = "linux64";
    extra_includes.clear();
    extra_libdirs.clear();
    extra_libs.clear();

    // If no args, fall back to .ks_input file (backwards compatible)
    if arg_count < 2 {
        let path_result: Result<Str> = File.read(".ks_input");
        if !path_result.ok {
            print_usage();
            return 1;
        }
        input_file = path_result.value.trim();
    } else {
        // Parse CLI flags
        let mut i: I32 = 1;
        while i < arg_count {
            let arg: Str = _ks_get_argv(i);
            if arg == "--help" || arg == "-h" { print_usage(); return 0; }
            if arg == "--cpp" { cpp_only = true; i = i + 1; continue; }
            if arg == "--no-stdlib" { no_stdlib = true; i = i + 1; continue; }
            if arg == "--pack" { pack_support = true; i = i + 1; continue; }
            // --target=windows64 or --target windows64
            if arg.starts("--target=") {
                target_platform = arg.substr(9, arg.len() - 9);
                i = i + 1; continue;
            }
            if arg == "--target" && i + 1 < arg_count {
                i = i + 1;
                target_platform = _ks_get_argv(i);
                i = i + 1; continue;
            }
            if arg == "-o" && i + 1 < arg_count {
                i = i + 1;
                output_file = _ks_get_argv(i);
                i = i + 1;
                continue;
            }
            // -Idir or -I dir
            if arg.starts("-I") {
                if arg.len() > 2 {
                    extra_includes.push(arg.substr(2, arg.len() - 2));
                } else {
                    if i + 1 < arg_count { i = i + 1; extra_includes.push(_ks_get_argv(i)); }
                }
                i = i + 1;
                continue;
            }
            // -Ldir or -L dir
            if arg.starts("-L") {
                if arg.len() > 2 {
                    extra_libdirs.push(arg.substr(2, arg.len() - 2));
                } else {
                    if i + 1 < arg_count { i = i + 1; extra_libdirs.push(_ks_get_argv(i)); }
                }
                i = i + 1;
                continue;
            }
            // -llib or -l lib
            if arg.starts("-l") {
                if arg.len() > 2 {
                    extra_libs.push(arg.substr(2, arg.len() - 2));
                } else {
                    if i + 1 < arg_count { i = i + 1; extra_libs.push(_ks_get_argv(i)); }
                }
                i = i + 1;
                continue;
            }
            // Positional argument: input file
            if !arg.starts("-") && input_file.len() == 0 {
                input_file = arg;
            }
            i = i + 1;
        }
    }

    if input_file.len() == 0 {
        Print("error: no input file specified");
        print_usage();
        return 1;
    }

    // Derive output name from input if not specified
    if output_file.len() == 0 {
        // Strip .ks extension and path
        let mut stem: Str = input_file;
        // Remove directory path
        let mut last_slash: I32 = -1;
        let mut si: I32 = 0;
        while si < stem.len() {
            if stem.substr(si, 1) == "/" { last_slash = si; }
            si = si + 1;
        }
        if last_slash >= 0 { stem = stem.substr(last_slash + 1, stem.len() - last_slash - 1); }
        // Remove .ks extension
        if stem.ends(".ks") { stem = stem.substr(0, stem.len() - 3); }
        if cpp_only {
            output_file = stem + ".cpp";
        } else {
            output_file = stem;
        }
    }

    // ── Read source file ────────────────────────────────────────────────
    let src_result: Result<Str> = File.read(input_file);
    if !src_result.ok {
        Print("error: cannot read '", input_file, "'");
        return 1;
    }
    let src: Str = src_result.value;

    // ── Compile ─────────────────────────────────────────────────────────
    let mut total_steps: I32 = 5;
    if cpp_only { total_steps = 4; }
    let has_cli: Bool = arg_count >= 2;
    cg_target_label = target_platform;
    let mut t0: F64 = 0.0;

    // Pre-process #include "file.ks" — inline included source before lexing
    // Uses a queue processed front-to-back. Each file's content is PREPENDED
    // to full_src, so the last-processed file ends up first in output.
    // This naturally produces correct dependency order.
    let mut included_files: [Str] = [""];
    let mut included_file_count: I32 = 0;
    let mut inc_queue: [Str] = [""];
    let mut inc_queue_count: I32 = 0;
    // Seed queue with entry file
    inc_queue_count = inc_queue_count + 1;
    inc_queue.push(src);
    let mut full_src: Str = "";
    let mut iq: I32 = 1;
    while iq <= inc_queue_count {
        let cur: Str = inc_queue[iq];
        iq = iq + 1;
        // Scan this text for #include "file.ks" directives
        let mut si: I32 = 0;
        let slen: I32 = cur.len();
        while si < slen {
            // Skip // comments
            if cur.substr(si, 2) == "//" {
                while si < slen && cur.substr(si, 1) != "\n" { si = si + 1; }
                si = si + 1;
                continue;
            }
            // Skip # comments (not #include)
            if cur.substr(si, 1) == "#" && !(si + 7 < slen && cur.substr(si, 8) == "#include") {
                while si < slen && cur.substr(si, 1) != "\n" { si = si + 1; }
                si = si + 1;
                continue;
            }
            if cur.substr(si, 8) == "#include" {
                let mut qi: I32 = si + 8;
                while qi < slen && cur.substr(qi, 1) != "\"" { qi = qi + 1; }
                qi = qi + 1;
                let mut qe: I32 = qi;
                while qe < slen && cur.substr(qe, 1) != "\"" { qe = qe + 1; }
                let inc_path: Str = cur.substr(qi, qe - qi);
                if inc_path.ends(".ks") {
                    // Normalize: strip leading "../segment/" and "./"
                    let mut norm: Str = inc_path;
                    let mut strip_again: Bool = true;
                    while strip_again && norm.len() > 3 && norm.substr(0, 3) == "../" {
                        let mut sl: I32 = 3;
                        while sl < norm.len() && norm.substr(sl, 1) != "/" { sl = sl + 1; }
                        if sl < norm.len() { norm = norm.substr(sl + 1, norm.len() - sl - 1); }
                        else { strip_again = false; }
                    }
                    if norm.len() > 2 && norm.substr(0, 2) == "./" { norm = norm.substr(2, norm.len() - 2); }
                    // Check if already included
                    let mut already: Bool = false;
                    let mut ai: I32 = 1;
                    while ai <= included_file_count {
                        if included_files[ai] == norm { already = true; }
                        ai = ai + 1;
                    }
                    if !already {
                        included_file_count = included_file_count + 1;
                        included_files.push(norm);
                        let inc_result: Result<Str> = File.read(inc_path);
                        if inc_result.ok {
                            inc_queue_count = inc_queue_count + 1;
                            inc_queue.push(inc_result.value);
                        } else {
                            Print("warning: cannot read include '", inc_path, "'");
                        }
                    }
                }
                while si < slen && cur.substr(si, 1) != "\n" { si = si + 1; }
            }
            si = si + 1;
        }
        // Prepend this file to output (last processed = first in output)
        full_src = cur + "\n" + full_src;
    }

    if has_cli { stage_doing(1, total_steps, "Lexing"); t0 = _ks_time_ms(); }
    let ntoks: I32 = lex(full_src);
    if has_cli { stage_ok(1, total_steps, "Lexing", _ks_time_ms() - t0); }

    if has_cli { stage_doing(2, total_steps, "Parsing"); t0 = _ks_time_ms(); }
    let prog: I32 = parse(ntoks);
    included_progs.clear();
    if has_cli { stage_ok(2, total_steps, "Parsing", _ks_time_ms() - t0); }

    if has_cli { stage_doing(3, total_steps, "Type checking"); t0 = _ks_time_ms(); }
    typecheck(prog);
    if has_cli { stage_ok(3, total_steps, "Type checking", _ks_time_ms() - t0); }

    if has_cli { stage_doing(4, total_steps, "Transpiling"); t0 = _ks_time_ms(); }
    let mut cpp_path: Str = "/tmp/konscript_out.cpp";
    if cpp_only { cpp_path = output_file; }

    cg_generate(prog);
    let cg_ok: Bool = cg_write_to_file(cpp_path);
    if !cg_ok {
        Print("error: cannot write C++ to ", cpp_path);
        return 1;
    }
    if has_cli { stage_ok(4, total_steps, "Transpiling", _ks_time_ms() - t0); }

    // If --cpp mode, we're done
    if cpp_only {
        if has_cli { print_success(output_file); }
        return 0;
    }

    // ── Compile C++ to native binary ────────────────────────────────────
    if has_cli { stage_doing(5, total_steps, "Linking"); t0 = _ks_time_ms(); }
    let mut is_windows: Bool = target_platform.starts("windows") || target_platform == "win64";
    let rt_obj: Str = "/tmp/_ks_runtime.o";

    // Detect MXE cross-compiler for Windows targets
    let mut mingw_prefix: Str = "x86_64-w64-mingw32";
    let mut mingw_sysroot: Str = "";
    if is_windows {
        _ks_system("echo $HOME/mxe > /tmp/_ks_mxe_home.txt");
        let home_mxe_result: Result<Str> = File.read("/tmp/_ks_mxe_home.txt");
        let mut home_mxe: Str = "";
        if home_mxe_result.ok { home_mxe = home_mxe_result.value.trim(); }
        let mut mxe_paths: [Str] = ["/usr/lib/mxe", "/opt/mxe"];
        if home_mxe.len() > 0 { mxe_paths.push(home_mxe); }
        let mut mi: I32 = 0;
        while mi < mxe_paths.len() {
            let mxe: Str = mxe_paths[mi];
            if File.exists(mxe + "/usr/bin/x86_64-w64-mingw32.static-g++") {
                mingw_prefix = mxe + "/usr/bin/x86_64-w64-mingw32.static";
                mingw_sysroot = mxe + "/usr/x86_64-w64-mingw32.static";
            }
            mi = mi + 1;
        }
        if mingw_sysroot.len() == 0 {
            Print("warning: MXE not found. Searched: /usr/lib/mxe, /opt/mxe, ", home_mxe);
            Print("  Install MXE: git clone https://github.com/mxe/mxe.git ~/mxe");
            Print("  Build:       cd ~/mxe && make MXE_TARGETS=x86_64-w64-mingw32.static cc");
        }
    }

    // Compile runtime (unless --no-stdlib or Windows — CMake handles Windows)
    if !no_stdlib && !is_windows {
        let mut rt_src: Str = _ks_self_dir() + "/_ks_runtime.c";
        if !File.exists(rt_src) { rt_src = "_ks_runtime.c"; }
        let mut rt_cc: Str = "cc";
        let mut rt_defs: Str = " -D_POSIX_C_SOURCE=200809L";
        if is_windows {
            rt_cc = mingw_prefix + "-gcc";
            rt_defs = " -D_WIN32";
            if mingw_sysroot.len() > 0 {
                rt_defs = rt_defs + " -nostdinc -isystem " + mingw_sysroot + "/include";
            }
        }
        let rt_cmd: Str = rt_cc + " -std=c11 -O2" + rt_defs + " -c " + rt_src + " -o " + rt_obj + " 2>&1";
        let rt_ret: I32 = _ks_system(rt_cmd);
        if rt_ret != 0 {
            let rt_cmd2: Str = rt_cc + " -std=c11 -O2" + rt_defs + " -c " + rt_src + " -o " + rt_obj + " 2>&1";
            _ks_system(rt_cmd2);
        }
    }

    // Build compilation command — pick compiler based on target
    let mut cxx_flags: Str = "g++ -std=c++17 -O2 -DGLM_FORCE_PURE";
    if is_windows {
        cxx_flags = mingw_prefix + "-g++ -std=c++17 -O2 -DGLM_FORCE_PURE -D_WIN32";
        if mingw_sysroot.len() > 0 {
            // Suppress ALL default include paths to prevent host header contamination,
            // then explicitly add back MXE's own C and C++ include directories.
            // Auto-detect GCC version from the lib/gcc directory.
            _ks_system("ls " + mingw_sysroot + "/../lib/gcc/x86_64-w64-mingw32.static/ 2>/dev/null | head -1 > /tmp/_ks_gcc_ver.txt");
            let gcc_ver_result: Result<Str> = File.read("/tmp/_ks_gcc_ver.txt");
            let mut gcc_ver: Str = "";
            if gcc_ver_result.ok { gcc_ver = gcc_ver_result.value.trim(); }
            if gcc_ver.len() > 0 {
                let gcc_base: Str = mingw_sysroot + "/../lib/gcc/x86_64-w64-mingw32.static/" + gcc_ver;
                cxx_flags = cxx_flags + " -nostdinc -nostdinc++";
                cxx_flags = cxx_flags + " -isystem " + gcc_base + "/include/c++";
                cxx_flags = cxx_flags + " -isystem " + gcc_base + "/include/c++/x86_64-w64-mingw32.static";
                cxx_flags = cxx_flags + " -isystem " + gcc_base + "/include";
                cxx_flags = cxx_flags + " -isystem " + gcc_base + "/include-fixed";
                cxx_flags = cxx_flags + " -isystem " + mingw_sysroot + "/include";
            }
        }
        if output_file.len() > 0 && !output_file.ends(".exe") {
            output_file = output_file + ".exe";
        }
    }
    let mut link_flags: Str = "";

    // Add user -I flags
    let mut ii: I32 = 0;
    while ii < extra_includes.len() {
        cxx_flags = cxx_flags + " -I" + extra_includes[ii];
        ii = ii + 1;
    }

    // Add user -L flags
    let mut li: I32 = 0;
    while li < extra_libdirs.len() {
        link_flags = link_flags + " -L" + extra_libdirs[li];
        li = li + 1;
    }

    // Add user -l flags
    let mut ki: I32 = 0;
    while ki < extra_libs.len() {
        link_flags = link_flags + " -l" + extra_libs[ki];
        ki = ki + 1;
    }

    // Engine mode: auto-detect and add engine paths
    let mut engine_dir: Str = "";
    if cg_is_engine {
        if is_windows {
            // Cross-compiling engine games requires the Windows engine toolchain
            // built by: cd tools/KonScript && ./build-engine-lib.sh --windows
            Print("note: cross-compiling engine game for windows64");
        }
        engine_dir = "";
        let self_dir: Str = _ks_self_dir();
        let mut eng_plat: Str = "linux64";
        if is_windows { eng_plat = "windows64"; }
        // Search order: next to binary, CWD toolchain, repo layout, system-wide
        if File.exists(self_dir + "/toolchain/engine/" + eng_plat + "/libKonEngine.a") {
            engine_dir = self_dir + "/toolchain/engine/" + eng_plat;
        }
        if engine_dir.len() == 0 && File.exists("toolchain/engine/" + eng_plat + "/libKonEngine.a") {
            engine_dir = "toolchain/engine/" + eng_plat;
        }
        if engine_dir.len() == 0 && File.exists("../tools/KonScript/toolchain/engine/" + eng_plat + "/libKonEngine.a") {
            engine_dir = "../tools/KonScript/toolchain/engine/" + eng_plat;
        }
        if engine_dir.len() > 0 {
            let inc: Str = engine_dir + "/include";
            cxx_flags = cxx_flags + " -I" + inc;
            cxx_flags = cxx_flags + " -I" + inc + "/glad/include";
            cxx_flags = cxx_flags + " -I" + inc + "/stb";
            if File.exists(inc + "/glm/glm/glm.hpp") { cxx_flags = cxx_flags + " -I" + inc + "/glm"; }
            if File.exists("../../libs/glm/glm/glm.hpp") { cxx_flags = cxx_flags + " -I../../libs/glm"; }
            if File.exists(self_dir + "/../../libs/glm/glm/glm.hpp") { cxx_flags = cxx_flags + " -I" + self_dir + "/../../libs/glm"; }
            link_flags = link_flags + " " + engine_dir + "/libKonEngine.a";
            if File.exists(engine_dir + "/libglfw3.a") {
                link_flags = link_flags + " " + engine_dir + "/libglfw3.a";
            } else {
                link_flags = link_flags + " -lglfw";
            }
            if is_windows {
                link_flags = link_flags + " -lopengl32 -lgdi32 -lwinmm -lws2_32 -lbcrypt -static-libstdc++ -static-libgcc";
            } else {
                link_flags = link_flags + " -lGL -lX11 -lXrandr -lXi -ldl -lpthread";
                // KonPak is always compiled into libKonEngine.a — link crypto libs
                link_flags = link_flags + " -lssl -lcrypto -lz";
            }
        } else {
            // System-wide fallback: check /usr/local for engine install
            if File.exists("/usr/local/lib/libKonEngine.a") {
                cxx_flags = cxx_flags + " -I/usr/local/include";
                // GLM might be in a subdirectory or system path
                if File.exists("/usr/local/include/glm/glm/glm.hpp") { cxx_flags = cxx_flags + " -I/usr/local/include/glm"; }
                if File.exists("/usr/include/glm/glm.hpp") { cxx_flags = cxx_flags + " -I/usr/include"; }
                link_flags = link_flags + " /usr/local/lib/libKonEngine.a";
                if File.exists("/usr/local/lib/libglfw3.a") {
                    link_flags = link_flags + " /usr/local/lib/libglfw3.a";
                } else {
                    link_flags = link_flags + " -lglfw";
                }
            } else if File.exists("/usr/local/include/KonEngine.hpp") {
                // Headers installed but no static lib — try dynamic linking
                cxx_flags = cxx_flags + " -I/usr/local/include";
                link_flags = link_flags + " -lKonEngine -lglfw";
            } else {
                Print("warning: engine toolchain not found — run build-engine-lib.sh");
            }
            if is_windows {
                link_flags = link_flags + " -lopengl32 -lgdi32 -lwinmm -lws2_32 -lbcrypt -static-libstdc++ -static-libgcc";
            } else {
                link_flags = link_flags + " -lGL -lX11 -lXrandr -lXi -ldl -lpthread";
                link_flags = link_flags + " -lssl -lcrypto -lz";
            }
        }
    }

    // Always add -lm
    if !is_windows { link_flags = link_flags + " -lm"; }

    // KonPak support: add define and link libraries
    if pack_support || src.contains("AssetManager") {
        cxx_flags = cxx_flags + " -DKON_USE_PACK";
        if !is_windows {
            link_flags = link_flags + " -lssl -lcrypto -lz";
        }
    }

    // Build final command
    let mut compile_src: Str = cpp_path;
    if !no_stdlib { compile_src = compile_src + " " + rt_obj; }

    // Windows cross-compile: use CMake with MXE toolchain (only way to isolate headers)
    if is_windows && mingw_sysroot.len() > 0 {
        if !output_file.ends(".exe") { output_file = output_file + ".exe"; }
        let bdir: Str = "/tmp/_ks_win_build";
        _ks_system("rm -rf " + bdir + " && mkdir -p " + bdir);
        // Write toolchain file
        let mut tc: Str = "set(CMAKE_SYSTEM_NAME Windows)\n";
        tc = tc + "set(CMAKE_C_COMPILER " + mingw_prefix + "-gcc)\n";
        tc = tc + "set(CMAKE_CXX_COMPILER " + mingw_prefix + "-g++)\n";
        tc = tc + "set(CMAKE_RC_COMPILER " + mingw_prefix + "-windres)\n";
        tc = tc + "set(CMAKE_FIND_ROOT_PATH " + mingw_sysroot + ")\n";
        tc = tc + "set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)\n";
        tc = tc + "set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)\n";
        tc = tc + "set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)\n";
        File.write(bdir + "/toolchain.cmake", tc);
        // Write CMakeLists.txt
        let mut cm: Str = "cmake_minimum_required(VERSION 3.10)\n";
        cm = cm + "project(KsGame CXX C)\n";
        cm = cm + "set(CMAKE_CXX_STANDARD 17)\n";
        cm = cm + "add_definitions(-DGLM_FORCE_PURE -D_WIN32)\n";
        if pack_support || src.contains("AssetManager") {
            cm = cm + "add_definitions(-DKON_USE_PACK)\n";
        }
        // Source files — include runtime .c source directly (CMake compiles it)
        let mut rt_src_path: Str = _ks_self_dir() + "/_ks_runtime.c";
        if !File.exists(rt_src_path) { rt_src_path = "_ks_runtime.c"; }
        cm = cm + "add_executable(game " + cpp_path;
        if !no_stdlib { cm = cm + " " + rt_src_path; }
        cm = cm + ")\n";
        if !no_stdlib {
            cm = cm + "set_source_files_properties(" + rt_src_path + " PROPERTIES LANGUAGE C)\n";
            cm = cm + "target_compile_definitions(game PRIVATE $<$<COMPILE_LANGUAGE:C>:_WIN32>)\n";
        }
        // Engine includes and libs
        if cg_is_engine {
            let mut eng: Str = engine_dir;
            if eng.len() > 0 {
                let einc: Str = eng + "/include";
                cm = cm + "target_include_directories(game PRIVATE " + einc + " " + einc + "/glad/include " + einc + "/stb";
                if File.exists(einc + "/glm/glm/glm.hpp") { cm = cm + " " + einc + "/glm"; }
                cm = cm + ")\n";
                // When --pack is used, compile asset_manager.cpp from source
                // with KON_USE_PACK so it overrides the one in the pre-built lib
                if pack_support || src.contains("AssetManager") {
                    let mut am_src: Str = "";
                    let mut engine_root: Str = _ks_self_dir() + "/../..";
                    if File.exists(engine_root + "/src/asset_manager.cpp") {
                        am_src = engine_root + "/src/asset_manager.cpp";
                    } else if File.exists("../../src/asset_manager.cpp") {
                        am_src = "../../src/asset_manager.cpp";
                        engine_root = "../..";
                    }
                    if am_src.len() > 0 {
                        cm = cm + "target_sources(game PRIVATE \"" + am_src + "\")\n";
                        cm = cm + "target_include_directories(game PRIVATE \"" + engine_root + "/src\")\n";
                        // Add miniz for compression support on Windows
                        let miniz_dir: Str = engine_root + "/tools/KonPaktor/third_party";
                        if File.exists(miniz_dir + "/miniz.h") {
                            cm = cm + "target_sources(game PRIVATE \"" + engine_root + "/src/miniz_impl.cpp\")\n";
                            cm = cm + "target_include_directories(game PRIVATE \"" + miniz_dir + "\")\n";
                            cm = cm + "target_compile_definitions(game PRIVATE KONPAK_USE_MINIZ)\n";
                        }
                    }
                }
                cm = cm + "target_link_libraries(game PRIVATE " + eng + "/libKonEngine.a";
                if File.exists(eng + "/libglfw3.a") { cm = cm + " " + eng + "/libglfw3.a"; }
                cm = cm + " opengl32 gdi32 winmm ws2_32 bcrypt -static-libstdc++ -static-libgcc)\n";
            } else {
                // No pre-built windows64 toolchain — try building engine from repo source
                let mut engine_root: Str = _ks_self_dir() + "/../..";
                if !File.exists(engine_root + "/src/KonEngine.hpp") {
                    if File.exists("../../src/KonEngine.hpp") { engine_root = "../.."; }
                    else if File.exists("../src/KonEngine.hpp") { engine_root = ".."; }
                }
                if File.exists(engine_root + "/src/KonEngine.hpp") {
                    Print("note: no pre-built windows64 toolchain — building engine from source");
                    cm = cm + "file(GLOB_RECURSE ENGINE_SRCS\n";
                    cm = cm + "  \"" + engine_root + "/src/window/*.cpp\"\n";
                    cm = cm + "  \"" + engine_root + "/src/renderer/opengl/*.cpp\"\n";
                    cm = cm + "  \"" + engine_root + "/src/time/*.cpp\"\n";
                    cm = cm + "  \"" + engine_root + "/src/input/*.cpp\"\n";
                    cm = cm + "  \"" + engine_root + "/src/font/*.cpp\"\n";
                    cm = cm + "  \"" + engine_root + "/src/audio/*.cpp\"\n";
                    cm = cm + "  \"" + engine_root + "/src/collision/*.cpp\"\n";
                    cm = cm + "  \"" + engine_root + "/src/animation/*.cpp\"\n";
                    cm = cm + ")\n";
                    cm = cm + "list(APPEND ENGINE_SRCS \"" + engine_root + "/src/asset_manager.cpp\")\n";
                    cm = cm + "list(APPEND ENGINE_SRCS \"" + engine_root + "/src/glad/src/glad.c\")\n";
                    cm = cm + "list(FILTER ENGINE_SRCS EXCLUDE REGEX \"anim_compiler|imgui\")\n";
                    cm = cm + "target_sources(game PRIVATE ${ENGINE_SRCS})\n";
                    cm = cm + "target_include_directories(game PRIVATE";
                    cm = cm + " \"" + engine_root + "/src\"";
                    cm = cm + " \"" + engine_root + "/src/glad/include\"";
                    cm = cm + " \"" + engine_root + "/src/stb\"";
                    cm = cm + " \"" + engine_root + "/libs/glfw/include\"";
                    cm = cm + " \"" + engine_root + "/libs/glm\"";
                    cm = cm + ")\n";
                    // Build GLFW from source via add_subdirectory
                    cm = cm + "set(GLFW_BUILD_DOCS OFF CACHE BOOL \"\" FORCE)\n";
                    cm = cm + "set(GLFW_BUILD_TESTS OFF CACHE BOOL \"\" FORCE)\n";
                    cm = cm + "set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL \"\" FORCE)\n";
                    cm = cm + "add_subdirectory(\"" + engine_root + "/libs/glfw\" \"${CMAKE_BINARY_DIR}/glfw_build\")\n";
                    cm = cm + "target_link_libraries(game PRIVATE glfw opengl32 gdi32 winmm ws2_32 bcrypt -static-libstdc++ -static-libgcc)\n";
                } else {
                    Print("error: Windows engine toolchain not found.");
                    Print("  Option 1: cd tools/KonScript && ./build-engine-lib.sh --windows");
                    Print("  Option 2: run from the KonEngine repo directory.");
                    return 1;
                }
            }
        }
        File.write(bdir + "/CMakeLists.txt", cm);
        // Build
        let cmake_cmd: Str = "cd " + bdir + " && cmake -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake -DCMAKE_BUILD_TYPE=Release . 2>&1 && cmake --build . 2>&1";
        let cmake_ret: I32 = _ks_system(cmake_cmd);
        if cmake_ret == 0 {
            _ks_system("cp " + bdir + "/game.exe " + output_file);
            if has_cli { stage_ok(5, total_steps, "Linking", _ks_time_ms() - t0); }
            if has_cli { print_success(output_file); }
        } else {
            Print("error: Windows cross-compilation failed");
        }
        _ks_system("rm -rf " + bdir);
        return 0;
    }

    // Compile C++ + link (Linux / native)
    let cxx_cmd: Str = cxx_flags + " -o " + output_file + " " + compile_src + link_flags + " 2>&1";
    let cxx_ret: I32 = _ks_system(cxx_cmd);
    if cxx_ret != 0 {
        // Fallback: try clang (but use cross-compiler for windows)
        let mut fallback_cxx: Str = "clang++ -std=c++17 -O2 -DGLM_FORCE_PURE";
        if is_windows { fallback_cxx = "x86_64-w64-mingw32-g++ -std=c++17 -O2 -DGLM_FORCE_PURE"; }
        let cxx_cmd2: Str = fallback_cxx + " -o " + output_file + " " + compile_src + link_flags + " 2>&1";
        let cxx_ret2: I32 = _ks_system(cxx_cmd2);
        if cxx_ret2 != 0 {
            Print("error: compilation failed");
            return 1;
        }
    }

    if has_cli { stage_ok(5, total_steps, "Linking", _ks_time_ms() - t0); }

    if has_cli { print_success(output_file); }
    return 0;
}

