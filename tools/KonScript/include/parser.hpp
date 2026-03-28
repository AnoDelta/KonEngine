#pragma once
// -----------------------------------------------------------------------
// KonScript Parser
// Recursive descent parser. Turns a token stream into an AST.
// -----------------------------------------------------------------------
#include "lexer.hpp"
#include "ast.hpp"
#include <stdexcept>
#include <sstream>

namespace KonScript {

class Parser {
public:
    Parser(std::vector<Token> tokens, const std::string& filename = "<input>")
        : m_tokens(std::move(tokens)), m_filename(filename) {}

    Program parse() {
        Program prog;
        prog.filename = m_filename;
        while (!atEnd()) {
            try {
                prog.stmts.push_back(parseTopLevel());
            } catch (std::exception& e) {
                m_errors.push_back(e.what());
                synchronize();
            }
        }
        return prog;
    }

    const std::vector<std::string>& errors() const { return m_errors; }
    bool hasErrors() const { return !m_errors.empty(); }

private:
    std::vector<Token>      m_tokens;
    std::string             m_filename;
    size_t                  m_pos = 0;
    std::vector<std::string> m_errors;

    // -----------------------------------------------------------------------
    // Token helpers
    // -----------------------------------------------------------------------
    Token& peek(int offset = 0) {
        size_t i = m_pos + offset;
        if (i >= m_tokens.size()) return m_tokens.back(); // Eof
        return m_tokens[i];
    }

    bool atEnd() { return peek().type == TokenType::Eof; }

    Token advance() {
        Token t = peek();
        if (!atEnd()) m_pos++;
        return t;
    }

    bool check(TokenType t) { return peek().type == t; }

    bool match(TokenType t) {
        if (check(t)) { advance(); return true; }
        return false;
    }

    bool matchAny(std::initializer_list<TokenType> types) {
        for (auto t : types) if (check(t)) { advance(); return true; }
        return false;
    }

    Token expect(TokenType t, const std::string& msg) {
        if (!check(t)) error(msg);
        return advance();
    }

    void error(const std::string& msg) {
        auto& tok = peek();
        throw std::runtime_error(
            m_filename + ":" + std::to_string(tok.line) +
            ":" + std::to_string(tok.col) + ": " + msg);
    }

    void synchronize() {
        // Always advance at least one token to prevent infinite loops
        if (!atEnd()) advance();
 
        while (!atEnd()) {
            auto t = peek().type;
            if (t == TokenType::Semicolon) { advance(); return; }
            if (t == TokenType::RBrace)    return;
            if (t == TokenType::Func  ||
                t == TokenType::Node   ||
                t == TokenType::Let   ||
                t == TokenType::Const ||
                t == TokenType::If    ||
                t == TokenType::While ||
                t == TokenType::For   ||
                t == TokenType::Return) return;
            advance();
        }
    }

    // -----------------------------------------------------------------------
    // Type annotations
    // -----------------------------------------------------------------------
    TypeAnnotation parseType() {
        TypeAnnotation ta;

        // Tuple: (F64, F64)
        if (check(TokenType::LParen)) {
            advance();
            ta.isTuple = true;
            while (!check(TokenType::RParen) && !atEnd()) {
                ta.tupleTypes.push_back(parseType());
                if (!match(TokenType::Comma)) break;
            }
            expect(TokenType::RParen, "expected ')' after tuple type");
            if (check(TokenType::Question)) { advance(); ta.nullable = true; }
            return ta;
        }

        // Array: [T] or [T; N]
        if (check(TokenType::LBracket)) {
            advance();
            ta.isArray = true;
            auto inner = parseType();
            ta.base = inner.base;
            if (match(TokenType::Semicolon)) {
                auto sz = expect(TokenType::Int, "expected array size");
                ta.arraySize = std::stoi(sz.value);
            }
            expect(TokenType::RBracket, "expected ']'");
            if (check(TokenType::Question)) { advance(); ta.nullable = true; }
            return ta;
        }

        // Base type
        auto& tok = peek();
        switch (tok.type) {
            case TokenType::TI8:  ta.base = "I8";  break;
            case TokenType::TI16: ta.base = "I16"; break;
            case TokenType::TI32: ta.base = "I32"; break;
            case TokenType::TI64: ta.base = "I64"; break;
            case TokenType::TU8:  ta.base = "U8";  break;
            case TokenType::TU16: ta.base = "U16"; break;
            case TokenType::TU32: ta.base = "U32"; break;
            case TokenType::TU64: ta.base = "U64"; break;
            case TokenType::TF32: ta.base = "F32"; break;
            case TokenType::TF64: ta.base = "F64"; break;
            case TokenType::TBool:   ta.base = "Bool";   break;
            case TokenType::TStr:    ta.base = "str";    break;
            case TokenType::TString: ta.base = "String"; break;
            case TokenType::TVec2:   ta.base = "Vec2";   break;
            case TokenType::Ident:   ta.base = tok.value; break;
            default:
                error("expected type, got '" + tok.value + "'");
        }
        advance();

        if (check(TokenType::Question)) { advance(); ta.nullable = true; }
        return ta;
    }

