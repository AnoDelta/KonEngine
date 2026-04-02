#pragma once
#include <functional>
// -----------------------------------------------------------------------
// KonScript Parser
// Recursive descent parser. Turns a token stream into an AST.
// -----------------------------------------------------------------------
#include "lexer.hpp"
#include "ast.hpp"
#include <stdexcept>
#include <sstream>
#include <unordered_set>

namespace KonScript {

class Parser {
public:
    Parser(std::vector<Token> tokens, const std::string& filename = "<input>")
        : m_tokens(std::move(tokens)), m_filename(filename) {}

    Program parse() {
        Program prog;
        prog.filename = m_filename;
        // #![intel_asm] — file-level attribute
        if (check(TokenType::Hash) && peek(1).type == TokenType::Bang) {
            advance(); advance(); // #!
            expect(TokenType::LBracket, "expected '['");
            std::string attr;
            while (!check(TokenType::RBracket) && !atEnd()) attr += advance().value;
            expect(TokenType::RBracket, "expected ']'");
            if (attr == "intel_asm") prog.intelAsm = true;
        }
        size_t prevPos = (size_t)-1;
        while (!atEnd()) {
            if (m_pos == prevPos) {
                m_errors.push_back("parser stuck at: " + peek().value);
                advance();
                continue;
            }
            prevPos = m_pos;
            try {
                auto stmt = parseTopLevel();
                if (stmt) prog.stmts.push_back(std::move(stmt));
            } catch (std::exception& e) {
                m_errors.push_back(e.what());
                synchronize();
            }
        }
        return prog;
    }

    // Allow calling parseExpr() standalone (used by parseFStrContent sub-parser)
    ExprPtr parseExpr() { return parseAssign(); }

    const std::vector<std::string>& errors() const { return m_errors; }
    bool hasErrors() const { return !m_errors.empty(); }

private:
    std::vector<Token>       m_tokens;
    std::string              m_filename;
    size_t                   m_pos = 0;
    std::vector<std::string> m_errors;

    // -----------------------------------------------------------------------
    // Token helpers
    // -----------------------------------------------------------------------
    Token& peek(int offset = 0) {
        size_t i = m_pos + offset;
        if (i >= m_tokens.size()) return m_tokens.back();
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
        while (!atEnd()) {
            auto t = peek().type;
            if (t == TokenType::Semicolon) { advance(); return; }
            if (t == TokenType::RBrace)    return;
            if (t == TokenType::Func   ||
                t == TokenType::Node   ||
                t == TokenType::Let    ||
                t == TokenType::Const  ||
                t == TokenType::If     ||
                t == TokenType::While  ||
                t == TokenType::For    ||
                t == TokenType::Return) return;
            advance();
        }
    }

    // -----------------------------------------------------------------------
    // F-string parser
    // -----------------------------------------------------------------------
    std::unique_ptr<FStrLitExpr> parseFStrContent(
        const std::string& raw, int line, int col)
    {
        std::vector<std::string> strParts;
        std::vector<ExprPtr>     exprParts;
        std::string lit;
        size_t i = 0;

        while (i < raw.size()) {
            char c = raw[i];

            if (c == '{' && i + 1 < raw.size() && raw[i+1] == '{') {
                lit += '{'; i += 2; continue;
            }
            if (c == '}' && i + 1 < raw.size() && raw[i+1] == '}') {
                lit += '}'; i += 2; continue;
            }

            if (c == '{') {
                strParts.push_back(lit); lit.clear();
                i++;
                std::string exprSrc;
                int depth = 1;
                while (i < raw.size() && depth > 0) {
                    if      (raw[i] == '{') { depth++; exprSrc += raw[i]; }
                    else if (raw[i] == '}') { depth--; if (depth > 0) exprSrc += raw[i]; }
                    else                    { exprSrc += raw[i]; }
                    i++;
                }
                Lexer subLexer(exprSrc, "<fstring>");
                auto subToks = subLexer.tokenize();
                if (!subLexer.hasErrors() && !subToks.empty()) {
                    Parser subParser(std::move(subToks), "<fstring>");
                    auto expr = subParser.parseExpr();
                    if (expr) {
                        exprParts.push_back(std::move(expr));
                    } else {
                        exprParts.push_back(
                            std::make_unique<IdentExpr>(exprSrc, line, col));
                    }
                } else {
                    exprParts.push_back(
                        std::make_unique<IdentExpr>(exprSrc, line, col));
                }
                continue;
            }

            lit += raw[i++];
        }
        strParts.push_back(lit);

        return std::make_unique<FStrLitExpr>(
            std::move(strParts), std::move(exprParts), line, col);
    }

