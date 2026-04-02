/* konscript.ks — KonScript self-hosted compiler
   Phase 1: Lexer
   Compiles and runs under: konscript --check / ksc */

enum TokenKind {
    /* literals */
    Int, Float, Str, Bool, Null, Ident,
    /* keywords */
    KwLet, KwMut, KwConst, KwFunc, KwReturn,
    KwIf, KwElse, KwWhile, KwLoop, KwFor, KwIn,
    KwBreak, KwContinue, KwStruct, KwEnum, KwClass,
    KwInterface, KwImplements, KwPub, KwAs, KwSpawn,
    KwWait, KwExtern, KwSelf,
    /* symbols */
    LParen, RParen, LBrace, RBrace, LBracket, RBracket,
    Semicolon, Colon, Comma, Dot, DotDot, Arrow,
    Hash, Bang, Question, Star, And,
    /* operators */
    Plus, Minus, Slash, Percent,
    Eq, NotEq, Lt, Gt, LtEq, GtEq,
    Assign, PlusEq, MinusEq, StarEq,
    And2, Or2,
    /* special */
    Eof, Error,
}

struct Token {
    let kind:  TokenKind;
    let value: Str;
    let line:  I32;
    let col:   I32;
}

class Lexer {
    let mut src:    Str = "";
    let mut pos:    I32 = 0;
    let mut line:   I32 = 1;
    let mut col:    I32 = 1;
    let mut tokens: [Token] = [];

    func init(mut self, source: Str) {
        self.src    = source;
        self.pos    = 0;
        self.line   = 1;
        self.col    = 1;
        self.tokens = [];
    }

    func atEnd(self) -> Bool {
        return self.pos >= self.src.len();
    }

    func peek(self) -> Str {
        if self.atEnd() { return ""; }
        return self.src.charAt(self.pos);
    }

    func peek2(self) -> Str {
        if self.pos + 1 >= self.src.len() { return ""; }
        return self.src.charAt(self.pos + 1);
    }

    func advance(mut self) -> Str {
        let ch: Str = self.peek();
        self.pos += 1;
        if ch == "\n" {
            self.line += 1;
            self.col = 1;
        } else {
            self.col += 1;
        }
        return ch;
    }

    func emit(mut self, kind: TokenKind, value: Str, l: I32, c: I32) {
        let t: Token = Token { kind: kind, value: value, line: l, col: c };
        self.tokens.push(t);
    }

    func skipWhitespace(mut self) {
        while !self.atEnd() {
            let ch: Str = self.peek();
            if ch == " " || ch == "\t" || ch == "\r" || ch == "\n" {
                self.advance();
            } else if ch == "/" && self.peek2() == "/" {
                while !self.atEnd() && self.peek() != "\n" {
                    self.advance();
                }
            } else {
                break;
            }
        }
    }

    func readIdent(mut self) -> Str {
        let mut s: Str = "";
        while !self.atEnd() {
            let ch: Str = self.peek();
            if ch.isAlpha() || ch.isDigit() || ch == "_" {
                s = s + self.advance();
            } else {
                break;
            } }
        return s;
    }

    func readNumber(mut self) -> Str {
        let mut s: Str = "";
        while !self.atEnd() && self.peek().isDigit() {
            s = s + self.advance();
        }
        if !self.atEnd() && self.peek() == "." && self.peek2().isDigit() {
            s = s + self.advance();
            while !self.atEnd() && self.peek().isDigit() {
                s = s + self.advance();
            }
        }
        return s;
    }

    func readString(mut self) -> Str {
        self.advance();
        let mut s: Str = "";
        while !self.atEnd() && self.peek() != "\"" {
            let ch: Str = self.advance();
            if ch == "\\" {
                let esc: Str = self.advance();
                if esc == "n"  { s = s + "\n"; }
                else if esc == "t"  { s = s + "\t"; }
                else if esc == "\\" { s = s + "\\"; }
                else { s = s + esc; }
            } else {
                s = s + ch;
            }
        }
        if !self.atEnd() { self.advance(); }
        return s;
    }

    func keyword(self, s: Str) -> TokenKind {
        if s == "let"        { return TokenKind::KwLet; }
        if s == "mut"        { return TokenKind::KwMut; }
        if s == "const"      { return TokenKind::KwConst; }
        if s == "func"       { return TokenKind::KwFunc; }
        if s == "return"     { return TokenKind::KwReturn; }
        if s == "if"         { return TokenKind::KwIf; }
        if s == "else"       { return TokenKind::KwElse; }
        if s == "while"      { return TokenKind::KwWhile; }
        if s == "loop"       { return TokenKind::KwLoop; }
        if s == "for"        { return TokenKind::KwFor; }
        if s == "in"         { return TokenKind::KwIn; }
        if s == "break"      { return TokenKind::KwBreak; }
        if s == "continue"   { return TokenKind::KwContinue; }
        if s == "struct"     { return TokenKind::KwStruct; }
        if s == "enum"       { return TokenKind::KwEnum; }
        if s == "class"      { return TokenKind::KwClass; }
        if s == "interface"  { return TokenKind::KwInterface; }
        if s == "implements" { return TokenKind::KwImplements; }
        if s == "pub"        { return TokenKind::KwPub; }
        if s == "as"         { return TokenKind::KwAs; }
        if s == "spawn"      { return TokenKind::KwSpawn; }
        if s == "wait"       { return TokenKind::KwWait; }
        if s == "extern"     { return TokenKind::KwExtern; }
        if s == "self"       { return TokenKind::KwSelf; }
        if s == "true"       { return TokenKind::Bool; }
        if s == "false"      { return TokenKind::Bool; }
        if s == "null"       { return TokenKind::Null; }
        return TokenKind::Ident;
    }

