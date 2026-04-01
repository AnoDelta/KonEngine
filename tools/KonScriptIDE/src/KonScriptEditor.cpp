#include "KonScriptEditor.hpp"
#include <QSettings>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QTextCursor>
#include <QTextBlock>
#include <QPainter>
#include <QFontMetrics>
#include <QScrollBar>
#include <QAbstractItemView>
#include <QApplication>
#include <QRegularExpression>

// -----------------------------------------------------------------------
// LineNumberArea
// Defined here (not in the header) so KonScriptEditor is a complete type
// -----------------------------------------------------------------------
class LineNumberArea : public QWidget {
public:
    explicit LineNumberArea(KonScriptEditor* editor)
        : QWidget(editor), m_editor(editor) {}

    QSize sizeHint() const override {
        return QSize(m_editor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        m_editor->lineNumberAreaPaintEvent(event);
    }

private:
    KonScriptEditor* m_editor;
};

// -----------------------------------------------------------------------
// Colours
// -----------------------------------------------------------------------
static const QColor BG_EDITOR    = QColor(0x1E, 0x1E, 0x1E);
static const QColor BG_LINE_NUM  = QColor(0x25, 0x25, 0x25);
static const QColor FG_LINE_NUM  = QColor(0x85, 0x85, 0x85);
static const QColor FG_LINE_CUR  = QColor(0xC8, 0xC8, 0xC8);
static const QColor BG_CUR_LINE  = QColor(0x2A, 0x2A, 0x2A);
static const QColor FG_TEXT      = QColor(0xD4, 0xD4, 0xD4);
static const QColor COLOR_BORDER = QColor(0x3A, 0x3A, 0x3A);

// -----------------------------------------------------------------------
// Built-in word list
// -----------------------------------------------------------------------
static QStringList builtinWords() {
    return {
        // Keywords
        "func","let","mut","const","return","if","else",
        "while","for","in","loop","break","continue",
        "pub","as","struct","enum","class","node",
        "spawn","wait","switch","include","true","false","null","None","Some",
        // Types
        "I8","I16","I32","I64","U8","U16","U32","U64",
        "F32","F64","Bool","Str","String","Char","Void","Int","Float",
        "Node","Node2D","Sprite2D","Collider2D","AnimationPlayer","Camera2D",
        // Core builtins
        "Print","Printf","ToString","Len","Push","Pop","Assert",
        // Window
        "InitWindow","WindowShouldClose","SetTargetFPS","SetVSync",
        "GetScreenWidth","GetScreenHeight","SetWindowTitle","SetWindowSize",
        "Present","PollEvents","CloseWindow","GetTime","GetDeltaTime","GetFPS",
        // Renderer / Draw
        "ClearBackground","DrawText","DrawRect","DrawRectangle",
        "DrawCircle","DrawLine","DrawTexture","DrawSprite",
        "DrawTextureEx","DrawRectLines","DrawCircleLines",
        "SetDrawColor","BeginCamera","EndCamera",
        // Textures
        "LoadTexture","UnloadTexture","GetTextureWidth","GetTextureHeight",
        // Input — Keyboard
        "IsKeyDown","IsKeyPressed","IsKeyReleased","IsKeyUp",
        "GetKeyPressed","GetCharPressed",
        // Input — Mouse
        "IsMouseButtonDown","IsMouseButtonPressed","IsMouseButtonReleased",
        "GetMouseX","GetMouseY","GetMousePosition",
        "GetMouseDeltaX","GetMouseDeltaY","GetMouseWheelMove",
        "SetMousePosition","ShowCursor","HideCursor",
        // Input — Gamepad
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
        "CheckCollisionRecs","CheckCollisionCircles","CheckCollisionPointRec",
        // Nodes / Scene
        "AddNode","RemoveNode","GetNode","SetNodePosition","GetNodePosition",
        "SetNodeRotation","GetNodeRotation","SetNodeScale","GetNodeScale",
    };
}

// -----------------------------------------------------------------------
// KonScriptEditor
// -----------------------------------------------------------------------
KonScriptEditor::KonScriptEditor(QWidget* parent)
    : QPlainTextEdit(parent)
{
    m_lineNumberArea = new LineNumberArea(this);
    m_highlighter    = new KonScriptHighlighter(document());

    setupAppearance();
    setupCompleter();

    connect(this, &QPlainTextEdit::blockCountChanged,
            this, &KonScriptEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest,
            this, &KonScriptEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged,
            this, &KonScriptEditor::highlightCurrentLine);
    connect(this, &QPlainTextEdit::textChanged,
            this, &KonScriptEditor::updateCompleterWords);

    // Real-time syntax check — debounced 800ms after typing stops
    m_checkTimer = new QTimer(this);
    m_checkTimer->setSingleShot(true);
    m_checkTimer->setInterval(800);
    connect(this, &QPlainTextEdit::textChanged, [this]{
        m_checkTimer->start();
    });
    connect(m_checkTimer, &QTimer::timeout, this, &KonScriptEditor::runSyntaxCheck);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

// -----------------------------------------------------------------------
// Appearance
// -----------------------------------------------------------------------
void KonScriptEditor::setupAppearance() {
    // Pick first available monospace font
    QFont font("Fira Code");
    if (!QFontInfo(font).fixedPitch()) font = QFont("Cascadia Code");
    if (!QFontInfo(font).fixedPitch()) font = QFont("Consolas");
    if (!QFontInfo(font).fixedPitch()) font = QFont("DejaVu Sans Mono");
    if (!QFontInfo(font).fixedPitch()) font = QFont("Liberation Mono");
    if (!QFontInfo(font).fixedPitch()) font.setStyleHint(QFont::Monospace);
    font.setPointSize(11);
    font.setFixedPitch(true);
    setFont(font);

    setTabStopDistance(QFontMetrics(font).horizontalAdvance(' ') * 4);

    QPalette p = palette();
    p.setColor(QPalette::Base, BG_EDITOR);
    p.setColor(QPalette::Text, FG_TEXT);
    setPalette(p);

    setLineWrapMode(QPlainTextEdit::NoWrap);
    setFrameStyle(QFrame::NoFrame);
}

// -----------------------------------------------------------------------
// Completer
// -----------------------------------------------------------------------
void KonScriptEditor::setupCompleter() {
    m_completerModel = new QStringListModel(builtinWords(), this);
    m_completer      = new QCompleter(m_completerModel, this);
    m_completer->setWidget(this);
    m_completer->setCompletionMode(QCompleter::PopupCompletion);
    m_completer->setCaseSensitivity(Qt::CaseSensitive);
    m_completer->setMaxVisibleItems(12);

    auto* popup = m_completer->popup();
    popup->setStyleSheet(R"(
        QListView {
            background: #252526;
            color: #d4d4d4;
            border: 1px solid #555;
            font-family: monospace;
            font-size: 13px;
            padding: 2px;
            selection-background-color: #094771;
            selection-color: #ffffff;
            outline: none;
            min-width: 220px;
        }
        QListView::item {
            padding: 3px 8px;
            min-height: 22px;
        }
        QListView::item:hover {
            background: #2a2d2e;
        }
        QListView::item:selected {
            background: #094771;
            color: #fff;
        }
        QScrollBar:vertical { background:#252526; width:8px; }
        QScrollBar::handle:vertical { background:#424242; border-radius:4px; min-height:20px; }
        QScrollBar::handle:vertical:hover { background:#686868; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height:0; }
    )");
    popup->setMouseTracking(true);

    connect(m_completer,
            QOverload<const QString&>::of(&QCompleter::activated),
            this, &KonScriptEditor::insertCompletion);
}

void KonScriptEditor::updateCompleterWords() {
    // Scan document for user-defined func/let/const names
    QString text = toPlainText();
    QRegularExpression re(R"(\b(?:func|let|mut|const)\s+([A-Za-z_][A-Za-z0-9_]*))");
    QSet<QString> userWords;
    auto it = re.globalMatch(text);
    while (it.hasNext())
        userWords.insert(it.next().captured(1));

    QStringList all = builtinWords();
    for (auto& w : userWords)
        if (!all.contains(w)) all.append(w);
    all.sort(Qt::CaseSensitive);
    all.removeDuplicates();
    m_completerModel->setStringList(all);
}

QString KonScriptEditor::wordUnderCursor() const {
    QTextCursor tc = textCursor();
    tc.select(QTextCursor::WordUnderCursor);
    return tc.selectedText();
}

void KonScriptEditor::insertCompletion(const QString& completion) {
    if (m_completer->widget() != this) return;
    QTextCursor tc = textCursor();
    int extra = completion.length() - m_completer->completionPrefix().length();
    tc.movePosition(QTextCursor::Left);
    tc.movePosition(QTextCursor::EndOfWord);
    tc.insertText(completion.right(extra));
    setTextCursor(tc);
}

// -----------------------------------------------------------------------
// Line number area
// -----------------------------------------------------------------------
int KonScriptEditor::lineNumberAreaWidth() const {
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) { max /= 10; ++digits; }
    return 6 + fontMetrics().horizontalAdvance('9') * qMax(digits, 3) + 10;
}

void KonScriptEditor::updateLineNumberAreaWidth(int) {
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void KonScriptEditor::updateLineNumberArea(const QRect& rect, int dy) {
    if (dy) m_lineNumberArea->scroll(0, dy);
    else    m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());
    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void KonScriptEditor::resizeEvent(QResizeEvent* event) {
    QPlainTextEdit::resizeEvent(event);
    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(),
                                        lineNumberAreaWidth(), cr.height()));
}

void KonScriptEditor::lineNumberAreaPaintEvent(QPaintEvent* event) {
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), BG_LINE_NUM);

    painter.setPen(COLOR_BORDER);
    painter.drawLine(m_lineNumberArea->width() - 1, event->rect().top(),
                     m_lineNumberArea->width() - 1, event->rect().bottom());

    QTextBlock block  = firstVisibleBlock();
    int blockNum      = block.blockNumber();
    int top    = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());
    int curLine = textCursor().blockNumber();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            bool isCur = (blockNum == curLine);
            bool isErr = m_errorLines.contains(blockNum);
            // Error lines: red background in gutter
            if (isErr) {
                painter.fillRect(0, top, m_lineNumberArea->width(), fontMetrics().height(),
                                 QColor(0x6B, 0x00, 0x00));
            }
            painter.setPen(isErr ? QColor(0xFF, 0x55, 0x55)
                         : isCur ? FG_LINE_CUR : FG_LINE_NUM);
            QFont f = painter.font();
            f.setBold(isCur || isErr);
            painter.setFont(f);
            painter.drawText(0, top, m_lineNumberArea->width() - 8,
                             fontMetrics().height(), Qt::AlignRight,
                             QString::number(blockNum + 1));
        }
        block  = block.next();
        top    = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNum;
    }
}