    // -----------------------------------------------------------------------
    // Top-level statements
    // -----------------------------------------------------------------------
    StmtPtr parseTopLevel() {
        int l = peek().line, c = peek().col;

        if (check(TokenType::Include)) return parseInclude();
        if (check(TokenType::Pub))     return parsePubDecl();
        if (check(TokenType::Node))    return parseNodeDecl(false);
        if (check(TokenType::Struct))  return parseStructDecl(false);
        if (check(TokenType::Enum))    return parseEnumDecl();
        if (check(TokenType::Class))   return parseClassDecl(false);
        if (check(TokenType::Func))    return parseFuncDecl(false);
        if (check(TokenType::Const))   return parseConst();
        if (check(TokenType::Let))     return parseLet();

        error("unexpected token '" + peek().value + "' at top level");
        return nullptr;
    }

    StmtPtr parsePubDecl() {
        advance(); // pub
        if (check(TokenType::Func))   return parseFuncDecl(true);
        if (check(TokenType::Node))   return parseNodeDecl(true);
        if (check(TokenType::Struct)) return parseStructDecl(true);
        if (check(TokenType::Class))  return parseClassDecl(true);
        error("expected declaration after 'pub'");
        return nullptr;
    }

    StmtPtr parseInclude() {
        int l = peek().line, c = peek().col;
        advance(); // #include
        auto path = expect(TokenType::Str, "expected path after #include");
        bool isSys = !path.value.empty() && path.value.front() == '<';
        std::string p = path.value;
        if (!p.empty() && (p.front() == '<' || p.front() == '"'))
            p = p.substr(1, p.size() - 2);
        return std::make_unique<IncludeStmt>(p, isSys, l, c);
    }

    // -----------------------------------------------------------------------
    // Declarations
    // -----------------------------------------------------------------------
    StmtPtr parseNodeDecl(bool pub) {
        int l = peek().line, c = peek().col;
        advance(); // node
        std::string name = expect(TokenType::Ident, "expected node name").value;
        std::string base = "Node";
        if (match(TokenType::Colon))
            base = expect(TokenType::Ident, "expected base type").value;

        expect(TokenType::LBrace, "expected '{' after node declaration");

        std::vector<FieldDecl> fields;
        std::vector<std::unique_ptr<FuncDecl>> methods;

        while (!check(TokenType::RBrace) && !atEnd()) {
            bool isPub = match(TokenType::Pub);
            if (check(TokenType::Func)) {
                methods.push_back(parseFuncDeclInner(isPub));
            } else if (check(TokenType::Let)) {
                fields.push_back(parseFieldDecl(isPub));
            } else {
                error("expected field or method in node body");
            }
        }
        expect(TokenType::RBrace, "expected '}' after node body");

        return std::make_unique<NodeDecl>(name, base,
            std::move(fields), std::move(methods), l, c);
    }

    StmtPtr parseStructDecl(bool pub) {
        int l = peek().line, c = peek().col;
        advance(); // struct
        std::string name = expect(TokenType::Ident, "expected struct name").value;
        expect(TokenType::LBrace, "expected '{'");

        std::vector<FieldDecl> fields;
        while (!check(TokenType::RBrace) && !atEnd()) {
            bool isPub = match(TokenType::Pub);
            fields.push_back(parseFieldDecl(isPub));
        }
        expect(TokenType::RBrace, "expected '}'");
        return std::make_unique<StructDecl>(name, std::move(fields), l, c);
    }