    // -----------------------------------------------------------------------
    // Type annotations
    // -----------------------------------------------------------------------
    TypeAnnotation parseType() {
        TypeAnnotation ta;

        // Raw pointer: *T or *mut T
        if (check(TokenType::Star)) {
            advance(); ta.isPtr=true; ta.isPtrMut=match(TokenType::Mut);
            auto inner=parseType();
            ta.base=inner.base; ta.typeParams=inner.typeParams;
            ta.isArray=inner.isArray; ta.isFuncType=inner.isFuncType;
            ta.funcParamTypes=inner.funcParamTypes;
            if(inner.funcReturnType) ta.funcReturnType=std::make_unique<TypeAnnotation>(*inner.funcReturnType);
            return ta;
        }

        // Function type: func(I32, Str) -> Bool
        if (check(TokenType::Func)) {
            advance();
            ta.isFuncType = true;
            expect(TokenType::LParen, "expected '(' in function type");
            while (!check(TokenType::RParen) && !atEnd()) {
                if (check(TokenType::Mut)) advance(); // skip mut in type pos
                // Allow "name: Type" or just "Type"
                if (peek(1).type == TokenType::Colon) { advance(); advance(); }
                ta.funcParamTypes.push_back(parseType());
                if (!match(TokenType::Comma)) break;
            }
            expect(TokenType::RParen, "expected ')'");
            if (match(TokenType::Arrow))
                ta.funcReturnType = std::make_unique<TypeAnnotation>(parseType());
            if (match(TokenType::Question)) ta.nullable = true;
            return ta;
        }

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

        auto& tok = peek();
        switch (tok.type) {
            case TokenType::TI8:     ta.base = "I8";     break;
            case TokenType::TI16:    ta.base = "I16";    break;
            case TokenType::TI32:    ta.base = "I32";    break;
            case TokenType::TI64:    ta.base = "I64";    break;
            case TokenType::TU8:     ta.base = "U8";     break;
            case TokenType::TU16:    ta.base = "U16";    break;
            case TokenType::TU32:    ta.base = "U32";    break;
            case TokenType::TU64:    ta.base = "U64";    break;
            case TokenType::TF32:    ta.base = "F32";    break;
            case TokenType::TF64:    ta.base = "F64";    break;
            case TokenType::TBool:   ta.base = "Bool";   break;
            case TokenType::TStr:    ta.base = "str";    break;
            case TokenType::TString: ta.base = "String"; break;
            case TokenType::TVec2:   ta.base = "Vec2";   break;
            case TokenType::Ident:   ta.base = tok.value; break;
            default:
                error("expected type, got '" + tok.value + "'");
        }
        advance();

        // Parse generic type parameters: Result<Str>, HashMap<Str, I32>, etc.
        // Detect '<' only when it follows a known generic type name to avoid
        // ambiguity with less-than in expressions.
        static const std::unordered_set<std::string> genericTypes = {
            "Result", "HashMap", "Option", "Array", "Vec"
        };
        // Generic instantiation: works for both builtins and user types
        if (check(TokenType::Lt)) {
            size_t tmp = m_pos + 1; int depth = 1; bool ok = false;
            auto isTypeStart = [](TokenType t) {
                switch(t) { case TokenType::Ident: case TokenType::TI8: case TokenType::TI16:
                  case TokenType::TI32: case TokenType::TI64: case TokenType::TU8:
                  case TokenType::TU16: case TokenType::TU32: case TokenType::TU64:
                  case TokenType::TF32: case TokenType::TF64: case TokenType::TBool:
                  case TokenType::TStr: case TokenType::TString: case TokenType::LBracket:
                    return true; default: return false; } };
            while (tmp < m_tokens.size() && depth > 0) {
                auto tt = m_tokens[tmp].type;
                if (tt == TokenType::Lt) depth++;
                else if (tt == TokenType::Gt) { depth--; if (!depth) { ok=true; break; } }
                else if (tt == TokenType::Semicolon || tt == TokenType::LBrace ||
                         tt == TokenType::Assign || tt == TokenType::LParen) break;
                tmp++;
            }
            if (ok && tmp > m_pos+1 && isTypeStart(m_tokens[m_pos+1].type)) {
                advance();
                while (!check(TokenType::Gt) && !atEnd()) {
                    ta.typeParams.push_back(parseType());
                    if (!match(TokenType::Comma)) break;
                }
                expect(TokenType::Gt, "expected '>'");
            }
        }

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
        if (check(TokenType::Hash)) {
            auto attrs=parseAttributes(); bool isPub=match(TokenType::Pub);
            if (check(TokenType::Func)) { auto fd=parseFuncDeclInner(isPub); fd->attributes=attrs; return std::make_unique<FuncDecl>(std::move(*fd)); }
            if (check(TokenType::Struct)) { auto sd=static_cast<StructDecl*>(parseStructDecl(isPub).release()); sd->attributes=attrs; return StmtPtr(sd); }
            error("expected func or struct after attributes"); return nullptr;
        }
        if (check(TokenType::Interface)) return parseInterfaceDecl(false);
        if (check(TokenType::Ident)&&peek().value=="extern"){advance();return parseExternDecl();}
        if (check(TokenType::Ident)&&peek().value=="asm")   {advance();return parseAsmStmt();}

        error("unexpected token '" + peek().value + "' at top level");
        return nullptr;
    }