// -----------------------------------------------------------------------
// Current line highlight
// -----------------------------------------------------------------------
void KonScriptEditor::highlightCurrentLine() {
    QList<QTextEdit::ExtraSelection> extras;
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection sel;
        sel.format.setBackground(BG_CUR_LINE);
        sel.format.setProperty(QTextFormat::FullWidthSelection, true);
        sel.cursor = textCursor();
        sel.cursor.clearSelection();
        extras.append(sel);
    }
    setExtraSelections(extras);
}

// -----------------------------------------------------------------------
// Key press — autocomplete, auto-indent, tab expansion
// -----------------------------------------------------------------------
void KonScriptEditor::zoomEditor(int delta) {
    QFont f = font();
    int size = f.pointSize() + delta;
    if (size < 6)  size = 6;
    if (size > 72) size = 72;
    f.setPointSize(size);
    setFont(f);
    setTabStopDistance(QFontMetrics(f).horizontalAdvance(' ') * 4);
    QSettings("AnoDelta", "KonEditor").setValue("editorFontSize", size);
}

void KonScriptEditor::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        int delta = event->angleDelta().y() > 0 ? 1 : -1;
        zoomEditor(delta);
        event->accept();
        return;
    }
    QPlainTextEdit::wheelEvent(event);
}

