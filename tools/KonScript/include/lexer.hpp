#pragma once
// -----------------------------------------------------------------------
// KonScript Lexer — with f-string support
// -----------------------------------------------------------------------
#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <cctype>

namespace KonScript {

// -----------------------------------------------------------------------
// Token types
// -----------------------------------------------------------------------
enum class TokenType {
    // Literals
    Int,        // 42
    Float,      // 3.14
    Bool,       // true false
    Str,        // "hello"
    FStr,       // f"hello {name}" or auto-detected "hello {name}"
    Null,       // null
    None_,      // None
    Some,       // Some

    // Identifiers and keywords
    Ident,

    // Keywords
    Let, Mut, Const, Func, Return,
    If, Else, While, Loop, For, In,
    Break, Continue, Switch, Case, Default,
    Node, Struct, Enum, Class, Pub, As,
    Spawn, Wait, Extends,

    // Type keywords
    TI8, TI16, TI32, TI64,
    TU8, TU16, TU32, TU64,
    TF32, TF64,
    TBool, TStr, TString, TVec2,

    // Symbols
    LParen, RParen, LBrace, RBrace, LBracket, RBracket,
    Semicolon, Colon, ColonColon, Comma,
    Dot, DotDot, DotDotEq,
    Arrow, FatArrow,
    Hash, Question, Bang, Apostrophe,

    // Operators
    Plus, Minus, Star, Slash, Percent,
    PlusEq, MinusEq, StarEq, SlashEq,
    PlusPlus, MinusMinus,

    // Comparison
    Eq, NotEq, Lt, Gt, LtEq, GtEq,

    // Logic
    And, Or,

    // Assignment
    Assign,

    // Null operators
    NullCoal, SafeDot, ForceUnwrap,

    // Preprocessor
    Include,

    // Special
    Eof, Newline,
};

// -----------------------------------------------------------------------
// Token
// -----------------------------------------------------------------------
struct Token {
    TokenType   type;
    std::string value;
    int         line;
    int         col;

    std::string toString() const {
        return "[" + std::to_string(line) + ":" + std::to_string(col) +
               " " + value + "]";
    }
};

// -----------------------------------------------------------------------
// Keywords map
// -----------------------------------------------------------------------
static const std::unordered_map<std::string, TokenType> KEYWORDS = {
    {"let",      TokenType::Let},
    {"mut",      TokenType::Mut},
    {"const",    TokenType::Const},
    {"func",     TokenType::Func},
    {"return",   TokenType::Return},
    {"if",       TokenType::If},
    {"else",     TokenType::Else},
    {"while",    TokenType::While},
    {"loop",     TokenType::Loop},
    {"for",      TokenType::For},
    {"in",       TokenType::In},
    {"break",    TokenType::Break},
    {"continue", TokenType::Continue},
    {"switch",   TokenType::Switch},
    {"case",     TokenType::Case},
    {"default",  TokenType::Default},
    {"node",     TokenType::Node},
    {"struct",   TokenType::Struct},
    {"enum",     TokenType::Enum},
    {"class",    TokenType::Class},
    {"pub",      TokenType::Pub},
    {"as",       TokenType::As},
    {"spawn",    TokenType::Spawn},
    {"wait",     TokenType::Wait},
    {"extends",  TokenType::Extends},
    {"true",     TokenType::Bool},
    {"false",    TokenType::Bool},
    {"null",     TokenType::Null},
    {"None",     TokenType::None_},
    {"Some",     TokenType::Some},
    // Types
    {"I8",       TokenType::TI8},
    {"I16",      TokenType::TI16},
    {"I32",      TokenType::TI32},
    {"I64",      TokenType::TI64},
    {"U8",       TokenType::TU8},
    {"U16",      TokenType::TU16},
    {"U32",      TokenType::TU32},
    {"U64",      TokenType::TU64},
    {"F32",      TokenType::TF32},
    {"F64",      TokenType::TF64},
    {"Bool",     TokenType::TBool},
    {"str",      TokenType::TStr},
    {"String",   TokenType::TString},
    {"Vec2",     TokenType::TVec2},
};

// -----------------------------------------------------------------------
// Lexer
// -----------------------------------------------------------------------
class Lexer {
public:
    struct Error { std::string message; int line, col; };