    StmtPtr parseEnumDecl() {
        int l = peek().line, c = peek().col;
        advance(); // enum
        std::string name = expect(TokenType::Ident, "expected enum name").value;
        expect(TokenType::LBrace, "expected '{'");

        std::vector<EnumVariant> variants;
        while (!check(TokenType::RBrace) && !atEnd()) {
            EnumVariant v;
            v.name = expect(TokenType::Ident, "expected variant name").value;
            if (check(TokenType::LParen)) {
                advance();
                v.payload = parseType();
                expect(TokenType::RParen, "expected ')'");
            }
            variants.push_back(std::move(v));
            match(TokenType::Comma);
        }
        expect(TokenType::RBrace, "expected '}'");
        return std::make_unique<EnumDecl>(name, std::move(variants), l, c);
    }

    StmtPtr parseClassDecl(bool pub) {
        int l = peek().line, c = peek().col;
        advance(); // class
        std::string name = expect(TokenType::Ident, "expected class name").value;
        std::string base;
        if (match(TokenType::Colon))
            base = expect(TokenType::Ident, "expected base class").value;

        expect(TokenType::LBrace, "expected '{'");

        std::vector<FieldDecl> fields;
        std::vector<std::unique_ptr<FuncDecl>> methods;

        while (!check(TokenType::RBrace) && !atEnd()) {
            bool isPub = match(TokenType::Pub);
            if (check(TokenType::Func))
                methods.push_back(parseFuncDeclInner(isPub));
            else if (check(TokenType::Let))
                fields.push_back(parseFieldDecl(isPub));
            else
                error("expected field or method in class body");
        }
        expect(TokenType::RBrace, "expected '}'");
        return std::make_unique<ClassDecl>(name, base,
            std::move(fields), std::move(methods), l, c);
    }

    FieldDecl parseFieldDecl(bool pub) {
        FieldDecl fd;
        fd.pub = pub;
        advance(); // let
        fd.mut = match(TokenType::Mut);
        fd.name = expect(TokenType::Ident, "expected field name").value;
        expect(TokenType::Colon, "expected ':' after field name");
        fd.type = parseType();
        if (match(TokenType::Assign))
            fd.init = parseExpr();
        expect(TokenType::Semicolon, "expected ';' after field");
        return fd;
    }

    StmtPtr parseFuncDecl(bool pub) {
        return std::make_unique<FuncDecl>(
            std::move(*parseFuncDeclInner(pub)));
    }

    std::unique_ptr<FuncDecl> parseFuncDeclInner(bool pub) {
        int l = peek().line, c = peek().col;
        advance(); // func
        std::string name = expect(TokenType::Ident, "expected function name").value;

        expect(TokenType::LParen, "expected '('");
        std::vector<Param> params;
        while (!check(TokenType::RParen) && !atEnd()) {
            Param p;
            p.name = expect(TokenType::Ident, "expected parameter name").value;
            expect(TokenType::Colon, "expected ':' after parameter name");
            p.type = parseType();
            params.push_back(std::move(p));
            if (!match(TokenType::Comma)) break;
        }
        expect(TokenType::RParen, "expected ')'");

        std::optional<TypeAnnotation> ret;
        if (match(TokenType::Arrow))
            ret = parseType();

        auto body = parseBlock();

        return std::make_unique<FuncDecl>(
            name, std::move(params), ret, std::move(body), pub, l, c);
    }

    // -----------------------------------------------------------------------
    // Statements
    // -----------------------------------------------------------------------
    StmtPtr parseStmt() {
        int l = peek().line, c = peek().col;

        if (check(TokenType::Let))      return parseLet();
        if (check(TokenType::Const))    return parseConst();
        if (check(TokenType::Return))   return parseReturn();
        if (check(TokenType::Break))    return parseBreak();
        if (check(TokenType::Continue)) return parseContinue();
        if (check(TokenType::If))       return parseIf();
        if (check(TokenType::While))    return parseWhile();
        if (check(TokenType::Loop))     return parseLoop();
        if (check(TokenType::For))      return parseFor();
        if (check(TokenType::Switch))   return parseSwitch();
        if (check(TokenType::Wait))     return parseWait();
        if (check(TokenType::Func))     return parseFuncDecl(false);
        if (check(TokenType::LBrace)) {
            auto b = parseBlock();
            return std::make_unique<BlockStmt>(std::move(b->stmts), l, c);
        }

        // Expression statement
        auto expr = parseExpr();
        expect(TokenType::Semicolon, "expected ';' after expression");
        return std::make_unique<ExprStmt>(std::move(expr), l, c);
    }