void KonScriptEditor::keyPressEvent(QKeyEvent* event) {
    // Ctrl+= or Ctrl++ to zoom in, Ctrl+- to zoom out
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->key() == Qt::Key_Equal || event->key() == Qt::Key_Plus) {
            zoomEditor(1); event->accept(); return;
        }
        if (event->key() == Qt::Key_Minus) {
            zoomEditor(-1); event->accept(); return;
        }
        if (event->key() == Qt::Key_0) {
            // Reset to default size
            QFont f = font(); f.setPointSize(11); setFont(f);
            setTabStopDistance(QFontMetrics(f).horizontalAdvance(' ') * 4);
            event->accept(); return;
        }
    }
    // Let completer handle its navigation keys
    if (m_completer && m_completer->popup()->isVisible()) {
        switch (event->key()) {
        case Qt::Key_Tab:
            if (m_completer->completionCount() > 0) {
                insertCompletion(m_completer->currentCompletion());
                m_completer->popup()->hide();
            }
            return;
        case Qt::Key_Enter:
        case Qt::Key_Return:
            if (m_completer->completionCount() > 0) {
                insertCompletion(m_completer->currentCompletion());
                m_completer->popup()->hide();
            }
            return;
        case Qt::Key_Escape:
            m_completer->popup()->hide();
            return;
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_PageUp:
        case Qt::Key_PageDown:
            // Forward directly to the popup — do NOT call ignore()
            QApplication::sendEvent(m_completer->popup(), event);
            return;
        default:
            break;
        }
    }

    bool forceComplete = (event->modifiers() == Qt::ControlModifier
                          && event->key() == Qt::Key_Space);

    // Tab → 4 spaces (when popup not visible)
    if (event->key() == Qt::Key_Tab) {
        textCursor().insertText("    ");
        return;
    }

    // Enter → auto-indent + auto-close braces
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        QTextCursor cur = textCursor();
        QString line    = cur.block().text();
        QString indent;
        for (QChar c : line) {
            if (c == ' ') indent += ' ';
            else if (c == '\t') indent += '\t';
            else break;
        }
        QString trimmed = line.trimmed();
        bool openBrace  = trimmed.endsWith('{');
        if (openBrace) indent += "    ";

        QPlainTextEdit::keyPressEvent(event);
        textCursor().insertText(indent);

        if (openBrace) {
            QTextCursor close = textCursor();
            QString dedent    = indent.left(indent.length() - 4);
            close.insertText("\n" + dedent + "}");
            close.movePosition(QTextCursor::Up);
            close.movePosition(QTextCursor::EndOfLine);
            setTextCursor(close);
        }
        return;
    }

    QPlainTextEdit::keyPressEvent(event);

    // ── Trigger completer ─────────────────────────────────────────────────
    // Don't trigger for modifier-only keys
    if (!forceComplete) {
        bool mod = event->modifiers() &
                   (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier);
        if (mod) { m_completer->popup()->hide(); return; }
    }

    QString word = wordUnderCursor();
    if (!forceComplete && word.length() < 2) {
        m_completer->popup()->hide();
        return;
    }

    m_completer->setCompletionPrefix(word);
    if (m_completer->completionCount() == 0) {
        m_completer->popup()->hide();
        return;
    }
    m_completer->popup()->setCurrentIndex(
        m_completer->completionModel()->index(0, 0));

    QRect cr = cursorRect();
    int popupWidth = qMax(280,
        m_completer->popup()->sizeHintForColumn(0)
        + m_completer->popup()->verticalScrollBar()->sizeHint().width() + 20);
    cr.setWidth(popupWidth);
    m_completer->complete(cr);
}