    func tokenize(mut self) -> [Token] {
        while !self.atEnd() {
            self.skipWhitespace();
            if self.atEnd() { break; }

            let l: I32 = self.line;
            let c: I32 = self.col;
            let ch: Str = self.peek();

            if ch.isAlpha() || ch == "_" {
                let word: Str = self.readIdent();
                let kind: TokenKind = self.keyword(word);
                self.emit(kind, word, l, c);
            } else if ch.isDigit() {
                let num: Str = self.readNumber();
                if num.contains(".") {
                    self.emit(TokenKind::Float, num, l, c);
                } else {
                    self.emit(TokenKind::Int, num, l, c);
                }
            } else if ch == "\"" {
                let s: Str = self.readString();
                self.emit(TokenKind::Str, s, l, c);
            } else if ch == "(" { self.advance(); self.emit(TokenKind::LParen,    "(", l, c); }
            else if ch == ")" { self.advance(); self.emit(TokenKind::RParen,    ")", l, c); }
            else if ch == "[" { self.advance(); self.emit(TokenKind::LBracket,  "[", l, c); }
            else if ch == "]" { self.advance(); self.emit(TokenKind::RBracket,  "]", l, c); }
            else if ch == ";" { self.advance(); self.emit(TokenKind::Semicolon, ";", l, c); }
            else if ch == "," { self.advance(); self.emit(TokenKind::Comma,     ",", l, c); }
            else if ch == "#" { self.advance(); self.emit(TokenKind::Hash,      "#", l, c); }
            else if ch == "?" { self.advance(); self.emit(TokenKind::Question,  "?", l, c); }
            else if ch == "{" { self.advance(); self.emit(TokenKind::LBrace,    "LB", l, c); }
            else if ch == "}" { self.advance(); self.emit(TokenKind::RBrace,    "RB", l, c); }
            else if ch == "+" {
                self.advance();
                if self.peek() == "=" { self.advance(); self.emit(TokenKind::PlusEq,  "+=", l, c); }
                else { self.emit(TokenKind::Plus,  "+", l, c); }
            } else if ch == "-" {
                self.advance();
                if self.peek() == ">" { self.advance(); self.emit(TokenKind::Arrow,   "->", l, c); }
                else if self.peek() == "=" { self.advance(); self.emit(TokenKind::MinusEq, "-=", l, c); }
                else { self.emit(TokenKind::Minus, "-", l, c); }
            } else if ch == "*" {
                self.advance();
                if self.peek() == "=" { self.advance(); self.emit(TokenKind::StarEq, "*=", l, c); }
                else { self.emit(TokenKind::Star, "*", l, c); }
            } else if ch == "/" {
                self.advance();
                self.emit(TokenKind::Slash, "/", l, c);
            } else if ch == "%" {
                self.advance();
                self.emit(TokenKind::Percent, "%", l, c);
            } else if ch == "=" {
                self.advance();
                if self.peek() == "=" { self.advance(); self.emit(TokenKind::Eq,     "==", l, c); }
                else { self.emit(TokenKind::Assign, "=", l, c); }
            } else if ch == "!" {
                self.advance();
                if self.peek() == "=" { self.advance(); self.emit(TokenKind::NotEq, "!=", l, c); }
                else { self.emit(TokenKind::Bang, "!", l, c); }
            } else if ch == "<" {
                self.advance();
                if self.peek() == "=" { self.advance(); self.emit(TokenKind::LtEq, "<=", l, c); }
                else { self.emit(TokenKind::Lt, "<", l, c); }
            } else if ch == ">" {
                self.advance();
                if self.peek() == "=" { self.advance(); self.emit(TokenKind::GtEq, ">=", l, c); }
                else { self.emit(TokenKind::Gt, ">", l, c); }
            } else if ch == "&" {
                self.advance();
                if self.peek() == "&" { self.advance(); self.emit(TokenKind::And2, "&&", l, c); }
                else { self.emit(TokenKind::And, "&", l, c); }
            } else if ch == "|" {
                self.advance();
                if self.peek() == "|" { self.advance(); self.emit(TokenKind::Or2, "||", l, c); }
                else { self.advance(); }
            } else if ch == ":" {
                self.advance();
                self.emit(TokenKind::Colon, ":", l, c);
            } else if ch == "." {
                self.advance();
                if self.peek() == "." { self.advance(); self.emit(TokenKind::DotDot, "..", l, c); }
                else { self.emit(TokenKind::Dot, ".", l, c); }
            } else {
                self.advance();
            }
        }
        self.emit(TokenKind::Eof, "EOF", self.line, self.col);
        return self.tokens;
    }
}