    std::unique_ptr<BlockStmt> parseBlock() {
        int l = peek().line, c = peek().col;
        expect(TokenType::LBrace, "expected '{'");
        std::vector<StmtPtr> stmts;
        while (!check(TokenType::RBrace) && !atEnd()) {
            try {
                stmts.push_back(parseStmt());
            } catch (std::exception& e) {
                m_errors.push_back(e.what());
                synchronize();
            }
        }
        expect(TokenType::RBrace, "expected '}'");
        return std::make_unique<BlockStmt>(std::move(stmts), l, c);
    }

    StmtPtr parseLet() {
        int l = peek().line, c = peek().col;
        advance(); // let
        bool mut = match(TokenType::Mut);
        std::string name = expect(TokenType::Ident, "expected variable name").value;
        expect(TokenType::Colon, "expected ':' after variable name");
        auto type = parseType();
        expect(TokenType::Assign, "expected '=' after type");
        auto init = parseExpr();
        expect(TokenType::Semicolon, "expected ';'");
        return std::make_unique<LetStmt>(name, type, std::move(init), mut, l, c);
    }

    StmtPtr parseConst() {
        int l = peek().line, c = peek().col;
        advance(); // const
        std::string name = expect(TokenType::Ident, "expected constant name").value;
        expect(TokenType::Colon, "expected ':'");
        auto type = parseType();
        expect(TokenType::Assign, "expected '='");
        auto init = parseExpr();
        expect(TokenType::Semicolon, "expected ';'");
        return std::make_unique<ConstStmt>(name, type, std::move(init), l, c);
    }

    StmtPtr parseReturn() {
        int l = peek().line, c = peek().col;
        advance(); // return
        ExprPtr val;
        if (!check(TokenType::Semicolon))
            val = parseExpr();
        expect(TokenType::Semicolon, "expected ';' after return");
        return std::make_unique<ReturnStmt>(std::move(val), l, c);
    }

    StmtPtr parseBreak() {
        int l = peek().line, c = peek().col;
        advance(); // break
        std::string label;
        if (check(TokenType::Apostrophe))
            label = advance().value;
        expect(TokenType::Semicolon, "expected ';'");
        return std::make_unique<BreakStmt>(label, l, c);
    }

    StmtPtr parseContinue() {
        int l = peek().line, c = peek().col;
        advance(); // continue
        std::string label;
        if (check(TokenType::Apostrophe))
            label = advance().value;
        expect(TokenType::Semicolon, "expected ';'");
        return std::make_unique<ContinueStmt>(label, l, c);
    }

    StmtPtr parseIf() {
        int l = peek().line, c = peek().col;
        advance(); // if
        auto cond = parseExpr();
        auto then_ = parseBlock();
        std::unique_ptr<BlockStmt> else_;
        if (match(TokenType::Else)) {
            if (check(TokenType::If)) {
                // else if -- wrap in a block
                int el = peek().line, ec = peek().col;
                std::vector<StmtPtr> s;
                s.push_back(parseIf());
                else_ = std::make_unique<BlockStmt>(std::move(s), el, ec);
            } else {
                else_ = parseBlock();
            }
        }
        return std::make_unique<IfStmt>(
            std::move(cond), std::move(then_), std::move(else_), l, c);
    }

    StmtPtr parseWhile() {
        int l = peek().line, c = peek().col;
        advance(); // while
        auto cond = parseExpr();
        auto body = parseBlock();
        return std::make_unique<WhileStmt>(std::move(cond), std::move(body), l, c);
    }

    StmtPtr parseLoop() {
        int l = peek().line, c = peek().col;
        advance(); // loop
        auto body = parseBlock();
        return std::make_unique<LoopStmt>(std::move(body), l, c);
    }