    StmtPtr parsePubDecl() {
        advance(); // pub
        if (check(TokenType::Func))   return parseFuncDecl(true);
        if (check(TokenType::Node))   return parseNodeDecl(true);
        if (check(TokenType::Struct)) return parseStructDecl(true);
        if (check(TokenType::Class))     return parseClassDecl(true);
        if (check(TokenType::Interface)) return parseInterfaceDecl(true);
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
            if (check(TokenType::Func))
                methods.push_back(parseFuncDeclInner(isPub));
            else if (check(TokenType::Let))
                fields.push_back(parseFieldDecl(isPub));
            else if (check(TokenType::Const))
                fields.push_back(parseConstFieldDecl(isPub));
            else
                error("expected field or method in node body");
        }
        expect(TokenType::RBrace, "expected '}' after node body");

        return std::make_unique<NodeDecl>(name, base,
            std::move(fields), std::move(methods), l, c);
    }

    StmtPtr parseStructDecl(bool pub) {
        int l = peek().line, c = peek().col;
        advance(); // struct
        std::string name = expect(TokenType::Ident, "expected struct name").value;
        auto typeParams = parseTypeParams();
        expect(TokenType::LBrace, "expected '{'");

        std::vector<FieldDecl> fields;
        while (!check(TokenType::RBrace) && !atEnd()) {
            bool isPub = match(TokenType::Pub);
            fields.push_back(parseFieldDecl(isPub));
        }
        expect(TokenType::RBrace, "expected '}'");
        return std::make_unique<StructDecl>(name, std::move(typeParams), std::move(fields), l, c);
    }