/* -----------------------------------------------------------------------
   Phase 2: Token stream reader
   ----------------------------------------------------------------------- */

class TokenStream {
    let mut tokens: [Token] = [];
    let mut pos:    I32 = 0;

    func init(mut self, toks: [Token]) {
        self.tokens = toks;
        self.pos = 0;
    }

    func peek(self) -> Token {
        return self.tokens[self.pos];
    }

    func advance(mut self) -> Token {
        let t: Token = self.tokens[self.pos];
        if self.pos + 1 < self.tokens.len() {
            self.pos += 1;
        }
        return t;
    }

    func check(self, kind: TokenKind) -> Bool {
        return self.tokens[self.pos].kind == kind;
    }

    func consume(mut self, kind: TokenKind) -> Bool {
        if self.tokens[self.pos].kind == kind {
            self.pos += 1;
            return true;
        }
        return false;
    }

    func expect(mut self, kind: TokenKind) -> Token {
        let t: Token = self.tokens[self.pos];
        if self.pos + 1 < self.tokens.len() {
            self.pos += 1;
        }
        return t;
    }

    func atEnd(self) -> Bool {
        return self.tokens[self.pos].kind == TokenKind::Eof;
    }
}

/* -----------------------------------------------------------------------
   Phase 3: Expression parser — returns text AST dump
   ----------------------------------------------------------------------- */

class Parser {
    let mut ts: TokenStream = TokenStream { tokens: [], pos: 0 };

    func init(mut self, stream: TokenStream) {
        self.ts = stream;
    }

    func parseExpr(mut self) -> Str {
        return self.parseComparison();
    }

    func parseComparison(mut self) -> Str {
        let mut left: Str = self.parseAddSub();
        while self.ts.check(TokenKind::Eq)   || self.ts.check(TokenKind::NotEq) ||
              self.ts.check(TokenKind::Lt)   || self.ts.check(TokenKind::Gt)    ||
              self.ts.check(TokenKind::LtEq) || self.ts.check(TokenKind::GtEq) {
            let op: Token = self.ts.advance();
            let right: Str = self.parseAddSub();
            left = "(" + left + " " + op.value + " " + right + ")";
        }
        return left;
    }

    func parseAddSub(mut self) -> Str {
        let mut left: Str = self.parseMulDiv();
        while self.ts.check(TokenKind::Plus) || self.ts.check(TokenKind::Minus) {
            let op: Token = self.ts.advance();
            let right: Str = self.parseMulDiv();
            left = "(" + left + " " + op.value + " " + right + ")";
        }
        return left;
    }

    func parseMulDiv(mut self) -> Str {
        let mut left: Str = self.parseUnary();
        while self.ts.check(TokenKind::Star) || self.ts.check(TokenKind::Slash) {
            let op: Token = self.ts.advance();
            let right: Str = self.parseUnary();
            left = "(" + left + " " + op.value + " " + right + ")";
        }
        return left;
    }

    func parseUnary(mut self) -> Str {
        if self.ts.check(TokenKind::Minus) || self.ts.check(TokenKind::Bang) {
            let op: Token = self.ts.advance();
            let operand: Str = self.parseUnary();
            return "(" + op.value + operand + ")";
        }
        return self.parseCall();
    }

    func parseCall(mut self) -> Str {
        let mut expr: Str = self.parsePrimary();
        if self.ts.consume(TokenKind::LParen) {
            let mut args: Str = "";
            let mut first: Bool = true;
            while !self.ts.check(TokenKind::RParen) && !self.ts.atEnd() {
                if !first { args = args + ", "; }
                first = false;
                args = args + self.parseExpr();
                self.ts.consume(TokenKind::Comma);
            }
            self.ts.consume(TokenKind::RParen);
            expr = expr + "(" + args + ")";
        }
        return expr;
    }

    func parsePrimary(mut self) -> Str {
        let t: Token = self.ts.peek();
        if t.kind == TokenKind::Int   { self.ts.advance(); return t.value; }
        if t.kind == TokenKind::Float { self.ts.advance(); return t.value; }
        if t.kind == TokenKind::Bool  { self.ts.advance(); return t.value; }
        if t.kind == TokenKind::Ident { self.ts.advance(); return t.value; }
        if t.kind == TokenKind::Str   { self.ts.advance(); return t.value; }
        if self.ts.consume(TokenKind::LParen) {
            let inner: Str = self.parseExpr();
            self.ts.consume(TokenKind::RParen);
            return "(" + inner + ")";
        }
        self.ts.advance();
        return "?";
    }
}

/* -----------------------------------------------------------------------
   Phase 4: Statement parser — text AST dump
   ----------------------------------------------------------------------- */

class StmtParser {
    let mut ts:     TokenStream = TokenStream { tokens: [], pos: 0 };
    let mut ep:     Parser      = Parser { ts: TokenStream { tokens: [], pos: 0 } };
    let mut indent: I32 = 0;

    func init(mut self, stream: TokenStream) {
        self.ts = stream;
        self.ep.ts = stream;
    }