void KonScriptEditor::setFilePath(const QString& path) { m_filePath = path; }

void KonScriptEditor::runSyntaxCheck() {
    QString text = toPlainText().trimmed();
    // Skip JSON
    if (text.startsWith("{") || text.startsWith("[")) {
        clearErrors();
        return;
    }
    // Skip scene files — they need recursive include context
    // Detect by: has this.add() calls (scene pattern) or is in scenes/ folder
    if (m_filePath.contains("/scenes/") || text.contains("this.add(")) {

        clearErrors();
        return;
    }

    // Kill any running check
    if (m_checkProcess) {
        m_checkProcess->disconnect();
        m_checkProcess->kill();
        m_checkProcess->waitForFinished(500);
        delete m_checkProcess;
        m_checkProcess = nullptr;
    }

    // Write current content to a temp file
    m_checkTmpPath = QDir::temp().filePath(
        QString("_koncheck_%1.ks").arg(quintptr(this)));
    QFile tmp(m_checkTmpPath);
    if (!tmp.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream(&tmp) << toPlainText();
    tmp.close();

    m_checkProcess = new QProcess(this);
    m_checkProcess->setProcessChannelMode(QProcess::MergedChannels);

    // Use finished signal with context = this to auto-disconnect if editor dies
    connect(m_checkProcess,
            static_cast<void(QProcess::*)(int,QProcess::ExitStatus)>(&QProcess::finished),
            this, &KonScriptEditor::onCheckFinished);

    // Check the real file if available (so includes resolve)
    // Fall back to temp file for unsaved content
    QString checkTarget = m_filePath.isEmpty() ? m_checkTmpPath : m_filePath;
    m_checkProcess->start("konscript", {"--check", checkTarget});
    if (!m_checkProcess->waitForStarted(1000)) {
        delete m_checkProcess;
        m_checkProcess = nullptr;
        QFile::remove(m_checkTmpPath);
    }
}

void KonScriptEditor::onCheckFinished(int, QProcess::ExitStatus) {
    if (!m_checkProcess) return;
    QString out = m_checkProcess->readAllStandardOutput();
    QSet<int> errLines;
    QRegularExpression re(R"([^:
]+:(\d+):\d+: error:)");
    auto it = re.globalMatch(out);
    while (it.hasNext())
        errLines.insert(it.next().captured(1).toInt() - 1); // 0-based
    setErrorLines(errLines);
    QFile::remove(m_checkTmpPath);
    m_checkProcess->deleteLater();
    m_checkProcess = nullptr;
}

