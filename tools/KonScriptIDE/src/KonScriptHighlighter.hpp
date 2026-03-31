#pragma once
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <vector>

// -----------------------------------------------------------------------
// KonScript syntax highlighter
// Covers: keywords, types, builtins, literals, strings, comments, operators
// -----------------------------------------------------------------------
class KonScriptHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    explicit KonScriptHighlighter(QTextDocument* parent = nullptr)
        : QSyntaxHighlighter(parent)
    {
        // ── Colour palette ────────────────────────────────────────────────
        // Dark theme — designed to work with the editor's dark background
        QColor clrKeyword  = QColor(0xC7, 0x8A, 0xFF); // purple
        QColor clrType     = QColor(0x4E, 0xC9, 0xB0); // teal
        QColor clrBuiltin  = QColor(0xDC, 0xDC, 0xAA); // yellow-white
        QColor clrLiteral  = QColor(0xB5, 0xCE, 0xA8); // green (numbers)
        QColor clrString   = QColor(0xCE, 0x91, 0x78); // orange
        QColor clrComment  = QColor(0x6A, 0x99, 0x55); // muted green
        QColor clrOperator = QColor(0xD4, 0xD4, 0xD4); // light grey

        // ── Format helpers ────────────────────────────────────────────────
        auto fmt = [](QColor c, bool bold = false, bool italic = false) {
            QTextCharFormat f;
            f.setForeground(c);
            if (bold)   f.setFontWeight(QFont::Bold);
            if (italic) f.setFontItalic(true);
            return f;
        };

        // ── Rules ─────────────────────────────────────────────────────────

        // Keywords
        QTextCharFormat kwFmt = fmt(clrKeyword, true);
        for (auto& kw : {
            "func", "let", "mut", "const", "return", "if", "else",
            "while", "for", "in", "loop", "break", "continue",
            "pub", "as", "struct", "enum", "class", "node",
            "spawn", "wait", "switch", "include", "import",
            "Some", "None", "null", "true", "false"
        }) {
            addRule("\\b" + QString(kw) + "\\b", kwFmt);
        }

        // Built-in types
        QTextCharFormat typeFmt = fmt(clrType);
        for (auto& t : {
            "I8","I16","I32","I64","U8","U16","U32","U64",
            "F32","F64","Bool","Str","String","Char","Void",
            "Int","Float","Long","UInt",
            // Engine types
            "Node","Node2D","Sprite2D","Collider2D","AnimationPlayer","Camera2D"
        }) {
            addRule("\\b" + QString(t) + "\\b", typeFmt);
        }

        // Built-in functions
        QTextCharFormat builtinFmt = fmt(clrBuiltin);
        for (auto& b : {
            // Core
            "Print","Printf","ToString","Len","Push","Pop","Assert",
            // Window
            "InitWindow","WindowShouldClose","SetTargetFPS","SetVsync",
            "GetWindowWidth","GetWindowHeight","GetScreenWidth","GetScreenHeight",
            "Present","PollEvents","CloseWindow",
            "BeginCamera2D","EndCamera2D","GetWorldMouseX","GetWorldMouseY",
            "DebugMode","IsDebugMode","SetVsync",
            "GetTime","GetDeltaTime","GetFPS",
            // Draw
            "ClearBackground","DrawText","DrawRect","DrawRectangle",
            "DrawCircle","DrawLine","DrawTexture","DrawSprite",
            "DrawTextureEx","DrawRectLines","DrawCircleLines",
            "SetDrawColor","BeginCamera","EndCamera",
            // Textures
            "LoadTexture","UnloadTexture",
            // Input - Keyboard
            "KeyDown","KeyPressed","KeyReleased","IsKeyDown","IsKeyPressed","IsKeyReleased",
            "GetKeyPressed","GetCharPressed",
            // Input - Mouse
            "IsMouseButtonDown","IsMouseButtonPressed","IsMouseButtonReleased",
            "GetMouseX","GetMouseY","GetMousePosition",
            "GetMouseDeltaX","GetMouseDeltaY","GetMouseWheelMove",
            "SetMousePosition","ShowCursor","HideCursor",
            // Input - Gamepad
            "IsGamepadAvailable","IsGamepadButtonDown","IsGamepadButtonPressed",
            "GetGamepadAxisValue",
            // Audio
            "LoadSound","UnloadSound","PlaySound","StopSound","PauseSound",
            "SetSoundVolume","SetSoundPitch",
            "LoadMusic","UnloadMusic","PlayMusic","StopMusic","PauseMusic",
            "ResumeMusic","UpdateMusic","SetMusicVolume","IsMusicPlaying",
            // Camera
            "SetCameraTarget","SetCameraZoom","SetCameraRotation",
            "GetCameraTarget","GetCameraZoom","ScreenToWorld","WorldToScreen",
            // Math
            "Abs","Min","Max","Clamp","Lerp","Floor","Ceil","Round",
            "Sqrt","Sin","Cos","Tan","Atan2","Pow","Rand","RandF",
            // Collision
            "CheckCollisionRecs","CheckCollisionCircles","CheckCollisionPointRec"
        }) {
            addRule("\\b" + QString(b) + "\\b", builtinFmt);
        }

        // Integer literals
        addRule("\\b[0-9]+\\b",        fmt(clrLiteral));
        addRule("\\b0x[0-9A-Fa-f]+\\b", fmt(clrLiteral));

        // Float literals
        addRule("\\b[0-9]+\\.[0-9]*\\b", fmt(clrLiteral));

        // f-string prefix — highlight the 'f' before the quote
        QTextCharFormat fstrPrefixFmt = fmt(QColor(0xFF, 0xAA, 0x44), true); // orange-bold
        addRule("\\bf(?=\")", fstrPrefixFmt);

        // f-string interpolation blocks {expr} — highlight differently inside strings
        QTextCharFormat fstrInterpFmt = fmt(QColor(0x9C, 0xDC, 0xFE)); // blue-ish
        addRule("\\{[^}]+\\}", fstrInterpFmt);

        // Regular string literals (single-line) — after f-string rules so they
        // still match the string body
        addRule("f?\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\"", fmt(clrString));

        // Operators
        addRule("[+\\-*/%=!<>&|^~?:]", fmt(clrOperator));
        addRule("->|\\.\\.=?|::", fmt(clrOperator));

        // Single-line comments
        m_commentFmt = fmt(clrComment, false, true);
        addRule("//[^\n]*", m_commentFmt);
    }

    // Mark specific lines as errors (0-based line numbers)
    void setErrorLines(const QSet<int>& lines) {
        m_errorLines = lines;
        rehighlight();
    }

    void clearErrors() {
        m_errorLines.clear();
        rehighlight();
    }