    func pad(self) -> Str {
        let mut s: Str = "";
        let mut i: I32 = 0;
        while i < self.indent {
            s = s + "  ";
            i += 1;
        }
        return s;
    }

    func parseExpr(mut self) -> Str {
        self.ep.ts = self.ts;
        let result: Str = self.ep.parseExpr();
        self.ts = self.ep.ts;
        return result;
    }

    func parseBlock(mut self) -> Str {
        self.ts.expect(TokenKind::LBrace);
        let mut out: Str = "";
        self.indent += 1;
        while !self.ts.check(TokenKind::RBrace) && !self.ts.atEnd() {
            out = out + self.parseStmt();
        }
        self.ts.expect(TokenKind::RBrace);
        self.indent -= 1;
        return out;
    }

    func skipType(mut self) {
        while !self.ts.check(TokenKind::Assign) &&
              !self.ts.check(TokenKind::Semicolon) &&
              !self.ts.check(TokenKind::LBrace) &&
              !self.ts.check(TokenKind::KwIn) &&
              !self.ts.atEnd() {
            self.ts.advance();
        }
    }

    func skipParens(mut self) {
        self.ts.consume(TokenKind::LParen);
        let mut depth: I32 = 1;
        while depth > 0 && !self.ts.atEnd() {
            if self.ts.check(TokenKind::LParen) { depth += 1; }
            if self.ts.check(TokenKind::RParen) { depth -= 1; }
            self.ts.advance();
        }
    }

    func parseStmt(mut self) -> Str {
        let t: Token = self.ts.peek();
        if t.kind == TokenKind::KwLet    { return self.parseLet(); }
        if t.kind == TokenKind::KwReturn { return self.parseReturn(); }
        if t.kind == TokenKind::KwIf     { return self.parseIf(); }
        if t.kind == TokenKind::KwWhile  { return self.parseWhile(); }
        if t.kind == TokenKind::KwFor    { return self.parseForIn(); }
        if t.kind == TokenKind::KwFunc   { return self.parseFuncDecl(); }
        let expr: Str = self.parseExpr();
        self.ts.consume(TokenKind::Semicolon);
        return self.pad() + expr + ";\n";
    }

    func parseLet(mut self) -> Str {
        self.ts.advance();
        let mut isMut: Bool = false;
        if self.ts.check(TokenKind::KwMut) { self.ts.advance(); isMut = true; }
        let name: Token = self.ts.expect(TokenKind::Ident);
        if self.ts.consume(TokenKind::Colon) { self.skipType(); }
        let mut val: Str = "";
        if self.ts.consume(TokenKind::Assign) { val = self.parseExpr(); }
        self.ts.consume(TokenKind::Semicolon);
        let mut kw: Str = "let ";
        if isMut { kw = "let mut "; }
        return self.pad() + kw + name.value + " = " + val + ";\n";
    }

    func parseReturn(mut self) -> Str {
        self.ts.advance();
        if self.ts.check(TokenKind::Semicolon) { self.ts.advance(); return self.pad() + "return;\n"; }
        let val: Str = self.parseExpr();
        self.ts.consume(TokenKind::Semicolon);
        return self.pad() + "return " + val + ";\n";
    }

    func parseIf(mut self) -> Str {
        self.ts.advance();
        let cond: Str = self.parseExpr();
        let body: Str = self.parseBlock();
        let mut out: Str = self.pad() + "if " + cond + " {\n" + body;
        if self.ts.check(TokenKind::KwElse) {
            self.ts.advance();
            if self.ts.check(TokenKind::KwIf) {
                return out + self.pad() + "} else " + self.parseIf();
            }
            let eb: Str = self.parseBlock();
            out = out + self.pad() + "} else {\n" + eb;
        }
        return out + self.pad() + "}\n";
    }

    func parseWhile(mut self) -> Str {
        self.ts.advance();
        let cond: Str = self.parseExpr();
        let body: Str = self.parseBlock();
        return self.pad() + "while " + cond + " {\n" + body + self.pad() + "}\n";
    }

    func parseForIn(mut self) -> Str {
        self.ts.advance();
        let var: Token = self.ts.expect(TokenKind::Ident);
        if self.ts.check(TokenKind::Colon) { self.ts.advance(); self.skipType(); }
        self.ts.consume(TokenKind::KwIn);
        let iter: Str = self.parseExpr();
        let body: Str = self.parseBlock();
        return self.pad() + "for " + var.value + " in " + iter + " {\n" + body + self.pad() + "}\n";
    }

    func parseFuncDecl(mut self) -> Str {
        self.ts.advance();
        let name: Token = self.ts.expect(TokenKind::Ident);
        self.skipParens();
        if self.ts.check(TokenKind::Arrow) {
            self.ts.advance();
            self.skipType();
        }
        let body: Str = self.parseBlock();
        return self.pad() + "func " + name.value + " {\n" + body + self.pad() + "}\n";
    }

    func parseProgram(mut self) -> Str {
        let mut out: Str = "";
        while !self.ts.atEnd() { out = out + self.parseStmt(); }
        return out;
    }
}

/* -----------------------------------------------------------------------
   Phase 5: Symbol table + type tracker
   A flat scope chain: each scope is an index into a parallel pair of
   arrays (names / types).  No heap allocation — all fixed arrays.
   ----------------------------------------------------------------------- */