    StmtPtr parseFor() {
        int l = peek().line, c = peek().col;
        advance(); // for

        // Label check: 'label: for
        std::string label;
        if (check(TokenType::Apostrophe)) {
            label = advance().value;
            expect(TokenType::Colon, "expected ':' after label");
        }

        std::string var = expect(TokenType::Ident, "expected variable name").value;
        expect(TokenType::Colon, "expected ':' after variable");
        auto type = parseType();

        // for i: I32 in ...   vs   for i: I32 = ...
        if (match(TokenType::In)) {
            auto iterable = parseExpr();
            auto body = parseBlock();
            return std::make_unique<ForInStmt>(
                var, type, std::move(iterable), std::move(body), label, l, c);
        } else {
            // C-style: for i: I32 = init; cond; step
            expect(TokenType::Assign, "expected '=' or 'in'");
            auto init = parseExpr();
            expect(TokenType::Semicolon, "expected ';'");
            auto cond = parseExpr();
            expect(TokenType::Semicolon, "expected ';'");
            auto step = parseExpr();
            auto body = parseBlock();
            return std::make_unique<ForCStmt>(
                var, type, std::move(init), std::move(cond),
                std::move(step), std::move(body), l, c);
        }
    }

    StmtPtr parseSwitch() {
        int l = peek().line, c = peek().col;
        advance(); // switch
        auto expr = parseExpr();
        expect(TokenType::LBrace, "expected '{'");

        std::vector<SwitchCase> cases;
        while (!check(TokenType::RBrace) && !atEnd()) {
            SwitchCase sc;
            if (match(TokenType::Default)) {
                sc.isDefault = true;
                expect(TokenType::Colon, "expected ':' after default");
            } else {
                expect(TokenType::Case, "expected 'case' or 'default'");
                sc.values.push_back(parseExpr());
                expect(TokenType::Colon, "expected ':' after case value");
            }
            // Parse body until next case/default/}
            while (!check(TokenType::Case) &&
                   !check(TokenType::Default) &&
                   !check(TokenType::RBrace) && !atEnd()) {
                sc.body.push_back(parseStmt());
            }
            cases.push_back(std::move(sc));
        }
        expect(TokenType::RBrace, "expected '}'");
        return std::make_unique<SwitchStmt>(std::move(expr), std::move(cases), l, c);
    }

    StmtPtr parseWait() {
        int l = peek().line, c = peek().col;
        advance(); // wait
        auto dur = parseExpr();
        expect(TokenType::Semicolon, "expected ';'");
        return std::make_unique<WaitStmt>(std::move(dur), l, c);
    }

    // -----------------------------------------------------------------------
    // Expressions — precedence climbing
    // -----------------------------------------------------------------------
    ExprPtr parseExpr() { return parseAssign(); }

    ExprPtr parseAssign() {
        int l = peek().line, c = peek().col;
        auto left = parseNullCoal();

        static const std::vector<TokenType> assignOps = {
            TokenType::Assign, TokenType::PlusEq,
            TokenType::MinusEq, TokenType::StarEq, TokenType::SlashEq
        };
        for (auto op : assignOps) {
            if (check(op)) {
                std::string opStr = advance().value;
                auto right = parseAssign();
                return std::make_unique<AssignExpr>(
                    opStr, std::move(left), std::move(right), l, c);
            }
        }
        return left;
    }

    ExprPtr parseNullCoal() {
        int l = peek().line, c = peek().col;
        auto left = parseOr();
        while (check(TokenType::NullCoal)) {
            advance();
            auto right = parseOr();
            left = std::make_unique<NullCoalExpr>(
                std::move(left), std::move(right), l, c);
        }
        return left;
    }

    ExprPtr parseOr() {
        int l = peek().line, c = peek().col;
        auto left = parseAnd();
        while (check(TokenType::Or)) {
            advance();
            auto right = parseAnd();
            left = std::make_unique<BinaryExpr>(
                "||", std::move(left), std::move(right), l, c);
        }
        return left;
    }

    ExprPtr parseAnd() {
        int l = peek().line, c = peek().col;
        auto left = parseEquality();
        while (check(TokenType::And)) {
            advance();
            auto right = parseEquality();
            left = std::make_unique<BinaryExpr>(
                "&&", std::move(left), std::move(right), l, c);
        }
        return left;
    }

