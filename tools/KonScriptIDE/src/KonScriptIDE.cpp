#include "KonScriptIDE.hpp"
#include <QApplication>
#include <QToolBar>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QVBoxLayout>
#include <QStatusBar>
#include <QProcess>
#include <QRegularExpression>
#include <QTextDocument>
#include <QScrollBar>
#include <QShortcut>
#include <QKeySequence>
#include <QListWidgetItem>

// -----------------------------------------------------------------------
// Theme
// -----------------------------------------------------------------------
static const QString STYLE_MAIN = R"(
QMainWindow, QWidget { background:#1e1e1e; color:#d4d4d4; }
QTabWidget::pane { border:none; background:#1e1e1e; }
QTabBar::tab {
    background:#2d2d2d; color:#888;
    padding:5px 16px; border:none; border-right:1px solid #1e1e1e;
    font-size:12px;
}
QTabBar::tab:selected { background:#1e1e1e; color:#d4d4d4; border-top:2px solid #c78aff; }
QTabBar::tab:hover:!selected { background:#333; color:#bbb; }
QToolBar { background:#252526; border-bottom:1px solid #3a3a3a; spacing:4px; padding:2px 6px; }
QToolBar QToolButton {
    background:transparent; color:#ccc; border:none;
    padding:4px 10px; border-radius:3px; font-size:12px;
}
QToolBar QToolButton:hover  { background:#3a3a3a; color:#fff; }
QToolBar QToolButton:pressed{ background:#4a4a4a; }
QDockWidget { color:#d4d4d4; font-size:12px; }
QDockWidget::title { background:#252526; padding:4px 8px; border-bottom:1px solid #3a3a3a; }
QListWidget {
    background:#1e1e1e; color:#d4d4d4; border:none;
    font-family:monospace; font-size:11px;
}
QListWidget::item:selected { background:#2a2d2e; color:#fff; }
QScrollBar:vertical { background:#1e1e1e; width:10px; }
QScrollBar::handle:vertical { background:#424242; border-radius:5px; min-height:20px; }
QScrollBar::handle:vertical:hover { background:#686868; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height:0; }
QScrollBar:horizontal { background:#1e1e1e; height:10px; }
QScrollBar::handle:horizontal { background:#424242; border-radius:5px; min-width:20px; }
QScrollBar::handle:horizontal:hover { background:#686868; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width:0; }
)";

// -----------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------
KonScriptIDE::KonScriptIDE(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("KonScript IDE");
    setMinimumSize(900, 600);
    resize(1200, 750);

    setupUI();
    setupToolbar();
    setupDocks();
    applyTheme();

    // Debounce timer for real-time error checking
    m_checkTimer = new QTimer(this);
    m_checkTimer->setSingleShot(true);
    m_checkTimer->setInterval(600);
    connect(m_checkTimer, &QTimer::timeout, this, &KonScriptIDE::runBackgroundCheck);

    // Keyboard shortcuts
    new QShortcut(QKeySequence::New,    this, this, &KonScriptIDE::newFile);
    new QShortcut(QKeySequence::Open,   this, this, &KonScriptIDE::openFileDialog);
    new QShortcut(QKeySequence::Save,   this, this, [this]{ saveCurrentFile(); });
    new QShortcut(QKeySequence::SaveAs, this, this, [this]{ saveCurrentFileAs(); });
    new QShortcut(Qt::Key_F5,           this, this, &KonScriptIDE::runCurrent);
    new QShortcut(Qt::Key_F6,           this, this, &KonScriptIDE::buildCurrent);
    new QShortcut(Qt::Key_F7,           this, this, &KonScriptIDE::checkCurrent);

    // Start with one blank tab
    newFile();
}

// -----------------------------------------------------------------------
// UI setup
// -----------------------------------------------------------------------
void KonScriptIDE::setupUI() {
    m_tabs = new QTabWidget(this);
    m_tabs->setTabsClosable(true);
    m_tabs->setMovable(true);
    m_tabs->setDocumentMode(true);
    connect(m_tabs, &QTabWidget::tabCloseRequested, this, &KonScriptIDE::closeTab);
    connect(m_tabs, &QTabWidget::currentChanged,    this, &KonScriptIDE::onTabChanged);
    setCentralWidget(m_tabs);
}

void KonScriptIDE::setupToolbar() {
    QToolBar* tb = addToolBar("Main");
    tb->setMovable(false);

    auto btn = [&](const QString& label, auto slot, const QString& tip = "") {
        QAction* a = tb->addAction(label);
        a->setToolTip(tip);
        connect(a, &QAction::triggered, this, slot);
    };

    btn("New",    &KonScriptIDE::newFile,        "Ctrl+N");
    btn("Open",   &KonScriptIDE::openFileDialog, "Ctrl+O");
    btn("Save",   [this]{ saveCurrentFile(); },  "Ctrl+S");
    btn("Save As",[this]{ saveCurrentFileAs(); },"Ctrl+Shift+S");
    tb->addSeparator();

    QAction* run = tb->addAction("▶  Run");
    run->setToolTip("F5"); connect(run, &QAction::triggered, this, &KonScriptIDE::runCurrent);

    QAction* build = tb->addAction("⚙  Build");
    build->setToolTip("F6"); connect(build, &QAction::triggered, this, &KonScriptIDE::buildCurrent);

    QAction* check = tb->addAction("✓  Check");
    check->setToolTip("F7"); connect(check, &QAction::triggered, this, &KonScriptIDE::checkCurrent);
}

void KonScriptIDE::setupDocks() {
    m_outputDock = new QDockWidget("Output", this);
    m_outputDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    m_outputDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);

    m_errorList = new QListWidget;
    m_errorList->setAlternatingRowColors(false);

    // Double-click error → jump to line
    connect(m_errorList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        QRegularExpression re(R"(:(\d+):\d+:)");
        auto m = re.match(item->data(Qt::UserRole).toString());
        if (m.hasMatch()) {
            int line = m.captured(1).toInt() - 1;
            if (auto* ed = currentEditor()) {
                QTextCursor cur = ed->textCursor();
                cur.movePosition(QTextCursor::Start);
                cur.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, line);
                ed->setTextCursor(cur);
                ed->centerCursor();
                ed->setFocus();
            }
        }
    });

    m_outputDock->setWidget(m_errorList);
    addDockWidget(Qt::BottomDockWidgetArea, m_outputDock);
    m_outputDock->setMinimumHeight(120);

    statusBar()->setVisible(true);
    m_statusBar = new QLabel("  KonScript IDE  ");
    m_cursorPos = new QLabel("  Ln 1, Col 1  ");
    statusBar()->addWidget(m_statusBar, 1);
    statusBar()->addPermanentWidget(m_cursorPos);
}

void KonScriptIDE::applyTheme() {
    setStyleSheet(STYLE_MAIN);
    statusBar()->setStyleSheet("background:#007acc; color:#fff; font-size:11px;");
}

// -----------------------------------------------------------------------
// Editor/tab helpers
// -----------------------------------------------------------------------
KonScriptEditor* KonScriptIDE::addEditorTab(const QString& title) {
    auto* ed = new KonScriptEditor;

    // modificationChanged handles dirty flag and check timer
    connect(ed, &QPlainTextEdit::textChanged, this, &KonScriptIDE::onTextModified);
    connect(ed->document(), &QTextDocument::modificationChanged, this, [this, ed](bool modified) {
        if (!m_paths.contains(ed)) return;
        if (modified != isModified(ed)) {
            setModified(ed, modified);
            updateTabTitle(ed);
        }
        if (modified) m_checkTimer->start();
    });
    connect(ed, &QPlainTextEdit::cursorPositionChanged, this, [this, ed] {
        if (currentEditor() != ed) return;
        auto pos = ed->textCursor();
        m_cursorPos->setText(QString("  Ln %1, Col %2  ")
            .arg(pos.blockNumber() + 1).arg(pos.columnNumber() + 1));
    });

    int idx = m_tabs->addTab(ed, title);
    m_tabs->setCurrentIndex(idx);
    m_paths[ed]    = "";
    m_modified[ed] = false;
    ed->setFocus();
    return ed;
}

bool KonScriptIDE::isBlankUntitled(KonScriptEditor* ed) const {
    return ed && pathFor(ed).isEmpty() && ed->toPlainText().trimmed().isEmpty();
}

KonScriptEditor* KonScriptIDE::currentEditor() {
    return editorAt(m_tabs->currentIndex());
}

int KonScriptIDE::findTabForPath(const QString& path) const {
    for (int i = 0; i < m_tabs->count(); i++) {
        auto* ed = editorAt(i);
        if (ed && pathFor(ed) == path) return i;
    }
    return -1;
}

void KonScriptIDE::updateTabTitle(KonScriptEditor* ed) {
    for (int i = 0; i < m_tabs->count(); i++) {
        if (editorAt(i) != ed) continue;
        QString path = pathFor(ed);
        QString name = path.isEmpty()
            ? QString("untitled-%1.ks").arg(i + 1)
            : QFileInfo(path).fileName();
        if (isModified(ed)) name = "● " + name;
        m_tabs->setTabText(i, name);
        break;
    }
}

// -----------------------------------------------------------------------
// Tab operations
// -----------------------------------------------------------------------
void KonScriptIDE::newFile() {
    // If current tab is blank untitled, don't open another one
    auto* cur = currentEditor();
    if (cur && isBlankUntitled(cur)) {
        cur->setFocus();
        return;
    }
    QString name = QString("untitled-%1.ks").arg(++m_untitledCount);
    addEditorTab(name);
}

void KonScriptIDE::openFile(const QString& path) {
    // Already open? Switch to it.
    int existing = findTabForPath(path);
    if (existing >= 0) { m_tabs->setCurrentIndex(existing); return; }

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Cannot open: " + path);
        return;
    }
    QString content = QTextStream(&f).readAll();

    // If the only tab is a blank untitled, reuse it instead of opening a new one
    auto* cur = currentEditor();
    KonScriptEditor* ed = nullptr;
    if (m_tabs->count() == 1 && cur && isBlankUntitled(cur)) {
        ed = cur;
        // Block textChanged signal to avoid triggering dirty flag
        ed->blockSignals(true);
        ed->setPlainText(content);
        ed->blockSignals(false);
    } else {
        ed = addEditorTab(QFileInfo(path).fileName());
        ed->blockSignals(true);
        ed->setPlainText(content);
        ed->blockSignals(false);
    }

    setPath(ed, path);
    setModified(ed, false);
    updateTabTitle(ed);

    // Switch to this tab
    for (int i = 0; i < m_tabs->count(); i++)
        if (editorAt(i) == ed) { m_tabs->setCurrentIndex(i); break; }

    m_statusBar->setText("  " + path);
    emit fileOpened(path);
}

void KonScriptIDE::openFileDialog() {
    QString path = QFileDialog::getOpenFileName(this,
        "Open KonScript File", "",
        "KonScript Files (*.ks);;All Files (*)");
    if (!path.isEmpty()) openFile(path);
}

bool KonScriptIDE::saveCurrentFile() {
    auto* ed = currentEditor();
    if (!ed) return false;

    QString path = pathFor(ed);
    if (path.isEmpty()) return saveCurrentFileAs();

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Cannot save: " + path);
        return false;
    }
    QTextStream(&f) << ed->toPlainText();
    ed->document()->setModified(false); // clears Qt's internal dirty flag
    setModified(ed, false);
    updateTabTitle(ed);
    m_statusBar->setText("  Saved: " + QFileInfo(path).fileName());
    emit fileSaved(path);
    return true;
}

bool KonScriptIDE::saveCurrentFileAs() {
    auto* ed = currentEditor();
    if (!ed) return false;

    QString path = QFileDialog::getSaveFileName(this,
        "Save KonScript File", "",
        "KonScript Files (*.ks);;All Files (*)");
    if (path.isEmpty()) return false;
    if (!path.endsWith(".ks")) path += ".ks";

    setPath(ed, path);
    updateTabTitle(ed);
    return saveCurrentFile();
}

void KonScriptIDE::closeTab(int index) {
    auto* ed = editorAt(index);
    if (!ed) return;

    if (isModified(ed)) {
        int ret = QMessageBox::question(this, "Unsaved Changes",
            QString("Save changes to %1?").arg(m_tabs->tabText(index)),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (ret == QMessageBox::Save) {
            m_tabs->setCurrentIndex(index);
            if (!saveCurrentFile()) return;
        } else if (ret == QMessageBox::Cancel) {
            return;
        }
    }

    m_paths.remove(ed);
    m_modified.remove(ed);
    m_tabs->removeTab(index);

    if (m_tabs->count() == 0) newFile();
}

// -----------------------------------------------------------------------
// Slot: tab changed
// -----------------------------------------------------------------------
void KonScriptIDE::onTabChanged(int index) {
    auto* ed = editorAt(index);
    if (!ed) return;
    QString path = pathFor(ed);
    m_statusBar->setText(path.isEmpty() ? "  New file" : "  " + path);
}

// -----------------------------------------------------------------------
// Slot: text modified — modificationChanged handles dirty flag
// This slot is kept for the check timer only
// -----------------------------------------------------------------------
void KonScriptIDE::onTextModified() {
    m_checkTimer->start();
}

// -----------------------------------------------------------------------
// Real-time background error check
// -----------------------------------------------------------------------
void KonScriptIDE::runBackgroundCheck() {
    auto* ed = currentEditor();
    if (!ed || ed->toPlainText().trimmed().isEmpty()) {
        if (ed) ed->clearErrors();
        return;
    }

    // Write to a temp file
    QString tempPath = QDir::tempPath() + "/konscript_check_tmp.ks";
    {
        QFile f(tempPath);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;
        QTextStream(&f) << ed->toPlainText();
    }

    // Kill any previous check
    if (m_checkProcess && m_checkProcess->state() != QProcess::NotRunning) {
        m_checkProcess->kill();
        m_checkProcess->waitForFinished(300);
    }
    delete m_checkProcess;
    m_checkProcess = new QProcess(this);

    connect(m_checkProcess,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, ed, tempPath](int exitCode, QProcess::ExitStatus) {
        // Guard: editor might have been closed
        if (!m_paths.contains(ed)) return;

        QString output = m_checkProcess->readAllStandardError()
                       + m_checkProcess->readAllStandardOutput();

        if (exitCode == 0) {
            ed->clearErrors();
            m_statusBar->setText("  ✓ No errors");
            // Clear error list entries from the background check
            for (int i = m_errorList->count() - 1; i >= 0; i--) {
                auto* item = m_errorList->item(i);
                if (item->data(Qt::UserRole + 1).toBool())
                    delete m_errorList->takeItem(i);
            }
        } else {
            QSet<int> errorLines;
            // KonScript format: "somefile.ks:line:col: message"
            // Anchor to .ks extension to avoid false matches in error messages
            QRegularExpression re(R"([\w./\\-]+\.ks:(\d+):(\d+):\s*(.+))",
                                  QRegularExpression::MultilineOption);

            // Clear old background-check errors from the list
            for (int i = m_errorList->count() - 1; i >= 0; i--) {
                auto* item = m_errorList->item(i);
                if (item->data(Qt::UserRole + 1).toBool())
                    delete m_errorList->takeItem(i);
            }

            auto it = re.globalMatch(output);
            while (it.hasNext()) {
                auto m = it.next();
                int lineNum = m.captured(1).toInt() - 1;
                errorLines.insert(lineNum);

                QString msg = m.captured(3).trimmed();
                auto* item = new QListWidgetItem(
                    QString("✗  Ln %1  %2")
                        .arg(m.captured(1))
                        .arg(msg));
                item->setForeground(QColor(0xF4, 0x47, 0x47));
                item->setData(Qt::UserRole,     output);
                item->setData(Qt::UserRole + 1, true);
                m_errorList->addItem(item);
            }

            ed->setErrorLines(errorLines);
            m_statusBar->setText(QString("  ✗ %1 error(s)").arg(errorLines.size()));
            m_outputDock->show();
        }

        QFile::remove(tempPath);
    });

    m_checkProcess->start("konscript", {"--check", tempPath});
}

// -----------------------------------------------------------------------
// Run / Build / Check (explicit)
// -----------------------------------------------------------------------
void KonScriptIDE::runCurrent() {
    if (!saveCurrentFile()) return;
    auto* ed = currentEditor();
    if (!ed) return;
    QString path = pathFor(ed);
    if (path.isEmpty()) return;

    clearOutput();
    appendOutput("▶ Running: " + path);

    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill(); m_process->waitForFinished(1000);
    }
    delete m_process;
    m_process = new QProcess(this);
    m_process->setWorkingDirectory(QFileInfo(path).absolutePath());
    connect(m_process, &QProcess::readyReadStandardOutput, this, &KonScriptIDE::onProcessOutput);
    connect(m_process, &QProcess::readyReadStandardError,  this, &KonScriptIDE::onProcessError);
    connect(m_process, QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
            this, &KonScriptIDE::onProcessFinished);

    m_process->start("konscript", { path });
    m_outputDock->show();
    m_statusBar->setText("  Building...");
}

void KonScriptIDE::buildCurrent() {
    if (!saveCurrentFile()) return;
    auto* ed = currentEditor();
    if (!ed) return;
    QString path = pathFor(ed);
    if (path.isEmpty()) return;

    clearOutput();
    appendOutput("⚙ Building: " + path);

    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill(); m_process->waitForFinished(1000);
    }
    delete m_process;
    m_process = new QProcess(this);
    m_process->setWorkingDirectory(QFileInfo(path).absolutePath());
    connect(m_process, &QProcess::readyReadStandardOutput, this, &KonScriptIDE::onProcessOutput);
    connect(m_process, &QProcess::readyReadStandardError,  this, &KonScriptIDE::onProcessError);
    connect(m_process, QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
            this, &KonScriptIDE::onProcessFinished);

    QStringList args = { path };
    if (m_buildTarget != "linux64") args << "--target" << m_buildTarget;
    m_process->start("konscript", args);
    m_outputDock->show();
    m_statusBar->setText("  Building...");
}

void KonScriptIDE::checkCurrent() {
    if (!saveCurrentFile()) return;
    auto* ed = currentEditor();
    if (!ed) return;
    QString path = pathFor(ed);
    if (path.isEmpty()) return;

    clearOutput();
    appendOutput("✓ Checking: " + path);

    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill(); m_process->waitForFinished(1000);
    }
    delete m_process;
    m_process = new QProcess(this);
    m_process->setWorkingDirectory(QFileInfo(path).absolutePath());
    connect(m_process, &QProcess::readyReadStandardOutput, this, &KonScriptIDE::onProcessOutput);
    connect(m_process, &QProcess::readyReadStandardError,  this, &KonScriptIDE::onProcessError);
    connect(m_process, QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
            this, &KonScriptIDE::onProcessFinished);

    m_process->start("konscript", {"--check", path});
    m_outputDock->show();
    m_statusBar->setText("  Checking...");
}

// -----------------------------------------------------------------------
// Process output
// -----------------------------------------------------------------------
void KonScriptIDE::onProcessOutput() {
    if (!m_process) return;
    for (auto& line : QString(m_process->readAllStandardOutput()).split('\n'))
        if (!line.trimmed().isEmpty()) appendOutput(line);
}

void KonScriptIDE::onProcessError() {
    if (!m_process) return;
    parseErrorOutput(m_process->readAllStandardError());
}

void KonScriptIDE::onProcessFinished(int exitCode, QProcess::ExitStatus) {
    if (exitCode == 0) {
        appendOutput("✓ Done.");
        m_statusBar->setText("  Build OK");

        // Run the built binary
        auto* ed = currentEditor();
        if (!ed) return;
        QString path = pathFor(ed);
        if (path.isEmpty()) return;

        QString outBin = QFileInfo(path).absolutePath() + "/" + QFileInfo(path).baseName();
#ifdef _WIN32
        outBin += ".exe";
#endif
        if (QFile::exists(outBin)) {
            appendOutput("▶ Running: " + outBin);
            QProcess* run = new QProcess(this);
            run->setWorkingDirectory(QFileInfo(outBin).absolutePath());
            connect(run, &QProcess::readyReadStandardOutput, this, [this, run]{
                appendOutput(run->readAllStandardOutput().trimmed());
            });
            connect(run, &QProcess::readyReadStandardError, this, [this, run]{
                appendOutput(run->readAllStandardError().trimmed(), true);
            });
            connect(run, QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
                    run, &QProcess::deleteLater);
            run->start(outBin, QStringList{});
        }
    } else {
        appendOutput(QString("✗ Failed (exit %1)").arg(exitCode), true);
        m_statusBar->setText("  Build failed");
    }
}

void KonScriptIDE::parseErrorOutput(const QString& output) {
    // KonScript format: "somefile.ks:line:col: message"
    QRegularExpression re(R"([\w./\\-]+\.ks:(\d+):(\d+):\s*(.+))",
                          QRegularExpression::MultilineOption);
    QSet<int> errorLines;

    auto it = re.globalMatch(output);
    while (it.hasNext()) {
        auto m = it.next();
        int lineNum = m.captured(1).toInt() - 1;
        errorLines.insert(lineNum);

        QString msg = m.captured(3).trimmed();
        auto* item = new QListWidgetItem(
            QString("✗  Ln %1, Col %2  %3")
                .arg(m.captured(1)).arg(m.captured(2))
                .arg(msg));
        item->setForeground(QColor(0xF4, 0x47, 0x47));
        item->setData(Qt::UserRole,     output);
        item->setData(Qt::UserRole + 1, false);
        m_errorList->addItem(item);
        m_errorList->scrollToBottom();
    }

    if (!errorLines.isEmpty())
        if (auto* ed = currentEditor())
            ed->setErrorLines(errorLines);
}

void KonScriptIDE::appendOutput(const QString& text, bool isError) {
    auto* item = new QListWidgetItem(text.trimmed());
    if (text.trimmed().isEmpty()) return;
    item->setForeground(isError ? QColor(0xF4, 0x47, 0x47) : QColor(0xD4, 0xD4, 0xD4));
    item->setData(Qt::UserRole + 1, false);
    m_errorList->addItem(item);
    m_errorList->scrollToBottom();
}

void KonScriptIDE::clearOutput() {
    m_errorList->clear();
    if (auto* ed = currentEditor())
        ed->clearErrors();
}