struct Symbol {
    let name:    Str;
    let typeName: Str;
    let isMut:   Bool;
    let scope:   I32;
}

class SymbolTable {
    let mut syms:       [Symbol] = [];
    let mut scopeDepth: I32 = 0;

    func pushScope(mut self) {
        self.scopeDepth += 1;
    }

    func popScope(mut self) {
        let mut i: I32 = self.syms.len() - 1;
        while i >= 0 {
            if self.syms[i].scope == self.scopeDepth {
                /* remove by swapping with last and shrinking */
                let last: I32 = self.syms.len() - 1;
                if i != last {
                    self.syms[i] = self.syms[last];
                }
                self.syms.pop();
            }
            i -= 1;
        }
        self.scopeDepth -= 1;
    }

    func define(mut self, name: Str, typeName: Str, isMut: Bool) {
        let s: Symbol = Symbol {
            name: name,
            typeName: typeName,
            isMut: isMut,
            scope: self.scopeDepth,
        };
        self.syms.push(s);
    }

    func lookup(self, name: Str) -> Str {
        let mut i: I32 = self.syms.len() - 1;
        while i >= 0 {
            if self.syms[i].name == name {
                return self.syms[i].typeName;
            }
            i -= 1;
        }
        return "unknown";
    }

    func isMut(self, name: Str) -> Bool {
        let mut i: I32 = self.syms.len() - 1;
        while i >= 0 {
            if self.syms[i].name == name {
                return self.syms[i].isMut;
            }
            i -= 1;
        }
        return false;
    }

    func exists(self, name: Str) -> Bool {
        let mut i: I32 = self.syms.len() - 1;
        while i >= 0 {
            if self.syms[i].name == name { return true; }
            i -= 1;
        }
        return false;
    }
}

/* -----------------------------------------------------------------------
   Phase 5b: Type checker — walks the token stream, tracks types,
   reports errors to a string buffer
   ----------------------------------------------------------------------- */

class TypeChecker {
    let mut ts:     TokenStream  = TokenStream  { tokens: [], pos: 0 };
    let mut ep:     Parser       = Parser       { ts: TokenStream { tokens: [], pos: 0 } };
    let mut syms:   SymbolTable  = SymbolTable  { syms: [], scopeDepth: 0 };
    let mut errors: [Str]        = [];

    func init(mut self, stream: TokenStream) {
        self.ts = stream;
        self.ep.ts = stream;
    }

    func error(mut self, msg: Str) {
        let tok: Token = self.ts.peek();
        let loc: Str = ToString(tok.line) + ":" + ToString(tok.col);
        self.errors.push(loc + ": error: " + msg);
    }

    func syncExpr(mut self) -> Str {
        self.ep.ts = self.ts;
        let r: Str = self.ep.parseExpr();
        self.ts = self.ep.ts;
        return r;
    }

    func skipUntil(mut self, kind: TokenKind) {
        while !self.ts.check(kind) && !self.ts.atEnd() {
            self.ts.advance();
        }
    }

    func parseTypeStr(mut self) -> Str {
        let mut t: Str = "";
        while !self.ts.check(TokenKind::Assign) &&
              !self.ts.check(TokenKind::Semicolon) &&
              !self.ts.check(TokenKind::LBrace) &&
              !self.ts.check(TokenKind::KwIn) &&
              !self.ts.check(TokenKind::Comma) &&
              !self.ts.check(TokenKind::RParen) &&
              !self.ts.atEnd() {
            t = t + self.ts.advance().value;
        }
        return t;
    }

    func checkBlock(mut self) {
        self.ts.expect(TokenKind::LBrace);
        self.syms.pushScope();
        while !self.ts.check(TokenKind::RBrace) && !self.ts.atEnd() {
            self.checkStmt();
        }
        self.ts.expect(TokenKind::RBrace);
        self.syms.popScope();
    }

    func checkStmt(mut self) {
        let t: Token = self.ts.peek();
        if t.kind == TokenKind::KwLet    { self.checkLet(); return; }
        if t.kind == TokenKind::KwReturn { self.checkReturn(); return; }
        if t.kind == TokenKind::KwIf     { self.checkIf(); return; }
        if t.kind == TokenKind::KwWhile  { self.checkWhile(); return; }
        if t.kind == TokenKind::KwFor    { self.checkForIn(); return; }
        if t.kind == TokenKind::KwFunc   { self.checkFuncDecl(); return; }
        /* expression statement — just consume it */
        self.syncExpr();
        self.ts.consume(TokenKind::Semicolon);
    }

    func checkLet(mut self) {
        self.ts.advance(); /* let */
        let mut isMut: Bool = false;
        if self.ts.check(TokenKind::KwMut) { self.ts.advance(); isMut = true; }
        let name: Token = self.ts.expect(TokenKind::Ident);
        let mut typeName: Str = "unknown";
        if self.ts.consume(TokenKind::Colon) {
            typeName = self.parseTypeStr();
        }
        if self.ts.consume(TokenKind::Assign) {
            self.syncExpr();
        }
        self.ts.consume(TokenKind::Semicolon);
        if self.syms.exists(name.value) {
            self.error("redefinition of '" + name.value + "'");
        }
        self.syms.define(name.value, typeName, isMut);
    }

