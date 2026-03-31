#include "ScriptEditor.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QLabel>
#include <QRegularExpression>

ScriptEditor::ScriptEditor(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Toolbar
    auto* bar = new QHBoxLayout();
    bar->setContentsMargins(6, 3, 6, 3);
    auto* newBtn  = new QPushButton("New");
    auto* openBtn = new QPushButton("Open");
    auto* saveBtn = new QPushButton("Save");
    auto* saveAllBtn = new QPushButton("Save All");
    for (auto* b : {newBtn, openBtn, saveBtn, saveAllBtn})
        b->setFixedHeight(22);
    bar->addWidget(newBtn);
    bar->addWidget(openBtn);
    bar->addWidget(saveBtn);
    bar->addWidget(saveAllBtn);
    bar->addStretch();
    layout->addLayout(bar);

    // Tab widget
    m_tabs = new QTabWidget();
    m_tabs->setTabsClosable(true);
    m_tabs->setMovable(true);
    layout->addWidget(m_tabs);

    connect(m_tabs, &QTabWidget::tabCloseRequested,
            this, &ScriptEditor::onTabCloseRequested);

    connect(newBtn, &QPushButton::clicked, [this] {
        auto* ed = new KonScriptEditor();
        int idx = m_tabs->addTab(ed, "untitled.ks");
        m_fileTabs[idx] = {"", false};
        m_tabs->setCurrentIndex(idx);
        connect(ed, &QPlainTextEdit::textChanged, this, &ScriptEditor::onTextChanged);
    });
    connect(openBtn, &QPushButton::clicked, [this] {
        QString path = QFileDialog::getOpenFileName(this, "Open", m_projectRoot,
            "KonScript (*.ks);;All Files (*)");
        if (!path.isEmpty()) openFile(path);
    });
    connect(saveBtn,    &QPushButton::clicked, this, &ScriptEditor::saveCurrentFile);
    connect(saveAllBtn, &QPushButton::clicked, this, &ScriptEditor::saveAll);
}

void ScriptEditor::setProjectRoot(const QString& root) {
    m_projectRoot = root;
    QString main = root + "/src/main.ks";
    if (QFile::exists(main)) openFile(main);
}

void ScriptEditor::reloadFile(const QString& path) {
    // Find open tab with this path and reload it
    for (auto it = m_fileTabs.begin(); it != m_fileTabs.end(); ++it) {
        if (it->path == path) {
            QFile f(path);
            if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return;
            QString text = QTextStream(&f).readAll();
            auto* ed = qobject_cast<KonScriptEditor*>(m_tabs->widget(it.key()));
            if (!ed) return;
            int cursor = ed->textCursor().position();
            ed->blockSignals(true);
            ed->setPlainText(text);
            ed->document()->setModified(false);
            ed->blockSignals(false);
            // Restore cursor position
            QTextCursor tc = ed->textCursor();
            tc.setPosition(qMin(cursor, text.length()));
            ed->setTextCursor(tc);
            return;
        }
    }
}

void ScriptEditor::openFile(const QString& path) {
    // Check already open
    for (auto it = m_fileTabs.begin(); it != m_fileTabs.end(); ++it) {
        if (it->path == path) { m_tabs->setCurrentIndex(it.key()); return; }
    }
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    auto* ed = new KonScriptEditor();
    ed->setFilePath(path);  // tell editor its file path for --check
    ed->blockSignals(true);
    ed->setPlainText(QTextStream(&f).readAll());
    ed->document()->setModified(false);
    ed->blockSignals(false);
    connect(ed, &QPlainTextEdit::textChanged, this, &ScriptEditor::onTextChanged);
    // Trigger syntax check after load, then re-clear modified flag
    // Clear modified flag after load and run syntax check
    QTimer::singleShot(200, ed, [ed]{
        ed->document()->setModified(false);
        ed->blockSignals(false); // ensure signals work
        ed->runSyntaxCheck();
    });

    QString name = QFileInfo(path).fileName();
    int idx = m_tabs->addTab(ed, name);
    m_fileTabs[idx] = {path, false};
    m_tabs->setCurrentIndex(idx);
}

void ScriptEditor::saveCurrentFile() {
    int idx = m_tabs->currentIndex();
    if (idx < 0) return;
    auto& ft = m_fileTabs[idx];
    if (ft.path.isEmpty()) {
        ft.path = QFileDialog::getSaveFileName(this, "Save As", m_projectRoot,
            "KonScript (*.ks);;All Files (*)");
        if (ft.path.isEmpty()) return;
    }
    QFile f(ft.path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    auto* ed = editorAt(idx);
    QTextStream(&f) << ed->toPlainText();
    ed->document()->setModified(false);
    ft.modified = false;
    updateTabTitle(idx);
}

void ScriptEditor::saveAll() {
    for (int i = 0; i < m_tabs->count(); i++) {
        int prev = m_tabs->currentIndex();
        m_tabs->setCurrentIndex(i);
        saveCurrentFile();
        m_tabs->setCurrentIndex(prev);
    }
}

void ScriptEditor::highlightErrors(const QStringList& errorLines) {
    // Parse "file:line:col: error: msg" from build output
    // and highlight the appropriate editor tab
    QMap<QString, QSet<int>> fileErrors;
    QRegularExpression re(R"(([^:]+):(\d+):\d+: error:)");
    for (auto& line : errorLines) {
        auto m = re.match(line);
        if (m.hasMatch()) {
            QString file = m.captured(1);
            int lineNo   = m.captured(2).toInt();
            fileErrors[file].insert(lineNo);
        }
    }
    for (auto it = m_fileTabs.begin(); it != m_fileTabs.end(); ++it) {
        auto* ed = editorAt(it.key());
        if (!ed) continue;
        QString path = it->path;
        if (fileErrors.contains(path))
            ed->setErrorLines(fileErrors[path]);
        else
            ed->clearErrors();
    }
}

void ScriptEditor::onTabCloseRequested(int index) {
    m_tabs->removeTab(index);
    m_fileTabs.remove(index);
}

void ScriptEditor::onTextChanged() {
    // Find the editor that sent this signal
    auto* ed = qobject_cast<KonScriptEditor*>(sender());
    if (!ed || !ed->document()->isModified()) return;
    for (int i = 0; i < m_tabs->count(); i++) {
        if (editorAt(i) == ed) {
            if (!m_fileTabs[i].modified) {
                m_fileTabs[i].modified = true;
                updateTabTitle(i);
            }
            return;
        }
    }
}

void ScriptEditor::updateTabTitle(int index) {
    auto& ft = m_fileTabs[index];
    QString name = ft.path.isEmpty() ? "untitled.ks" : QFileInfo(ft.path).fileName();
    m_tabs->setTabText(index, ft.modified ? name + " •" : name);
}

KonScriptEditor* ScriptEditor::editorAt(int index) {
    return qobject_cast<KonScriptEditor*>(m_tabs->widget(index));
}