    ExprPtr parseEquality() {
        int l = peek().line, c = peek().col;
        auto left = parseComparison();
        while (check(TokenType::Eq) || check(TokenType::NotEq)) {
            std::string op = advance().value;
            auto right = parseComparison();
            left = std::make_unique<BinaryExpr>(
                op, std::move(left), std::move(right), l, c);
        }
        return left;
    }

    ExprPtr parseComparison() {
        int l = peek().line, c = peek().col;
        auto left = parseAddSub();
        while (check(TokenType::Lt) || check(TokenType::Gt) ||
               check(TokenType::LtEq) || check(TokenType::GtEq)) {
            std::string op = advance().value;
            auto right = parseAddSub();
            left = std::make_unique<BinaryExpr>(
                op, std::move(left), std::move(right), l, c);
        }
        return left;
    }

    ExprPtr parseAddSub() {
        int l = peek().line, c = peek().col;
        auto left = parseMulDiv();
        while (check(TokenType::Plus) || check(TokenType::Minus)) {
            std::string op = advance().value;
            auto right = parseMulDiv();
            left = std::make_unique<BinaryExpr>(
                op, std::move(left), std::move(right), l, c);
        }
        return left;
    }

    ExprPtr parseMulDiv() {
        int l = peek().line, c = peek().col;
        auto left = parseUnary();
        while (check(TokenType::Star) || check(TokenType::Slash) ||
               check(TokenType::Percent)) {
            std::string op = advance().value;
            auto right = parseUnary();
            left = std::make_unique<BinaryExpr>(
                op, std::move(left), std::move(right), l, c);
        }
        return left;
    }

    ExprPtr parseUnary() {
        int l = peek().line, c = peek().col;
        if (check(TokenType::Minus)) {
            advance();
            return std::make_unique<UnaryExpr>(
                "-", parseUnary(), false, l, c);
        }
        if (check(TokenType::Bang)) {
            advance();
            return std::make_unique<UnaryExpr>(
                "!", parseUnary(), false, l, c);
        }
        return parseCast();
    }

    ExprPtr parseCast() {
        int l = peek().line, c = peek().col;
        auto expr = parsePostfix();
        if (match(TokenType::As)) {
            auto type = parseType();
            return std::make_unique<CastExpr>(std::move(expr), type, l, c);
        }
        return expr;
    }

    ExprPtr parsePostfix() {
        int l = peek().line, c = peek().col;
        auto expr = parsePrimary();

        while (true) {
            if (check(TokenType::Dot) || check(TokenType::SafeDot) ||
                check(TokenType::ColonColon)) {
                bool safe = check(TokenType::SafeDot);
                advance();
                std::string member = expect(TokenType::Ident,
                    "expected member name").value;
                expr = std::make_unique<MemberExpr>(
                    std::move(expr), member, safe, l, c);

            } else if (check(TokenType::LParen)) {
                // Call
                advance();
                std::vector<ExprPtr> args;
                while (!check(TokenType::RParen) && !atEnd()) {
                    args.push_back(parseExpr());
                    if (!match(TokenType::Comma)) break;
                }
                expect(TokenType::RParen, "expected ')'");
                expr = std::make_unique<CallExpr>(
                    std::move(expr), std::move(args), l, c);

            } else if (check(TokenType::LBracket)) {
                // Index
                advance();
                auto idx = parseExpr();
                expect(TokenType::RBracket, "expected ']'");
                expr = std::make_unique<IndexExpr>(
                    std::move(expr), std::move(idx), l, c);

            } else if (check(TokenType::PlusPlus)) {
                advance();
                expr = std::make_unique<UnaryExpr>(
                    "++", std::move(expr), true, l, c);

            } else if (check(TokenType::MinusMinus)) {
                advance();
                expr = std::make_unique<UnaryExpr>(
                    "--", std::move(expr), true, l, c);

            } else if (check(TokenType::Bang)) {
                // Force unwrap x!
                advance();
                expr = std::make_unique<ForceUnwrapExpr>(
                    std::move(expr), l, c);

            } else if (check(TokenType::DotDot) || check(TokenType::DotDotEq)) {
                // Range  x..y  x..=y
                bool inc = check(TokenType::DotDotEq);
                advance();
                auto to = parsePrimary();
                expr = std::make_unique<RangeExpr>(
                    std::move(expr), std::move(to), inc, l, c);

            } else {
                break;
            }
        }
        return expr;
    }

