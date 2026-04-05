#include "BuildPanel.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDir>
#include <QScrollBar>
#include <QDateTime>
#include <QCoreApplication>
#include <QMenu>
#include <QClipboard>
#include <QApplication>
#include <QProgressBar>
#include <QTimer>
#include <QRegularExpression>
#include <QFile>

BuildPanel::BuildPanel(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    auto* bar = new QHBoxLayout();
    m_status = new QLabel("Ready");
    m_status->setStyleSheet("color: #aaa; font-size: 11px;");
    bar->addWidget(m_status);
    bar->addStretch();
    auto* clearBtn = new QPushButton("Clear");
    clearBtn->setFixedHeight(22);
    clearBtn->setFixedWidth(60);
    bar->addWidget(clearBtn);

    m_cancelBtn = new QPushButton("Cancel");
    m_cancelBtn->setFixedHeight(22);
    m_cancelBtn->setFixedWidth(60);
    m_cancelBtn->setEnabled(false);
    m_cancelBtn->setStyleSheet("color: #f44336;");
    connect(m_cancelBtn, &QPushButton::clicked, [this]{
        if (m_proc && m_proc->state() != QProcess::NotRunning) {
            m_proc->kill();
            appendLog("\n⚠ Build cancelled.");
            m_status->setText("Cancelled");
            m_status->setStyleSheet("color: #f0a500; font-size: 11px;");
        }
    });
    bar->addWidget(m_cancelBtn);
    layout->addLayout(bar);

    m_progress = new QProgressBar();
    m_progress->setRange(0, 0); // indeterminate by default
    m_progress->setFixedHeight(3);
    m_progress->setTextVisible(false);
    m_progress->setStyleSheet(
        "QProgressBar { background: #1a1a1a; border: none; }"
        "QProgressBar::chunk { background: #0078d7; }");
    m_progress->hide();
    layout->addWidget(m_progress);

    // m_log must be created BEFORE connecting signals that reference it
    m_log = new QPlainTextEdit();
    m_log->setReadOnly(true);
    m_log->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    m_log->setContextMenuPolicy(Qt::CustomContextMenu);
    m_log->setFont(QFont("Monospace", 9));
    m_log->setStyleSheet("background: #111; color: #ccc; border: none;");
    m_log->setMaximumBlockCount(5000);
    layout->addWidget(m_log);

    // Wire up signals now that m_log exists
    connect(clearBtn, &QPushButton::clicked, [this]{ m_log->clear(); });
    connect(m_log, &QPlainTextEdit::customContextMenuRequested, [this](const QPoint& pos) {
        QMenu* menu = new QMenu(this);
        auto* copyAll = menu->addAction("Copy All");
        auto* copySel = menu->addAction("Copy Selected");
        auto* clear   = menu->addAction("Clear");
        copySel->setEnabled(m_log->textCursor().hasSelection());
        connect(copyAll, &QAction::triggered, [this]{ qApp->clipboard()->setText(m_log->toPlainText()); });
        connect(copySel, &QAction::triggered, [this]{ m_log->copy(); });
        connect(clear,   &QAction::triggered, [this]{ m_log->clear(); });
        menu->exec(m_log->mapToGlobal(pos));
        delete menu;
    });
}

void BuildPanel::appendLog(const QString& text) {
    if (!m_log) return;
    for (const QString& line : text.split('\n')) {
        QString t = line.trimmed();
        if (t.isEmpty()) continue;
        // Colour errors/warnings
        QTextCharFormat fmt;
        if (t.contains("error:") || t.startsWith("✗"))
            fmt.setForeground(QColor("#f44336"));
        else if (t.contains("warning:"))
            fmt.setForeground(QColor("#f0a500"));
        else if (t.startsWith("✓") || t.startsWith("▶"))
            fmt.setForeground(QColor("#4caf50"));
        else
            fmt.setForeground(QColor("#ccc"));

        QTextCursor cursor = m_log->textCursor();
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(t + "\n", fmt);
        m_log->setTextCursor(cursor);
    }
    m_log->ensureCursorVisible();
    m_log->verticalScrollBar()->setValue(m_log->verticalScrollBar()->maximum());
    qApp->processEvents(); // flush UI immediately
}

void BuildPanel::build(const QString& entryFile, const QString& target,
                       const QString& outDir, bool runAfter) {
    if (m_proc && m_proc->state() != QProcess::NotRunning) {
        appendLog("⚠ Build already in progress.");
        return;
    }
    m_entryFile = entryFile;
    m_target    = target;
    m_outDir    = outDir;
    m_runAfter  = runAfter;

    QDir().mkpath(outDir);
    m_log->clear();

    appendLog("[" + QDateTime::currentDateTime().toString("hh:mm:ss") + "] Starting build...");
    appendLog("  Entry : " + entryFile);
    appendLog("  Target: " + (target.isEmpty() ? "linux64" : target));
    appendLog("  Output: " + outDir);
    appendLog("");

    // Find konscript
    m_konscript = "";
    QStringList candidates = {
        QCoreApplication::applicationDirPath() + "/konscript",
        QCoreApplication::applicationDirPath() + "/konscript.exe",
        "/usr/local/bin/konscript",
        "/usr/bin/konscript"
    };
    for (auto& c : candidates) {
        if (QFile::exists(c)) { m_konscript = c; break; }
    }

    if (m_konscript.isEmpty()) {
        appendLog("✗ konscript not found!");
        appendLog("  Install konscript to /usr/local/bin/ or place it next to KonEditor.");
        m_status->setText("konscript not found");
        m_status->setStyleSheet("color: #f44336; font-size: 11px;");
        return;
    }

    appendLog("  konscript: " + m_konscript);
    appendLog("");

    startBuild();
}

