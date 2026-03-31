#include "KonScriptEditor.hpp"
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
        // Builtins
        "Print","Printf","ToString","Len","Push","Pop","Assert",
        "DrawText","DrawRect","DrawCircle","DrawLine",
        "LoadTexture","PlaySound","PlayMusic",
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
            painter.setPen(isCur ? FG_LINE_CUR : FG_LINE_NUM);
            QFont f = painter.font(); f.setBold(isCur); painter.setFont(f);
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
void KonScriptEditor::keyPressEvent(QKeyEvent* event) {
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