    Lexer(const std::string& source, const std::string& filename = "<input>")
        : m_src(source), m_filename(filename) {}

    std::vector<Token> tokenize() {
        m_pos = 0; m_line = 1; m_col = 1;
        m_tokens.clear();
        m_errors.clear();

        while (!atEnd()) {
            skipWhitespaceAndComments();
            if (atEnd()) break;

            int startLine = m_line;
            int startCol  = m_col;
            char c = peek();

            // f-string: f"..."
            if (c == 'f' && peek(1) == '"') {
                advance(); // consume 'f'
                m_tokens.push_back(readFString(startLine, startCol));
                continue;
            }

            // String literal — also auto-upgrade if it contains {expr}
            if (c == '"') {
                auto tok = readString(startLine, startCol);
                if (looksLikeFStr(tok.value))
                    tok.type = TokenType::FStr;
                m_tokens.push_back(tok);
                continue;
            }

            // Number
            if (std::isdigit(c)) {
                m_tokens.push_back(readNumber(startLine, startCol));
                continue;
            }

            // Label / lifetime apostrophe 'outer:
            if (c == '\'') {
                advance();
                std::string label;
                while (!atEnd() && (std::isalnum(peek()) || peek() == '_'))
                    label += advance();
                m_tokens.push_back({TokenType::Apostrophe, "'" + label, startLine, startCol});
                continue;
            }

            // Identifier or keyword
            if (std::isalpha(c) || c == '_') {
                m_tokens.push_back(readIdent(startLine, startCol));
                continue;
            }

            // Preprocessor #include
            if (c == '#') {
                advance();
                skipWhitespace();
                std::string directive;
                while (!atEnd() && std::isalpha(peek()))
                    directive += advance();
                if (directive == "include") {
                    m_tokens.push_back({TokenType::Include, "#include", startLine, startCol});
                    skipWhitespace();
                    char delim = peek();
                    bool isSys = (delim == '<');
                    advance();
                    std::string path;
                    char end = isSys ? '>' : '"';
                    while (!atEnd() && peek() != end && peek() != '\n')
                        path += advance();
                    if (!atEnd()) advance();
                    m_tokens.push_back({TokenType::Str,
                        (isSys ? "<" : "\"") + path + (isSys ? ">" : "\""),
                        startLine, startCol});
                } else {
                    // Non-#include # line (e.g. "# comment") — treat as line comment
                    while (!atEnd() && peek() != '\n') advance();
                }
                continue;
            }

            // Symbols and operators
            advance();
            switch (c) {
                case '(': emit(TokenType::LParen,    "(", startLine, startCol); break;
                case ')': emit(TokenType::RParen,    ")", startLine, startCol); break;
                case '{': emit(TokenType::LBrace,    "{", startLine, startCol); break;
                case '}': emit(TokenType::RBrace,    "}", startLine, startCol); break;
                case '[': emit(TokenType::LBracket,  "[", startLine, startCol); break;
                case ']': emit(TokenType::RBracket,  "]", startLine, startCol); break;
                case ';': emit(TokenType::Semicolon, ";", startLine, startCol); break;
                case ',': emit(TokenType::Comma,     ",", startLine, startCol); break;
                case '%': emit(TokenType::Percent,   "%", startLine, startCol); break;
                case ':':
                    if (match(':')) emit(TokenType::ColonColon, "::", startLine, startCol);
                    else            emit(TokenType::Colon,      ":",  startLine, startCol);
                    break;
                case '.':
                    if (match('.')) {
                        if (match('=')) emit(TokenType::DotDotEq, "..=", startLine, startCol);
                        else            emit(TokenType::DotDot,   "..",  startLine, startCol);
                    } else {
                        emit(TokenType::Dot, ".", startLine, startCol);
                    }
                    break;
                case '-':
                    if (match('>'))      emit(TokenType::Arrow,    "->", startLine, startCol);
                    else if (match('-')) emit(TokenType::MinusMinus,"--", startLine, startCol);
                    else if (match('=')) emit(TokenType::MinusEq,  "-=", startLine, startCol);
                    else                 emit(TokenType::Minus,    "-",  startLine, startCol);
                    break;
                case '+':
                    if (match('+'))      emit(TokenType::PlusPlus, "++", startLine, startCol);
                    else if (match('=')) emit(TokenType::PlusEq,   "+=", startLine, startCol);
                    else                 emit(TokenType::Plus,     "+",  startLine, startCol);
                    break;
                case '*':
                    if (match('=')) emit(TokenType::StarEq, "*=", startLine, startCol);
                    else            emit(TokenType::Star,   "*",  startLine, startCol);
                    break;
                case '/':
                    if (match('=')) emit(TokenType::SlashEq, "/=", startLine, startCol);
                    else            emit(TokenType::Slash,   "/",  startLine, startCol);
                    break;
                case '=':
                    if (match('='))      emit(TokenType::Eq,      "==", startLine, startCol);
                    else if (match('>')) emit(TokenType::FatArrow,"=>", startLine, startCol);
                    else                 emit(TokenType::Assign,  "=",  startLine, startCol);
                    break;
                case '!':
                    if (match('=')) emit(TokenType::NotEq,      "!=", startLine, startCol);
                    else            emit(TokenType::Bang,        "!",  startLine, startCol);
                    break;
                case '<':
                    if (match('=')) emit(TokenType::LtEq, "<=", startLine, startCol);
                    else            emit(TokenType::Lt,   "<",  startLine, startCol);
                    break;
                case '>':
                    if (match('=')) emit(TokenType::GtEq, ">=", startLine, startCol);
                    else            emit(TokenType::Gt,   ">",  startLine, startCol);
                    break;
                case '&':
                    if (match('&')) emit(TokenType::And, "&&", startLine, startCol);
                    break;
                case '|':
                    if (match('|')) emit(TokenType::Or, "||", startLine, startCol);
                    break;
                case '?':
                    if (match('?'))      emit(TokenType::NullCoal, "??", startLine, startCol);
                    else if (match('.')) emit(TokenType::SafeDot,  "?.", startLine, startCol);
                    else                 emit(TokenType::Question, "?",  startLine, startCol);
                    break;
                default:
                    // Skip non-ASCII bytes (UTF-8 multi-byte chars in comments etc.)
                    if ((unsigned char)c < 0x80)
                        error("unexpected character '" + std::string(1,c) + "'", startLine, startCol);
                    break;
            }
        }
        m_tokens.push_back({TokenType::Eof, "", m_line, m_col});
        return m_tokens;
    }