    StmtPtr parseEnumDecl() {
        int l = peek().line, c = peek().col;
        advance(); // enum
        std::string name = expect(TokenType::Ident, "expected enum name").value;
        expect(TokenType::LBrace, "expected '{'");

        std::vector<EnumVariant> variants;
        while (!check(TokenType::RBrace) && !atEnd()) {
            EnumVariant v;
            // Allow keywords as variant names (e.g. Bool, Str, Int, Float)
            if (check(TokenType::Ident) || peek().type >= TokenType::TI8) {
                v.name = advance().value;
            } else {
                v.name = expect(TokenType::Ident, "expected variant name").value;
            }
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
        auto typeParams = parseTypeParams();
        std::string base;
        std::vector<std::string> ifaces;
        if (match(TokenType::Colon))
            base = expect(TokenType::Ident, "expected base class").value;
        if (match(TokenType::Implements)) {
            ifaces.push_back(expect(TokenType::Ident,"expected interface name").value);
            while (match(TokenType::Comma))
                ifaces.push_back(expect(TokenType::Ident,"expected interface name").value);
        }

        expect(TokenType::LBrace, "expected '{'");

        std::vector<FieldDecl> fields;
        std::vector<std::unique_ptr<FuncDecl>> methods;

        while (!check(TokenType::RBrace) && !atEnd()) {
            bool isPub = match(TokenType::Pub);
            if (check(TokenType::Func))
                methods.push_back(parseFuncDeclInner(isPub));
            else if (check(TokenType::Let))
                fields.push_back(parseFieldDecl(isPub));
            else if (check(TokenType::Const))
                fields.push_back(parseConstFieldDecl(isPub));
            else
                error("expected field or method in class body");
        }
        expect(TokenType::RBrace, "expected '}'");
        return std::make_unique<ClassDecl>(name, std::move(typeParams), base,
            std::move(ifaces), std::move(fields), std::move(methods), l, c);
    }

    // Parses: let [mut] name: Type = expr;
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

    // Parses: const name: Type = expr;
    // Treated as an immutable field (mut = false) inside node/class bodies.
    FieldDecl parseConstFieldDecl(bool pub) {
        FieldDecl fd;
        fd.pub = pub;
        fd.mut = false;
        advance(); // const
        fd.name = expect(TokenType::Ident, "expected field name").value;
        expect(TokenType::Colon, "expected ':' after field name");
        fd.type = parseType();
        if (match(TokenType::Assign))
            fd.init = parseExpr();
        expect(TokenType::Semicolon, "expected ';' after const field");
        return fd;
    }

    StmtPtr parseInterfaceDecl(bool pub) {
        int l=peek().line, c=peek().col;
        advance(); // interface
        std::string name = expect(TokenType::Ident,"expected interface name").value;
        auto typeParams = parseTypeParams();
        expect(TokenType::LBrace,"expected '{'");
        std::vector<InterfaceMethod> methods;
        while (!check(TokenType::RBrace) && !atEnd()) {
            if (!check(TokenType::Func)) { advance(); continue; }
            advance(); // func
            std::string mname = expect(TokenType::Ident,"expected method name").value;
            expect(TokenType::LParen,"expected '('");
            std::vector<Param> params;
            while (!check(TokenType::RParen) && !atEnd()) {
                Param p;
                p.mut = match(TokenType::Mut);
                p.name = expect(TokenType::Ident,"expected param name").value;
                if (match(TokenType::Colon)) p.type = parseType();
                params.push_back(std::move(p));
                if (!match(TokenType::Comma)) break;
            }
            expect(TokenType::RParen,"expected ')'");
            std::optional<TypeAnnotation> ret;
            if (match(TokenType::Arrow)) ret = parseType();
            // Optional default body
            std::unique_ptr<BlockStmt> body;
            if (check(TokenType::LBrace)) {
                body = parseBlock();
            } else {
                match(TokenType::Semicolon);
            }
            InterfaceMethod im;
            im.name = mname; im.params = std::move(params);
            im.returnType = ret; im.defaultBody = std::move(body);
            methods.push_back(std::move(im));
        }
        expect(TokenType::RBrace,"expected '}'");
        return std::make_unique<InterfaceDecl>(name,std::move(typeParams),
                                               std::move(methods),l,c);
    }

    StmtPtr parseFuncDecl(bool pub) {
        return std::make_unique<FuncDecl>(
            std::move(*parseFuncDeclInner(pub)));
    }

    std::vector<std::string> parseAttributes() {
        std::vector<std::string> attrs;
        while(check(TokenType::Hash)){
            advance();
            expect(TokenType::LBracket,"expected '['");
            std::string attr;
            while(!check(TokenType::RBracket)&&!atEnd()) attr+=advance().value;
            expect(TokenType::RBracket,"expected ']'");
            attrs.push_back(attr);
        }
        return attrs;
    }

    StmtPtr parseExternDecl() {
        int l=peek().line,c=peek().col;
        std::string linkage="C";
        if(check(TokenType::Str)) linkage=advance().value;
        if(!check(TokenType::Func)){error("expected 'func' after 'extern'");return nullptr;}
        advance();
        std::string name=expect(TokenType::Ident,"expected function name").value;
        expect(TokenType::LParen,"expected '('");
        std::vector<ExternParam> params; bool variadic=false;
        while(!check(TokenType::RParen)&&!atEnd()){
            if((check(TokenType::DotDot)&&peek(1).type==TokenType::Dot)||
               (check(TokenType::Dot)&&peek(1).type==TokenType::DotDot)){
                while(!check(TokenType::RParen)&&!atEnd())advance();
                variadic=true; break;
            }
            ExternParam p;
            if(peek(1).type==TokenType::Colon){p.name=advance().value;advance();}
            p.type=parseType(); params.push_back(std::move(p));
            if(!match(TokenType::Comma))break;
        }
        expect(TokenType::RParen,"expected ')'");
        TypeAnnotation ret; if(match(TokenType::Arrow)) ret=parseType();
        match(TokenType::Semicolon);
        return std::make_unique<ExternDecl>(name,std::move(params),ret,linkage,variadic,l,c);
    }

    StmtPtr parseAsmStmt() {
        int l=peek().line,c=peek().col;
        expect(TokenType::LParen,"expected '('");
        std::string tmpl=expect(TokenType::Str,"expected asm template string").value;
        std::vector<std::string> outs,ins,clobs;
        std::vector<ExprPtr> outExprs,inExprs;
        if(match(TokenType::Colon)){
            while(!check(TokenType::Colon)&&!check(TokenType::RParen)&&!atEnd()){
                std::string con=expect(TokenType::Str,"expected constraint").value;
                expect(TokenType::LParen,"expected '('");
                outExprs.push_back(parseExpr());
                expect(TokenType::RParen,"expected ')'");
                outs.push_back(con); if(!match(TokenType::Comma))break;
            }
        }
        if(match(TokenType::Colon)){
            while(!check(TokenType::Colon)&&!check(TokenType::RParen)&&!atEnd()){
                std::string con=expect(TokenType::Str,"expected constraint").value;
                expect(TokenType::LParen,"expected '('");
                inExprs.push_back(parseExpr());
                expect(TokenType::RParen,"expected ')'");
                ins.push_back(con); if(!match(TokenType::Comma))break;
            }
        }
        if(match(TokenType::Colon)){
            while(!check(TokenType::RParen)&&!atEnd()){
                clobs.push_back(expect(TokenType::Str,"expected clobber").value);
                if(!match(TokenType::Comma))break;
            }
        }
        expect(TokenType::RParen,"expected ')'");
        match(TokenType::Semicolon);
        return std::make_unique<AsmStmt>(tmpl,std::move(outs),std::move(outExprs),
            std::move(ins),std::move(inExprs),std::move(clobs),l,c);
    }

    std::vector<std::string> parseTypeParams() {
        std::vector<std::string> tp;
        if (!check(TokenType::Lt)) return tp;
        advance();
        while (!check(TokenType::Gt) && !atEnd()) {
            tp.push_back(expect(TokenType::Ident, "expected type param name").value);
            if (!match(TokenType::Comma)) break;
        }
        expect(TokenType::Gt, "expected '>'");
        return tp;
    }

    std::unique_ptr<FuncDecl> parseFuncDeclInner(bool pub) {
        int l = peek().line, c = peek().col;
        advance(); // func
        std::string name = expect(TokenType::Ident, "expected function name").value;
        auto typeParams = parseTypeParams();

        expect(TokenType::LParen, "expected '('");
        std::vector<Param> params;
        while (!check(TokenType::RParen) && !atEnd()) {
            Param p;
            p.mut  = match(TokenType::Mut);
            p.name = expect(TokenType::Ident, "expected parameter name").value;
            // self param has no type annotation
            if (p.name != "self") {
                expect(TokenType::Colon, "expected ':' after parameter name");
                p.type = parseType();
            }
            params.push_back(std::move(p));
            if (!match(TokenType::Comma)) break;
        }
        expect(TokenType::RParen, "expected ')'");

        std::optional<TypeAnnotation> ret;
        if (match(TokenType::Arrow))
            ret = parseType();

        auto body = parseBlock();

        bool isCoro = false;
        if (body) {
            std::function<bool(const BlockStmt*)> hw;
            hw = [&](const BlockStmt* blk) -> bool {
                if (!blk) return false;
                for (auto& s : blk->stmts) {
                    if (s->kind == Stmt::Kind::Wait) return true;
                    if (s->kind == Stmt::Kind::If) { auto* i = static_cast<const IfStmt*>(s.get()); if (hw(i->then_.get())||hw(i->else_.get())) return true; }
                    if (s->kind == Stmt::Kind::While) { if (hw(static_cast<const WhileStmt*>(s.get())->body.get())) return true; }
                    if (s->kind == Stmt::Kind::Loop)  { if (hw(static_cast<const LoopStmt*>(s.get())->body.get()))  return true; }
                    if (s->kind == Stmt::Kind::ForIn) { if (hw(static_cast<const ForInStmt*>(s.get())->body.get())) return true; }
                    if (s->kind == Stmt::Kind::Block) { if (hw(static_cast<const BlockStmt*>(s.get()))) return true; }
                }
                return false;
            };
            isCoro = hw(body.get());
        }
        auto fd = std::make_unique<FuncDecl>(
            name, std::move(typeParams), std::move(params), ret, std::move(body), pub, l, c);
        fd->isCoroutine = isCoro;
        return fd;
    }

    // -----------------------------------------------------------------------
    // Statements
    // -----------------------------------------------------------------------
    StmtPtr parseStmt() {
        int l = peek().line, c = peek().col;

        // Labelled loops: 'outer: while / loop / for
        if (check(TokenType::Apostrophe)) {
            std::string lbl = advance().value; // e.g. "'outer"
            if (!lbl.empty() && lbl[0] == '\'')
                lbl = lbl.substr(1); // strip leading apostrophe
            expect(TokenType::Colon, "expected ':' after loop label");
            int ll = peek().line, lc = peek().col;
            if (check(TokenType::While)) {
                advance();
                auto cond = parseExpr(); auto body = parseBlock();
                return std::make_unique<WhileStmt>(std::move(cond), std::move(body), lbl, ll, lc);
            } else if (check(TokenType::Loop)) {
                advance();
                auto body = parseBlock();
                return std::make_unique<LoopStmt>(std::move(body), lbl, ll, lc);
            } else if (check(TokenType::For)) {
                advance();
                std::string v2 = expect(TokenType::Ident, "expected variable").value;
                TypeAnnotation t2;
                if (check(TokenType::Colon)) { advance(); t2 = parseType(); }
                if (match(TokenType::In)) {
                    auto iter = parseExpr(); auto body = parseBlock();
                    return std::make_unique<ForInStmt>(v2, t2, std::move(iter), std::move(body), lbl, ll, lc);
                }
                expect(TokenType::Assign, "expected '='");
                auto i2 = parseExpr();
                expect(TokenType::Semicolon, "expected ';'");
                auto c2 = parseExpr();
                expect(TokenType::Semicolon, "expected ';'");
                auto s2 = parseExpr(); auto b2 = parseBlock();
                return std::make_unique<ForCStmt>(v2, t2, std::move(i2), std::move(c2), std::move(s2), std::move(b2), lbl, ll, lc);
            }
            error("expected 'while', 'loop', or 'for' after loop label");
            return nullptr;
        }

        if (check(TokenType::Hash)) {
            auto attrs=parseAttributes(); bool isPub=match(TokenType::Pub);
            if (check(TokenType::Func)){auto fd=parseFuncDeclInner(isPub);fd->attributes=attrs;return std::make_unique<FuncDecl>(std::move(*fd));}
            if (check(TokenType::Struct)){auto sd=static_cast<StructDecl*>(parseStructDecl(isPub).release());sd->attributes=attrs;return StmtPtr(sd);}
            error("expected func or struct after attributes");return nullptr;
        }
        if(check(TokenType::Ident)&&peek().value=="extern"){advance();return parseExternDecl();}
        if(check(TokenType::Ident)&&peek().value=="asm")   {advance();return parseAsmStmt();}
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
                auto stmt = parseStmt();
                if (stmt) stmts.push_back(std::move(stmt));
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

    static std::string stripLabel(const std::string& lbl) {
        return (!lbl.empty() && lbl[0] == '\'') ? lbl.substr(1) : lbl;
    }
    StmtPtr parseBreak() {
        int l = peek().line, c = peek().col;
        advance(); // break
        std::string label;
        if (check(TokenType::Apostrophe))
            label = stripLabel(advance().value);
        expect(TokenType::Semicolon, "expected ';'");
        return std::make_unique<BreakStmt>(label, l, c);
    }

    StmtPtr parseContinue() {
        int l = peek().line, c = peek().col;
        advance(); // continue
        std::string label;
        if (check(TokenType::Apostrophe))
            label = stripLabel(advance().value);
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
        return std::make_unique<WhileStmt>(std::move(cond), std::move(body), "", l, c);
    }

    StmtPtr parseLoop() {
        int l = peek().line, c = peek().col;
        advance(); // loop
        auto body = parseBlock();
        return std::make_unique<LoopStmt>(std::move(body), "", l, c);
    }

    StmtPtr parseFor() {
        int l = peek().line, c = peek().col;
        advance(); // for

        std::string label;
        if (check(TokenType::Apostrophe)) {
            label = advance().value;
            expect(TokenType::Colon, "expected ':' after label");
        }

        std::string var = expect(TokenType::Ident, "expected variable name").value;
        // Type annotation is optional: "for x in arr" or "for x: T in arr"
        TypeAnnotation type;
        if (check(TokenType::Colon)) {
            advance();
            type = parseType();
        }

        if (match(TokenType::In)) {
            auto iterable = parseExpr();
            auto body = parseBlock();
            return std::make_unique<ForInStmt>(
                var, type, std::move(iterable), std::move(body), label, l, c);
        } else {
            expect(TokenType::Assign, "expected '=' or 'in'");
            auto init = parseExpr();
            expect(TokenType::Semicolon, "expected ';'");
            auto cond = parseExpr();
            expect(TokenType::Semicolon, "expected ';'");
            auto step = parseExpr();
            auto body = parseBlock();
            return std::make_unique<ForCStmt>(
                var, type, std::move(init), std::move(cond),
                std::move(step), std::move(body), "", l, c);
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
        while (check(TokenType::Lt)   || check(TokenType::Gt) ||
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
            return std::make_unique<UnaryExpr>("-", parseUnary(), false, l, c);
        }
        if (check(TokenType::Bang)) {
            advance();
            return std::make_unique<UnaryExpr>("!", parseUnary(), false, l, c);
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
                // Allow keywords as member names: TokenKind::Bool, SomeEnum::Str etc.
                std::string member;
                if (check(TokenType::Ident) || (
                    peek().type != TokenType::LParen &&
                    peek().type != TokenType::Semicolon &&
                    peek().type != TokenType::RBrace &&
                    peek().type != TokenType::RParen &&
                    peek().type != TokenType::Comma &&
                    peek().type != TokenType::Eof)) {
                    member = advance().value;
                } else {
                    member = expect(TokenType::Ident, "expected member name").value;
                }
                expr = std::make_unique<MemberExpr>(
                    std::move(expr), member, safe, l, c);

            } else if (check(TokenType::LParen)) {
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
                advance();
                auto idx = parseExpr();
                expect(TokenType::RBracket, "expected ']'");
                expr = std::make_unique<IndexExpr>(
                    std::move(expr), std::move(idx), l, c);

            } else if (check(TokenType::PlusPlus)) {
                advance();
                expr = std::make_unique<UnaryExpr>("++", std::move(expr), true, l, c);

            } else if (check(TokenType::MinusMinus)) {
                advance();
                expr = std::make_unique<UnaryExpr>("--", std::move(expr), true, l, c);

            } else if (check(TokenType::Bang)) {
                advance();
                expr = std::make_unique<ForceUnwrapExpr>(std::move(expr), l, c);

            // Error propagation: expr? — unwrap Result or early-return Err
            } else if (check(TokenType::Question)) {
                advance();
                expr = std::make_unique<PropagateExpr>(std::move(expr), l, c);

            } else if (check(TokenType::DotDot) || check(TokenType::DotDotEq)) {
                bool inc = check(TokenType::DotDotEq);
                advance();
                auto to = parseUnary();
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

        if (check(TokenType::Int)) {
            std::string v = advance().value;
            int64_t val = v.find("0x") == 0
                ? std::stoll(v, nullptr, 16)
                : std::stoll(v);
            return std::make_unique<IntLitExpr>(val, l, c);
        }

        if (check(TokenType::Float)) {
            std::string raw = peek().value;
            double val = std::stod(advance().value);
            return std::make_unique<FloatLitExpr>(val, raw, l, c);
        }

        if (check(TokenType::Bool)) {
            bool val = advance().value == "true";
            return std::make_unique<BoolLitExpr>(val, l, c);
        }

        if (check(TokenType::FStr)) {
            auto tok = advance();
            return parseFStrContent(tok.value, tok.line, tok.col);
        }

        if (check(TokenType::Str)) {
            return std::make_unique<StrLitExpr>(advance().value, l, c);
        }

        if (check(TokenType::Null)) {
            advance();
            return std::make_unique<NullLitExpr>(l, c);
        }

        if (check(TokenType::None_)) {
            advance();
            return std::make_unique<NoneLitExpr>(l, c);
        }

        if (check(TokenType::Some)) {
            advance();
            expect(TokenType::LParen, "expected '(' after Some");
            auto val = parseExpr();
            expect(TokenType::RParen, "expected ')'");
            return std::make_unique<SomeExpr>(std::move(val), l, c);
        }

        // Dereference: *ptr
        if (check(TokenType::Star)) {
            advance(); auto operand=parsePrimary();
            return std::make_unique<DerefExpr>(std::move(operand),l,c);
        }
        // Address-of: &expr or &mut expr
        if (check(TokenType::And)) {
            advance(); bool m=match(TokenType::Mut); auto operand=parsePrimary();
            return std::make_unique<AddrOfExpr>(std::move(operand),m,l,c);
        }
        // Closure: func(x: I32) -> I32 { return x * 2; }
        if (check(TokenType::Func)) {
            advance();
            expect(TokenType::LParen, "expected '(' in closure");
            std::vector<Param> cparams;
            while (!check(TokenType::RParen) && !atEnd()) {
                Param p;
                p.mut  = match(TokenType::Mut);
                p.name = expect(TokenType::Ident, "expected parameter name").value;
                expect(TokenType::Colon, "expected ':'");
                p.type = parseType();
                cparams.push_back(std::move(p));
                if (!match(TokenType::Comma)) break;
            }
            expect(TokenType::RParen, "expected ')'");
            std::optional<TypeAnnotation> cret;
            if (match(TokenType::Arrow)) cret = parseType();
            auto cbody = parseBlock();
            return std::make_unique<FuncExpr>(
                std::move(cparams), cret, std::move(cbody), l, c);
        }

        if (check(TokenType::Spawn)) {
            advance();
            auto call = parsePostfix();
            return std::make_unique<SpawnExpr>(std::move(call), l, c);
        }

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

        if (check(TokenType::Ident)) {
            std::string name = advance().value;

            // Struct init: Name { field: val, ... }
            if (check(TokenType::LBrace)) {
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

        // Type keywords used as constructor calls in expression position.
        // e.g. Vec2(0.0, 0.0), Color(1.0, 0.0, 0.0, 1.0)
        // These are tokenized as type keywords rather than Ident, so we
        // handle them here by re-treating them as identifier expressions.
        static const std::vector<TokenType> typeKeywordsAsCtors = {
            TokenType::TVec2,
            TokenType::TI8,  TokenType::TI16, TokenType::TI32, TokenType::TI64,
            TokenType::TU8,  TokenType::TU16, TokenType::TU32, TokenType::TU64,
            TokenType::TF32, TokenType::TF64,
            TokenType::TBool, TokenType::TStr, TokenType::TString,
        };
        for (auto kt : typeKeywordsAsCtors) {
            if (check(kt)) {
                std::string name = advance().value;
                return std::make_unique<IdentExpr>(name, l, c);
            }
        }

        error("unexpected token '" + peek().value + "'");
        return nullptr;
    }
};

} // namespace KonScript
