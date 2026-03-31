#include "DebugConsole.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QScrollBar>

DebugConsole::DebugConsole(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    auto* bar = new QHBoxLayout();
    bar->addStretch();
    auto* clearBtn = new QPushButton("Clear");
    clearBtn->setFixedHeight(22);
    clearBtn->setFixedWidth(60);
    connect(clearBtn, &QPushButton::clicked, [this]{ m_output->clear(); });
    bar->addWidget(clearBtn);
    layout->addLayout(bar);

    m_output = new QPlainTextEdit();
    m_output->setReadOnly(true);
    m_output->setFont(QFont("Monospace", 9));
    m_output->setStyleSheet("background: #0d0d0d; color: #ccc; border: none;");
    m_output->setMaximumBlockCount(5000);
    layout->addWidget(m_output);
}

void DebugConsole::appendOutput(const QString& text) {
    m_output->appendPlainText(text.trimmed());
    m_output->verticalScrollBar()->setValue(m_output->verticalScrollBar()->maximum());
}