    bool hasErrors() const { return !m_errors.empty(); }
    const std::vector<Error>& errors() const { return m_errors; }

private:
    std::string        m_src;
    std::string        m_filename;
    size_t             m_pos  = 0;
    int                m_line = 1;
    int                m_col  = 1;
    std::vector<Token> m_tokens;
    std::vector<Error> m_errors;

    bool atEnd() const       { return m_pos >= m_src.size(); }
    char peek(int offset=0) const {
        return (m_pos + offset < m_src.size()) ? m_src[m_pos + offset] : '\0';
    }
    char advance() {
        char c = m_src[m_pos++];
        if (c == '\n') { m_line++; m_col = 1; } else { m_col++; }
        return c;
    }
    bool match(char c) {
        if (!atEnd() && peek() == c) { advance(); return true; }
        return false;
    }
    void emit(TokenType t, const std::string& v, int l, int c) {
        m_tokens.push_back({t, v, l, c});
    }
    void error(const std::string& msg, int l, int c) {
        m_errors.push_back({m_filename + ":" + std::to_string(l) + ":" +
                            std::to_string(c) + ": " + msg, l, c});
    }

    void skipWhitespace() {
        while (!atEnd() && (peek() == ' ' || peek() == '\t' || peek() == '\r'))
            advance();
    }