    ExprPtr parsePrimary() {
        int l = peek().line, c = peek().col;

        // Integer literal
        if (check(TokenType::Int)) {
            std::string v = advance().value;
            int64_t val = v.find("0x") == 0
                ? std::stoll(v, nullptr, 16)
                : std::stoll(v);
            return std::make_unique<IntLitExpr>(val, l, c);
        }

        // Float literal -- BUG-07: keep raw text so codegen emits it verbatim
        if (check(TokenType::Float)) {
            std::string raw = advance().value;
            double val = std::stod(raw);
            return std::make_unique<FloatLitExpr>(val, raw, l, c);
        }

        // Bool literal
        if (check(TokenType::Bool)) {
            bool val = advance().value == "true";
            return std::make_unique<BoolLitExpr>(val, l, c);
        }

        // String literal
        if (check(TokenType::Str)) {
            return std::make_unique<StrLitExpr>(advance().value, l, c);
        }

        // null
        if (check(TokenType::Null)) {
            advance();
            return std::make_unique<NullLitExpr>(l, c);
        }

        // None
        if (check(TokenType::None_)) {
            advance();
            return std::make_unique<NoneLitExpr>(l, c);
        }

        // Some(x)
        if (check(TokenType::Some)) {
            advance();
            expect(TokenType::LParen, "expected '(' after Some");
            auto val = parseExpr();
            expect(TokenType::RParen, "expected ')'");
            return std::make_unique<SomeExpr>(std::move(val), l, c);
        }

        // spawn
        if (check(TokenType::Spawn)) {
            advance();
            auto call = parsePostfix();
            return std::make_unique<SpawnExpr>(std::move(call), l, c);
        }

        // Array literal [1, 2, 3]
        if (check(TokenType::LBracket)) {
            advance();
            std::vector<ExprPtr> elems;
            while (!check(TokenType::RBracket) && !atEnd()) {
                elems.push_back(parseExpr());
                if (!match(TokenType::Comma)) break;
            }
            expect(TokenType::RBracket, "expected ']'");
            return std::make_unique<ArrayLitExpr>(std::move(elems), l, c);
        }

        // Grouped expr or tuple (a, b)
        if (check(TokenType::LParen)) {
            advance();
            auto first = parseExpr();
            if (match(TokenType::Comma)) {
                std::vector<ExprPtr> elems;
                elems.push_back(std::move(first));
                while (!check(TokenType::RParen) && !atEnd()) {
                    elems.push_back(parseExpr());
                    if (!match(TokenType::Comma)) break;
                }
                expect(TokenType::RParen, "expected ')'");
                return std::make_unique<TupleLitExpr>(std::move(elems), l, c);
            }
            expect(TokenType::RParen, "expected ')'");
            return first;
        }

        // Identifier -- could be a plain ident, struct init, or enum variant
        if (check(TokenType::Ident)) {
            std::string name = advance().value;

            // Struct init: Name { field: val, ... }
            if (check(TokenType::LBrace)) {
                // Peek ahead -- if next is ident followed by colon, it's struct init
                if (peek(1).type == TokenType::Ident &&
                    peek(2).type == TokenType::Colon) {
                    advance(); // {
                    std::vector<StructInitField> fields;
                    while (!check(TokenType::RBrace) && !atEnd()) {
                        StructInitField f;
                        f.name = expect(TokenType::Ident, "expected field name").value;
                        expect(TokenType::Colon, "expected ':'");
                        f.value = parseExpr();
                        fields.push_back(std::move(f));
                        if (!match(TokenType::Comma)) break;
                    }
                    expect(TokenType::RBrace, "expected '}'");
                    return std::make_unique<StructInitExpr>(
                        name, std::move(fields), l, c);
                }
            }

            return std::make_unique<IdentExpr>(name, l, c);
        }

        error("unexpected token '" + peek().value + "'");
        return nullptr;
    }
};

} // namespace KonScript
