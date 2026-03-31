#include "ScriptDiffDialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QGroupBox>
#include <QCheckBox>
#include <QPlainTextEdit>
#include <QFile>
#include <QTextStream>
#include <QDialogButtonBox>
#include <QFileInfo>

ScriptDiffDialog::ScriptDiffDialog(const QList<ScriptChange>& changes,
                                    const QString& projectRoot,
                                    QWidget* parent)
    : QDialog(parent), m_changes(changes), m_projectRoot(projectRoot)
{
    setWindowTitle("Script Update Required");
    setMinimumSize(700, 500);
    setStyleSheet("QDialog { background: #1e1e1e; } QLabel { color: #ccc; }"
                  "QPlainTextEdit { background: #111; color: #ddd; border: none; font-family: monospace; font-size: 11px; }"
                  "QGroupBox { color: #888; border: 1px solid #333; margin-top: 8px; padding-top: 4px; }"
                  "QCheckBox { color: #ddd; }");

    auto* layout = new QVBoxLayout(this);

    auto* info = new QLabel(
        QString("KonEditor detected %1 script update(s) needed due to scene changes.\n"
                "Review each change and accept or reject before applying.")
        .arg(changes.size()));
    info->setWordWrap(true);
    info->setStyleSheet("color: #f0a500; font-size: 12px; padding: 4px;");
    layout->addWidget(info);

    auto* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setStyleSheet("QScrollArea { border: none; }");
    auto* container = new QWidget();
    auto* cLayout   = new QVBoxLayout(container);

    for (int i = 0; i < changes.size(); i++) m_accepted.append(true);

    for (int i = 0; i < changes.size(); i++) {
        auto& c = changes[i];
        auto* group = new QGroupBox(QFileInfo(c.scriptPath).fileName() +
                                    "  [" + c.description.split('\n').first() + "]");
        auto* gl = new QVBoxLayout(group);

        auto* desc = new QLabel(c.description);
        desc->setWordWrap(true);
        desc->setStyleSheet("color: #aaa; font-size: 11px;");
        gl->addWidget(desc);

        // Show diff
        auto* diffEdit = new QPlainTextEdit();
        diffEdit->setReadOnly(true);
        diffEdit->setMaximumHeight(140);
        diffEdit->setPlainText(makeDiff(c));
        gl->addWidget(diffEdit);

        // Accept checkbox
        auto* cb = new QCheckBox("Apply this change");
        cb->setChecked(true);
        int idx = i;
        connect(cb, &QCheckBox::toggled, [this, idx](bool v){ m_accepted[idx] = v; });
        gl->addWidget(cb);

        cLayout->addWidget(group);
    }

    scroll->setWidget(container);
    layout->addWidget(scroll);

    auto* btns = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Cancel);
    btns->button(QDialogButtonBox::Apply)->setText("Apply Selected");
    connect(btns->button(QDialogButtonBox::Apply), &QPushButton::clicked,
            this, &QDialog::accept);
    connect(btns->button(QDialogButtonBox::Cancel), &QPushButton::clicked,
            this, &QDialog::reject);
    layout->addWidget(btns);
}

QString ScriptDiffDialog::makeDiff(const ScriptChange& c) {
    if (c.scriptPath.isEmpty()) return "(no file to show)";
    QFile f(c.scriptPath);
    if (!f.open(QIODevice::ReadOnly)) return "(could not open file)";
    QString src = QTextStream(&f).readAll();

    ScriptAnalyzer sa;
    QString after = sa.applyChange(src, c);

    // Simple line diff
    QStringList oldLines = src.split('\n');
    QStringList newLines = after.split('\n');
    QString diff;
    int maxLen = qMax(oldLines.size(), newLines.size());
    for (int i = 0; i < maxLen; i++) {
        QString ol = i < oldLines.size() ? oldLines[i] : "";
        QString nl = i < newLines.size() ? newLines[i] : "";
        if (ol != nl) {
            if (!ol.isEmpty()) diff += "- " + ol + "\n";
            if (!nl.isEmpty()) diff += "+ " + nl + "\n";
        }
    }
    return diff.isEmpty() ? "(no textual changes)" : diff;
}

void ScriptDiffDialog::applyAccepted() {
    ScriptAnalyzer sa;
    for (int i = 0; i < m_changes.size(); i++) {
        if (!m_accepted[i]) continue;
        auto& c = m_changes[i];
        if (c.scriptPath.isEmpty()) continue;
        QFile f(c.scriptPath);
        if (!f.open(QIODevice::ReadOnly)) continue;
        QString src = QTextStream(&f).readAll();
        f.close();
        QString updated = sa.applyChange(src, c);
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
            QTextStream(&f) << updated;
    }
}
