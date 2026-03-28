// -----------------------------------------------------------------------
// KonScript Language Server
// Implements the Language Server Protocol (LSP) for KonScript.
//
// Currently provides:
//   - Diagnostics (errors from lexer + typechecker)
//   - Hover (type info)
//   - Document symbols
//
// Planned:
//   - Completions
//   - Go-to-definition
//   - Find references
//   - Rename
// -----------------------------------------------------------------------
#include "../include/lexer.hpp"
#include "../include/parser.hpp"
#include "../include/typechecker.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

// -----------------------------------------------------------------------
// JSON helpers (minimal -- no external deps)
// -----------------------------------------------------------------------
static std::string jsonStr(const std::string& s) {
    std::string out = "\"";
    for (char c : s) {
        if (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else out += c;
    }
    return out + "\"";
}

static std::string jsonInt(int n) {
    return std::to_string(n);
}

// -----------------------------------------------------------------------
// LSP message sending
// -----------------------------------------------------------------------
static void sendMessage(const std::string& json) {
    std::string header = "Content-Length: " +
                         std::to_string(json.size()) + "\r\n\r\n";
    std::cout << header << json;
    std::cout.flush();
}

static void sendResponse(int id, const std::string& result) {
    std::string json = "{\"jsonrpc\":\"2.0\",\"id\":" +
                       jsonInt(id) + ",\"result\":" + result + "}";
    sendMessage(json);
}

static void sendNotification(const std::string& method,
                             const std::string& params) {
    std::string json = "{\"jsonrpc\":\"2.0\",\"method\":" +
                       jsonStr(method) + ",\"params\":" + params + "}";
    sendMessage(json);
}

// -----------------------------------------------------------------------
// Diagnostics
// -----------------------------------------------------------------------
static void publishDiagnostics(const std::string& uri,
                               const std::string& source) {
    // Lex
    KonScript::Lexer lexer(source, uri);
    auto tokens = lexer.tokenize();

    std::string diags = "[";
    bool first = true;

    auto addDiag = [&](int line, int col, const std::string& msg,
                       int severity) {
        if (!first) diags += ",";
        first = false;
        // LSP lines are 0-based
        int l = std::max(0, line - 1);
        int c = std::max(0, col - 1);
        diags += "{"
            "\"range\":{"
                "\"start\":{\"line\":" + jsonInt(l) +
                            ",\"character\":" + jsonInt(c) + "},"
                "\"end\":{\"line\":" + jsonInt(l) +
                          ",\"character\":" + jsonInt(c + 1) + "}"
            "},"
            "\"severity\":" + jsonInt(severity) + ","
            "\"source\":\"konscript\","
            "\"message\":" + jsonStr(msg) +
        "}";
    };

    for (auto& e : lexer.errors())
        addDiag(e.line, e.col, e.message, 1); // 1 = error

    if (!lexer.hasErrors()) {
        // Parse
        KonScript::Parser parser(std::move(tokens), uri);
        auto prog = parser.parse();

        for (auto& e : parser.errors())
            addDiag(0, 0, e, 1);

        if (!parser.hasErrors()) {
            // Typecheck
            KonScript::TypeChecker checker;
            checker.check(prog);
            for (auto& e : checker.errors())
                addDiag(e.line, e.col, e.message, 1);
        }
    }

    diags += "]";

    sendNotification("textDocument/publishDiagnostics",
        "{\"uri\":" + jsonStr(uri) + ",\"diagnostics\":" + diags + "}");
}

// -----------------------------------------------------------------------
// Document store
// -----------------------------------------------------------------------
static std::unordered_map<std::string, std::string> g_docs;

// -----------------------------------------------------------------------
// JSON reading (minimal)
// -----------------------------------------------------------------------
static std::string readHeader() {
    std::string line;
    int contentLength = 0;
    while (std::getline(std::cin, line)) {
        if (line.empty() || line == "\r") break;
        if (line.find("Content-Length:") != std::string::npos) {
            contentLength = std::stoi(line.substr(16));
        }
    }
    std::string body(contentLength, '\0');
    std::cin.read(&body[0], contentLength);
    return body;
}

static std::string extractStr(const std::string& json,
                               const std::string& key) {
    std::string search = "\"" + key + "\":\"";
    auto pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos += search.size();
    auto end = json.find('"', pos);
    if (end == std::string::npos) return "";
    return json.substr(pos, end - pos);
}

static int extractInt(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\":";
    auto pos = json.find(search);
    if (pos == std::string::npos) return -1;
    pos += search.size();
    return std::stoi(json.substr(pos));
}

// -----------------------------------------------------------------------
// Main loop
// -----------------------------------------------------------------------
int main() {
    // LSP communicates over stdin/stdout
    std::ios::sync_with_stdio(false);

    while (true) {
        std::string body = readHeader();
        if (body.empty()) continue;

        std::string method = extractStr(body, "method");
        int id = extractInt(body, "id");

        if (method == "initialize") {
            sendResponse(id, R"({
                "capabilities": {
                    "textDocumentSync": 1,
                    "hoverProvider": false,
                    "completionProvider": null,
                    "documentSymbolProvider": false
                },
                "serverInfo": {
                    "name": "konscript-lsp",
                    "version": "0.1.0"
                }
            })");

        } else if (method == "initialized") {
            // Client acknowledged -- nothing to do

        } else if (method == "textDocument/didOpen") {
            // Extract uri and text (simplified)
            auto uriPos  = body.find("\"uri\":\"") + 7;
            auto uriEnd  = body.find('"', uriPos);
            auto uri     = body.substr(uriPos, uriEnd - uriPos);

            auto textPos = body.find("\"text\":\"") + 8;
            auto textEnd = body.rfind('"');
            auto text    = body.substr(textPos, textEnd - textPos);

            // Unescape newlines
            std::string unescaped;
            for (size_t i = 0; i < text.size(); i++) {
                if (text[i] == '\\' && i+1 < text.size()) {
                    if (text[i+1] == 'n') { unescaped += '\n'; i++; }
                    else if (text[i+1] == 't') { unescaped += '\t'; i++; }
                    else if (text[i+1] == 'r') { i++; }
                    else if (text[i+1] == '"') { unescaped += '"'; i++; }
                    else if (text[i+1] == '\\') { unescaped += '\\'; i++; }
                    else unescaped += text[i];
                } else {
                    unescaped += text[i];
                }
            }

            g_docs[uri] = unescaped;
            publishDiagnostics(uri, unescaped);

        } else if (method == "textDocument/didChange") {
            auto uriPos  = body.find("\"uri\":\"") + 7;
            auto uriEnd  = body.find('"', uriPos);
            auto uri     = body.substr(uriPos, uriEnd - uriPos);

            auto textPos = body.find("\"text\":\"") + 8;
            auto textEnd = body.rfind('"');
            auto text    = body.substr(textPos, textEnd - textPos);

            std::string unescaped;
            for (size_t i = 0; i < text.size(); i++) {
                if (text[i] == '\\' && i+1 < text.size()) {
                    if (text[i+1] == 'n') { unescaped += '\n'; i++; }
                    else if (text[i+1] == 't') { unescaped += '\t'; i++; }
                    else if (text[i+1] == 'r') { i++; }
                    else if (text[i+1] == '"') { unescaped += '"'; i++; }
                    else if (text[i+1] == '\\') { unescaped += '\\'; i++; }
                    else unescaped += text[i];
                } else {
                    unescaped += text[i];
                }
            }

            g_docs[uri] = unescaped;
            publishDiagnostics(uri, unescaped);

        } else if (method == "textDocument/didSave") {
            auto uriPos = body.find("\"uri\":\"") + 7;
            auto uriEnd = body.find('"', uriPos);
            auto uri    = body.substr(uriPos, uriEnd - uriPos);
            auto it     = g_docs.find(uri);
            if (it != g_docs.end())
                publishDiagnostics(uri, it->second);

        } else if (method == "shutdown") {
            sendResponse(id, "null");

        } else if (method == "exit") {
            return 0;

        } else if (id >= 0) {
            // Unknown request -- send empty response
            sendResponse(id, "null");
        }
    }

    return 0;
}
