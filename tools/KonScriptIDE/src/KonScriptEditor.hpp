#pragma once
#include <QPlainTextEdit>
#include <QWidget>
#include <QCompleter>
#include <QStringListModel>
#include "KonScriptHighlighter.hpp"

// Forward declaration — LineNumberArea is defined in KonScriptEditor.cpp
// because it needs KonScriptEditor to be a complete type
class LineNumberArea;

// -----------------------------------------------------------------------
// KonScriptEditor
// -----------------------------------------------------------------------
class KonScriptEditor : public QPlainTextEdit {
    Q_OBJECT
public:
    explicit KonScriptEditor(QWidget* parent = nullptr);

    void lineNumberAreaPaintEvent(QPaintEvent* event);
    int  lineNumberAreaWidth() const;

    KonScriptHighlighter* highlighter() { return m_highlighter; }

    void setErrorLines(const QSet<int>& lines) {
        m_errorLines = lines;
        m_highlighter->setErrorLines(lines);
        viewport()->update();
    }
    void clearErrors() {
        m_errorLines.clear();
        m_highlighter->clearErrors();
        viewport()->update();
    }

    void updateCompleterWords();

signals:
    void fileModified(bool modified);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect& rect, int dy);
    void insertCompletion(const QString& completion);

private:
    void    setupAppearance();
    void    setupCompleter();
    QString wordUnderCursor() const;

    LineNumberArea*       m_lineNumberArea  = nullptr;
    KonScriptHighlighter* m_highlighter     = nullptr;
    QCompleter*           m_completer       = nullptr;
    QStringListModel*     m_completerModel  = nullptr;
    QSet<int>             m_errorLines;
};
