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
        self.src  = source;
        self.pos  = 0;
        self.line = 1;
        self.col  = 1;
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
            }
        }
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

func main() {
    let source: Str = "1 + 2 * 3";
    let mut lexer: Lexer = Lexer { src: "", pos: 0, line: 1, col: 1, tokens: [] };
    lexer.init(source);
    let toks: [Token] = lexer.tokenize();
    let mut ts: TokenStream = TokenStream { tokens: [], pos: 0 };
    ts.init(toks);
    let mut parser: Parser = Parser { ts: ts };
    let result: Str = parser.parseExpr();
    Print(result);
}