void BuildPanel::startBuild() {
    m_proc = new QProcess(this);
    // Separate channels so we can colour stdout/stderr differently
    m_proc->setProcessChannelMode(QProcess::SeparateChannels);

    connect(m_proc, &QProcess::readyReadStandardOutput, this, &BuildPanel::onProcessOutput);
    connect(m_proc, &QProcess::readyReadStandardError,  this, &BuildPanel::onProcessError);
    connect(m_proc, static_cast<void(QProcess::*)(int,QProcess::ExitStatus)>(&QProcess::finished),
            this, &BuildPanel::onProcessFinished);

    QString outBinary = m_outDir + (m_target == "windows64" ? "/game.exe" : "/game");
    QStringList args;
    if (!m_target.isEmpty() && m_target != "linux64")
        args << "--target" << m_target;
    args << m_entryFile << "-o" << outBinary;

    appendLog("$ " + m_konscript + " " + args.join(' '));
    appendLog("");

    m_progress->show();
    m_progress->setRange(0, 0); // indeterminate
    m_status->setText("Building...");
    m_status->setStyleSheet("color: #f0a500; font-size: 11px;");
    m_cancelBtn->setEnabled(true);

    m_proc->start(m_konscript, args);
    if (!m_proc->waitForStarted(3000)) {
        appendLog("✗ Failed to start konscript process: " + m_proc->errorString());
        m_proc->deleteLater();
        m_proc = nullptr;
        return;
    }

    // Timeout watchdog — kill if build hangs for > 60 seconds
    m_buildTimeout = new QTimer(this);
    m_buildTimeout->setSingleShot(true);
    m_buildTimeout->setInterval(60000);
    connect(m_buildTimeout, &QTimer::timeout, [this]{
        if (m_proc && m_proc->state() != QProcess::NotRunning) {
            m_proc->kill();
            appendLog("\n✗ Build timed out after 60s — compiler may be stuck.");
            appendLog("  Check your .ks files for infinite loops or syntax that confuses the parser.");
        }
    });
    m_buildTimeout->start();
}

void BuildPanel::onProcessOutput() {
    if (!m_proc) return;
    // Read ALL available bytes immediately — don't wait for newlines
    QByteArray bytes = m_proc->readAllStandardOutput();
    if (bytes.isEmpty()) return;
    QString text = QString::fromUtf8(bytes);

    // Parse [X/Y] progress markers
    QRegularExpression re(R"(\[(\d+)/(\d+)\])");
    auto m = re.match(text);
    if (m.hasMatch()) {
        int cur = m.captured(1).toInt();
        int tot = m.captured(2).toInt();
        m_progress->setRange(0, tot);
        m_progress->setValue(cur);
    }
    appendLog(text);
}

void BuildPanel::onProcessError() {
    if (!m_proc || m_proc->state() == QProcess::NotRunning) return;
    QByteArray bytes = m_proc->readAllStandardError();
    if (!bytes.isEmpty()) appendLog(QString::fromUtf8(bytes));
}

void BuildPanel::onProcessFinished(int exitCode, QProcess::ExitStatus) {
    if (m_buildTimeout) { m_buildTimeout->stop(); m_buildTimeout->deleteLater(); m_buildTimeout = nullptr; }
    if (!m_proc) return;
    // Disconnect all signals first to prevent re-entrant calls
    m_proc->disconnect();
    // Flush remaining output
    appendLog(m_proc->readAllStandardOutput());
    appendLog(m_proc->readAllStandardError());

    appendLog("");
    if (exitCode == 0) {
        appendLog("✓ Build succeeded in " +
                  QString::number(m_timer.elapsed() / 1000.0, 'f', 1) + "s");
        m_status->setText("✓ Build succeeded");
        m_status->setStyleSheet("color: #4caf50; font-size: 11px;");
        if (m_runAfter) runGame();
    } else {
        appendLog("✗ Build failed (exit code " + QString::number(exitCode) + ")");
        m_status->setText("✗ Build failed");
        m_status->setStyleSheet("color: #f44336; font-size: 11px;");
    }
    m_progress->hide();
    if (m_elapsedTimer) { m_elapsedTimer->stop(); m_elapsedTimer->deleteLater(); m_elapsedTimer = nullptr; }
    m_proc->disconnect();
    m_proc->deleteLater();
    m_proc = nullptr;
    m_cancelBtn->setEnabled(false);
}

void BuildPanel::runGame() {
    QString binary = m_outDir + (m_target == "windows64" ? "/game.exe" : "/game");
    if (!QFile::exists(binary)) {
        appendLog("✗ Binary not found: " + binary);
        return;
    }
    appendLog("▶ Running: " + binary + "\n");
    auto* gameProc = new QProcess();
    gameProc->setWorkingDirectory(QFileInfo(m_entryFile).absolutePath() + "/..");
    // Don't connect any signals — just fire and forget
    // Parent is nullptr so it won't be deleted with BuildPanel
    gameProc->start(binary, QStringList());
    if (!gameProc->waitForStarted(3000)) {
        appendLog("✗ Failed to start game: " + gameProc->errorString());
        delete gameProc;
        return;
    }
    appendLog("▶ Game running (PID " + QString::number(gameProc->processId()) + ")");
    emit gameStarted(gameProc);
}