    func checkReturn(mut self) {
        self.ts.advance();
        if !self.ts.check(TokenKind::Semicolon) { self.syncExpr(); }
        self.ts.consume(TokenKind::Semicolon);
    }

    func checkIf(mut self) {
        self.ts.advance();
        self.syncExpr();
        self.checkBlock();
        if self.ts.check(TokenKind::KwElse) {
            self.ts.advance();
            if self.ts.check(TokenKind::KwIf) { self.checkIf(); return; }
            self.checkBlock();
        }
    }

    func checkWhile(mut self) {
        self.ts.advance();
        self.syncExpr();
        self.checkBlock();
    }

    func checkForIn(mut self) {
        self.ts.advance();
        let var: Token = self.ts.expect(TokenKind::Ident);
        let mut typeName: Str = "unknown";
        if self.ts.check(TokenKind::Colon) {
            self.ts.advance();
            typeName = self.parseTypeStr();
        }
        self.ts.consume(TokenKind::KwIn);
        self.syncExpr();
        self.syms.pushScope();
        self.syms.define(var.value, typeName, false);
        self.ts.expect(TokenKind::LBrace);
        while !self.ts.check(TokenKind::RBrace) && !self.ts.atEnd() {
            self.checkStmt();
        }
        self.ts.expect(TokenKind::RBrace);
        self.syms.popScope();
    }

    func checkFuncDecl(mut self) {
        self.ts.advance(); /* func */
        let name: Token = self.ts.expect(TokenKind::Ident);
        /* parse params into symbol table */
        self.ts.consume(TokenKind::LParen);
        self.syms.pushScope();
        while !self.ts.check(TokenKind::RParen) && !self.ts.atEnd() {
            let pname: Token = self.ts.expect(TokenKind::Ident);
            self.ts.consume(TokenKind::Colon);
            let ptype: Str = self.parseTypeStr();
            self.syms.define(pname.value, ptype, false);
            self.ts.consume(TokenKind::Comma);
        }
        self.ts.consume(TokenKind::RParen);
        /* skip -> ReturnType */
        if self.ts.check(TokenKind::Arrow) {
            self.ts.advance();
            self.parseTypeStr();
        }
        /* body — reuse the pushed scope (params visible in body) */
        self.ts.expect(TokenKind::LBrace);
        while !self.ts.check(TokenKind::RBrace) && !self.ts.atEnd() {
            self.checkStmt();
        }
        self.ts.expect(TokenKind::RBrace);
        self.syms.popScope();
        /* register the function itself */
        self.syms.define(name.value, "func", false);
    }

    func checkProgram(mut self) {
        while !self.ts.atEnd() { self.checkStmt(); }
    }
}

/* -----------------------------------------------------------------------
   Phase 6: IR emitter — emits LLVM IR text to stdout
   Handles: func decls, let, return, binary ops, calls, if, while
   ----------------------------------------------------------------------- */

class IRGen {
    let mut ts:      TokenStream = TokenStream  { tokens: [], pos: 0 };
    let mut ep:      Parser      = Parser       { ts: TokenStream { tokens: [], pos: 0 } };
    let mut syms:    SymbolTable = SymbolTable  { syms: [], scopeDepth: 0 };
    let mut tmpId:   I32 = 0;
    let mut labelId: I32 = 0;
    let mut out:     [Str] = [];

    func init(mut self, stream: TokenStream) {
        self.ts = stream;
        self.ep.ts = stream;
    }

    func nextTmp(mut self) -> Str {
        self.tmpId += 1;
        return "%" + ToString(self.tmpId);
    }

    func nextLabel(mut self) -> Str {
        self.labelId += 1;
        return "L" + ToString(self.labelId);
    }

    func emit(mut self, line: Str) {
        self.out.push(line);
    }

    func emitIndent(mut self, line: Str) {
        self.out.push("  " + line);
    }

    func syncTs(mut self) {
        self.ep.ts = self.ts;
    }

    func syncBack(mut self) {
        self.ts = self.ep.ts;
    }

    func llvmType(self, t: Str) -> Str {
        if t == "I32"  { return "i32"; }
        if t == "I64"  { return "i64"; }
        if t == "F32"  { return "float"; }
        if t == "F64"  { return "double"; }
        if t == "Bool" { return "i1"; }
        if t == "U8"   { return "i8"; }
        if t == "U16"  { return "i16"; }
        if t == "U32"  { return "i32"; }
        if t == "U64"  { return "i64"; }
        if t == "Str"  { return "i8*"; }
        return "i32";
    }

    func parseTypeStr(mut self) -> Str {
        let mut t: Str = "";
        while !self.ts.check(TokenKind::Assign) &&
              !self.ts.check(TokenKind::Semicolon) &&
              !self.ts.check(TokenKind::LBrace) &&
              !self.ts.check(TokenKind::KwIn) &&
              !self.ts.check(TokenKind::Comma) &&
              !self.ts.check(TokenKind::RParen) &&
              !self.ts.atEnd() {
            t = t + self.ts.advance().value;
        }
        return t;
    }