    void skipWhitespaceAndComments() {
        while (!atEnd()) {
            // Whitespace
            if (peek() == ' ' || peek() == '\t' || peek() == '\r' || peek() == '\n') {
                advance(); continue;
            }
            // Single-line comment
            if (peek() == '/' && peek(1) == '/') {
                while (!atEnd() && peek() != '\n') advance();
                continue;
            }
            // Multi-line comment
            if (peek() == '/' && peek(1) == '*') {
                advance(); advance();
                while (!atEnd()) {
                    if (peek() == '*' && peek(1) == '/') { advance(); advance(); break; }
                    advance();
                }
                continue;
            }
            break;
        }
    }

    // -----------------------------------------------------------------------
    // Check if a string value contains f-string interpolation patterns
    // -----------------------------------------------------------------------
    static bool looksLikeFStr(const std::string& s) {
        size_t i = 0;
        while (i < s.size()) {
            if (s[i] == '{') {
                if (i+1 < s.size() && s[i+1] == '{') { i += 2; continue; } // escaped {{
                size_t j = i + 1;
                while (j < s.size() && s[j] != '}' && s[j] != '\n') j++;
                if (j < s.size() && j > i + 1) return true;
            }
            i++;
        }
        return false;
    }

    // -----------------------------------------------------------------------
    // Read a plain string literal "..."
    // -----------------------------------------------------------------------
    Token readString(int line, int col) {
        advance(); // skip opening "
        std::string value;
        while (!atEnd() && peek() != '"') {
            if (peek() == '\\') {
                advance();
                switch (advance()) {
                    case 'n':  value += '\n'; break;
                    case 't':  value += '\t'; break;
                    case 'r':  value += '\r'; break;
                    case '"':  value += '"';  break;
                    case '\\': value += '\\'; break;
                    case '0':  value += '\0'; break;
                    default:   value += '?';  break;
                }
            } else {
                value += advance();
            }
        }
        if (atEnd()) error("unterminated string", line, col);
        else advance(); // skip closing "
        return {TokenType::Str, value, line, col};
    }

    // -----------------------------------------------------------------------
    // Read an f-string literal f"..." (the 'f' has already been consumed)
    // -----------------------------------------------------------------------
    Token readFString(int line, int col) {
        advance(); // skip opening "
        std::string value;
        while (!atEnd() && peek() != '"') {
            if (peek() == '\\') {
                advance();
                switch (advance()) {
                    case 'n':  value += '\n'; break;
                    case 't':  value += '\t'; break;
                    case 'r':  value += '\r'; break;
                    case '"':  value += '"';  break;
                    case '\\': value += '\\'; break;
                    case '{':  value += '{';  break; // \{ → literal {
                    case '}':  value += '}';  break; // \} → literal }
                    case '0':  value += '\0'; break;
                    default:   value += '?';  break;
                }
            } else {
                value += advance();
            }
        }
        if (atEnd()) error("unterminated f-string", line, col);
        else advance(); // skip closing "
        return {TokenType::FStr, value, line, col};
    }

    Token readNumber(int line, int col) {
        std::string value;
        bool isFloat = false;
        if (peek() == '0' && (peek(1) == 'x' || peek(1) == 'X')) {
            value += advance(); value += advance();
            while (!atEnd() && std::isxdigit(peek())) value += advance();
            return {TokenType::Int, value, line, col};
        }
        while (!atEnd() && std::isdigit(peek())) value += advance();
        if (!atEnd() && peek() == '.' && peek(1) != '.') {
            isFloat = true;
            value += advance();
            while (!atEnd() && std::isdigit(peek())) value += advance();
        }
        if (!atEnd() && (peek() == 'e' || peek() == 'E')) {
            isFloat = true;
            value += advance();
            if (!atEnd() && (peek() == '+' || peek() == '-')) value += advance();
            while (!atEnd() && std::isdigit(peek())) value += advance();
        }
        return {isFloat ? TokenType::Float : TokenType::Int, value, line, col};
    }

    Token readIdent(int line, int col) {
        std::string value;
        while (!atEnd() && (std::isalnum(peek()) || peek() == '_'))
            value += advance();
        auto it = KEYWORDS.find(value);
        TokenType type = (it != KEYWORDS.end()) ? it->second : TokenType::Ident;
        return {type, value, line, col};
    }
};

} // namespace KonScript
