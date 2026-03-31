#pragma once
#include <QWidget>
#include <QPlainTextEdit>
#include <QLineEdit>

class DebugConsole : public QWidget {
    Q_OBJECT
public:
    explicit DebugConsole(QWidget* parent = nullptr);
    void appendOutput(const QString& text);
private:
    QPlainTextEdit* m_output  = nullptr;
    QLineEdit*      m_input   = nullptr;
};