    /* emitExpr: emit instructions for an expression, return the result register */
    func emitExpr(mut self) -> Str {
        self.syncTs();
        /* parse just one primary+op for now — simple Pratt */
        let left: Str = self.emitPrimary();
        let t: Token = self.ep.ts.peek();

        if t.kind == TokenKind::Plus || t.kind == TokenKind::Minus ||
           t.kind == TokenKind::Star || t.kind == TokenKind::Slash ||
           t.kind == TokenKind::Eq   || t.kind == TokenKind::NotEq ||
           t.kind == TokenKind::Lt   || t.kind == TokenKind::Gt    ||
           t.kind == TokenKind::LtEq || t.kind == TokenKind::GtEq {
            let op: Token = self.ep.ts.advance();
            let right: Str = self.emitPrimary();
            let res: Str = self.nextTmp();
            if op.value == "+"  { self.emitIndent(res + " = add i32 " + left + ", " + right); }
            else if op.value == "-"  { self.emitIndent(res + " = sub i32 " + left + ", " + right); }
            else if op.value == "*"  { self.emitIndent(res + " = mul i32 " + left + ", " + right); }
            else if op.value == "/"  { self.emitIndent(res + " = sdiv i32 " + left + ", " + right); }
            else if op.value == "==" { self.emitIndent(res + " = icmp eq i32 " + left + ", " + right); }
            else if op.value == "!=" { self.emitIndent(res + " = icmp ne i32 " + left + ", " + right); }
            else if op.value == "<"  { self.emitIndent(res + " = icmp slt i32 " + left + ", " + right); }
            else if op.value == ">"  { self.emitIndent(res + " = icmp sgt i32 " + left + ", " + right); }
            else if op.value == "<=" { self.emitIndent(res + " = icmp sle i32 " + left + ", " + right); }
            else if op.value == ">=" { self.emitIndent(res + " = icmp sge i32 " + left + ", " + right); }
            self.syncBack();
            return res;
        }
        self.syncBack();
        return left;
    }

    func emitPrimary(mut self) -> Str {
        let t: Token = self.ep.ts.peek();
        if t.kind == TokenKind::Int {
            self.ep.ts.advance();
            return t.value;
        }
        if t.kind == TokenKind::Ident {
            self.ep.ts.advance();
            /* check for call */
            if self.ep.ts.check(TokenKind::LParen) {
                self.ep.ts.advance();
                let mut args: [Str] = [];
                while !self.ep.ts.check(TokenKind::RParen) && !self.ep.ts.atEnd() {
                    args.push(self.emitPrimary());
                    self.ep.ts.consume(TokenKind::Comma);
                }
                self.ep.ts.consume(TokenKind::RParen);
                let res: Str = self.nextTmp();
                let mut argStr: Str = "";
                let mut first: Bool = true;
                for a in args {
                    if !first { argStr = argStr + ", "; }
                    first = false;
                    argStr = argStr + "i32 " + a;
                }
                self.emitIndent(res + " = call i32 @" + t.value + "(" + argStr + ")");
                return res;
            }
            /* variable load */
            let res: Str = self.nextTmp();
            self.emitIndent(res + " = load i32, i32* %" + t.value + "_ptr");
            return res;
        }
        self.ep.ts.advance();
        return "0";
    }

    func emitBlock(mut self) {
        self.ts.expect(TokenKind::LBrace);
        self.syms.pushScope();
        while !self.ts.check(TokenKind::RBrace) && !self.ts.atEnd() {
            self.emitStmt();
        }
        self.ts.expect(TokenKind::RBrace);
        self.syms.popScope();
    }

    func emitStmt(mut self) {
        let t: Token = self.ts.peek();
        if t.kind == TokenKind::KwLet    { self.emitLet(); return; }
        if t.kind == TokenKind::KwReturn { self.emitReturn(); return; }
        if t.kind == TokenKind::KwIf     { self.emitIf(); return; }
        if t.kind == TokenKind::KwWhile  { self.emitWhile(); return; }
        /* skip unknown stmts */
        while !self.ts.check(TokenKind::Semicolon) && !self.ts.atEnd() {
            self.ts.advance();
        }
        self.ts.consume(TokenKind::Semicolon);
    }

    func emitLet(mut self) {
        self.ts.advance();
        let mut isMut: Bool = false;
        if self.ts.check(TokenKind::KwMut) { self.ts.advance(); isMut = true; }
        let name: Token = self.ts.expect(TokenKind::Ident);
        let mut typeName: Str = "I32";
        if self.ts.consume(TokenKind::Colon) { typeName = self.parseTypeStr(); }
        let llty: Str = self.llvmType(typeName);
        self.emitIndent("%" + name.value + "_ptr = alloca " + llty);
        if self.ts.consume(TokenKind::Assign) {
            let val: Str = self.emitExpr();
            self.emitIndent("store " + llty + " " + val + ", " + llty + "* %" + name.value + "_ptr");
        }
        self.ts.consume(TokenKind::Semicolon);
        self.syms.define(name.value, typeName, isMut);
    }