KonScriptEditor::~KonScriptEditor() {
    if (m_checkTimer) m_checkTimer->stop();
    if (m_checkProcess) {
        m_checkProcess->disconnect();
        m_checkProcess->kill();
        m_checkProcess->waitForFinished(500);
        delete m_checkProcess;
        m_checkProcess = nullptr;
    }
    QFile::remove(m_checkTmpPath);
}

void KonScriptEditor::focusOutEvent(QFocusEvent* event) {
    if (m_completer) m_completer->popup()->hide();
    QPlainTextEdit::focusOutEvent(event);
}

// -----------------------------------------------------------------------
// Paint event — draws error line background + "(SYNTAX ERROR)" label
// -----------------------------------------------------------------------
void KonScriptEditor::paintEvent(QPaintEvent* event) {
    // Draw error backgrounds BEFORE base class so text renders on top
    if (!m_errorLines.isEmpty()) {
        QPainter painter(viewport());

        static const QColor ERR_BG   = QColor(0x4B, 0x00, 0x00, 120); // semi-transparent dark red
        static const QColor ERR_LINE = QColor(0xFF, 0x33, 0x33);       // solid left border
        static const QColor ERR_FG   = QColor(0xFF, 0x55, 0x55, 180);  // faded label

        QFont errFont = font();
        errFont.setPointSize(errFont.pointSize() - 1);
        errFont.setBold(true);
        errFont.setItalic(true);
        painter.setFont(errFont);

        QTextBlock block = firstVisibleBlock();
        while (block.isValid()) {
            int lineNum = block.blockNumber();
            QRectF blockRect = blockBoundingGeometry(block).translated(contentOffset());
            if (blockRect.top() > viewport()->height()) break;

            if (m_errorLines.contains(lineNum)) {
                // Subtle tinted background — text will paint over this
                QRectF bg(0, blockRect.top(), viewport()->width(), blockRect.height());
                painter.fillRect(bg, ERR_BG);

                // 3px left border
                painter.fillRect(QRectF(0, blockRect.top(), 3, blockRect.height()), ERR_LINE);

                // Right-aligned label — only in the right 180px, so it doesn't cover short lines
                QRectF labelRect(viewport()->width() - 175, blockRect.top(),
                                 170, blockRect.height());
                painter.setPen(ERR_FG);
                painter.drawText(labelRect, Qt::AlignVCenter | Qt::AlignRight, "(SYNTAX ERROR)");
            }
            block = block.next();
        }
    }

    // Base class paints text on top of our background
    QPlainTextEdit::paintEvent(event);
}