protected:
    void highlightBlock(const QString& text) override {
        // Apply all syntax rules
        for (auto& rule : m_rules) {
            QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
            while (it.hasNext()) {
                auto m = it.next();
                setFormat(m.capturedStart(), m.capturedLength(), rule.format);
            }
        }

        // Multi-line block comments /* ... */
        setCurrentBlockState(0);
        int startIdx = 0;
        if (previousBlockState() != 1)
            startIdx = text.indexOf("/*");
        while (startIdx >= 0) {
            int endIdx = text.indexOf("*/", startIdx);
            int len;
            if (endIdx == -1) { setCurrentBlockState(1); len = text.length() - startIdx; }
            else              { len = endIdx - startIdx + 2; }
            setFormat(startIdx, len, m_commentFmt);
            startIdx = text.indexOf("/*", startIdx + len);
        }
        // Note: error line background/text is drawn by KonScriptEditor::paintEvent
    }

private:
    struct Rule {
        QRegularExpression pattern;
        QTextCharFormat    format;
    };

    void addRule(const QString& pattern, const QTextCharFormat& fmt) {
        m_rules.push_back({ QRegularExpression(pattern), fmt });
    }

    std::vector<Rule> m_rules;
    QTextCharFormat   m_commentFmt;
    QSet<int>         m_errorLines;
};