    func emitReturn(mut self) {
        self.ts.advance();
        if self.ts.check(TokenKind::Semicolon) {
            self.ts.advance();
            self.emitIndent("ret void");
            return;
        }
        let val: Str = self.emitExpr();
        self.ts.consume(TokenKind::Semicolon);
        self.emitIndent("ret i32 " + val);
    }

    func emitIf(mut self) {
        self.ts.advance();
        let cond: Str = self.emitExpr();
        let thenL: Str = self.nextLabel();
        let elseL: Str = self.nextLabel();
        let endL:  Str = self.nextLabel();
        self.emitIndent("br i1 " + cond + ", label %" + thenL + ", label %" + elseL);
        self.emit(thenL + ":");
        self.emitBlock();
        self.emitIndent("br label %" + endL);
        self.emit(elseL + ":");
        if self.ts.check(TokenKind::KwElse) {
            self.ts.advance();
            self.emitBlock();
        }
        self.emitIndent("br label %" + endL);
        self.emit(endL + ":");
    }

    func emitWhile(mut self) {
        self.ts.advance();
        let condL: Str = self.nextLabel();
        let bodyL: Str = self.nextLabel();
        let endL:  Str = self.nextLabel();
        self.emitIndent("br label %" + condL);
        self.emit(condL + ":");
        let cond: Str = self.emitExpr();
        self.emitIndent("br i1 " + cond + ", label %" + bodyL + ", label %" + endL);
        self.emit(bodyL + ":");
        self.emitBlock();
        self.emitIndent("br label %" + condL);
        self.emit(endL + ":");
    }

    func emitFunc(mut self) {
        self.ts.advance();
        let name: Token = self.ts.expect(TokenKind::Ident);
        self.ts.consume(TokenKind::LParen);
        self.syms.pushScope();
        let mut params: [Str] = [];
        let mut paramTypes: [Str] = [];
        while !self.ts.check(TokenKind::RParen) && !self.ts.atEnd() {
            let mut isMutP: Bool = false;
            if self.ts.check(TokenKind::KwMut) { self.ts.advance(); isMutP = true; }
            let pname: Token = self.ts.expect(TokenKind::Ident);
            self.ts.consume(TokenKind::Colon);
            let ptype: Str = self.parseTypeStr();
            params.push(pname.value);
            paramTypes.push(ptype);
            self.syms.define(pname.value, ptype, isMutP);
            self.ts.consume(TokenKind::Comma);
        }
        self.ts.consume(TokenKind::RParen);
        let mut retType: Str = "void";
        if self.ts.check(TokenKind::Arrow) {
            self.ts.advance();
            retType = self.parseTypeStr();
        }
        let llRet: Str = self.llvmType(retType);
        let mut sig: Str = "";
        let mut first: Bool = true;
        let mut pi: I32 = 0;
        while pi < params.len() {
            if !first { sig = sig + ", "; }
            first = false;
            sig = sig + self.llvmType(paramTypes[pi]) + " %" + params[pi];
            pi += 1;
        }
        self.emit("define " + llRet + " @" + name.value + "(" + sig + ") {");
        self.emit("entry:");
        /* alloca params so they can be stored */
        pi = 0;
        while pi < params.len() {
            let llty: Str = self.llvmType(paramTypes[pi]);
            self.emitIndent("%" + params[pi] + "_ptr = alloca " + llty);
            self.emitIndent("store " + llty + " %" + params[pi] + ", " + llty + "* %" + params[pi] + "_ptr");
            pi += 1;
        }
        /* body */
        self.ts.expect(TokenKind::LBrace);
        while !self.ts.check(TokenKind::RBrace) && !self.ts.atEnd() {
            self.emitStmt();
        }
        self.ts.expect(TokenKind::RBrace);
        self.syms.popScope();
        self.emit("}");
        self.emit("");
        self.syms.define(name.value, retType, false);
    }

    func emitModule(mut self) {
        self.emit("; KonScript self-hosted IR — generated by konscript.ks");
        self.emit("target triple = \"x86_64-unknown-linux-gnu\"");
        self.emit("");
        while !self.ts.atEnd() {
            let t: Token = self.ts.peek();
            if t.kind == TokenKind::KwFunc { self.emitFunc(); }
            else { self.ts.advance(); }
        }
    }
}

func main() {
    let source: Str = "func add(a: I32, b: I32) -> I32 { let result: I32 = a + b; return result; } func main() -> I32 { let x: I32 = add(2, 3); return x; }";
    let mut lexer: Lexer = Lexer { src: "", pos: 0, line: 1, col: 1, tokens: [] };
    lexer.init(source);
    let toks: [Token] = lexer.tokenize();
    let mut ts: TokenStream = TokenStream { tokens: [], pos: 0 };
    ts.init(toks);
    let mut irgen: IRGen = IRGen {
        ts: ts,
        ep: Parser { ts: TokenStream { tokens: [], pos: 0 } },
        syms: SymbolTable { syms: [], scopeDepth: 0 },
        tmpId: 0,
        labelId: 0,
        out: [],
    };
    irgen.init(ts);
    irgen.emitModule();
    for line in irgen.out {
        Print(line);
    }
}
