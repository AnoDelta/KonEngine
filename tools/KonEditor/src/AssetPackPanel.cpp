#include "AssetPackPanel.hpp"
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QApplication>
#include <QMessageBox>

AssetPackPanel::AssetPackPanel(QWidget* parent) : QWidget(parent) {
    setupUI();
}

AssetPackPanel::~AssetPackPanel() {
    if (m_process && m_process->state() != QProcess::NotRunning)
        m_process->kill();
}

void AssetPackPanel::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Toolbar
    auto* toolbar = new QWidget();
    toolbar->setStyleSheet("QWidget { background: #242424; border-bottom: 1px solid #333; }");
    auto* tbLayout = new QHBoxLayout(toolbar);
    tbLayout->setContentsMargins(4, 2, 4, 2);
    tbLayout->setSpacing(4);

    auto makeBtn = [](const QString& text) {
        auto* btn = new QPushButton(text);
        btn->setFixedHeight(24);
        return btn;
    };

    auto* newBtn     = makeBtn("New Pack");
    auto* openBtn    = makeBtn("Open Pack");
    auto* addBtn     = makeBtn("Add Files");
    auto* extractBtn = makeBtn("Extract");
    auto* packBtn    = makeBtn("Pack All");
    tbLayout->addWidget(newBtn);
    tbLayout->addWidget(openBtn);
    tbLayout->addWidget(addBtn);
    tbLayout->addWidget(extractBtn);
    tbLayout->addWidget(packBtn);
    tbLayout->addStretch();

    mainLayout->addWidget(toolbar);

    // Title
    m_titleLabel = new QLabel("No pack open");
    m_titleLabel->setStyleSheet("QLabel { color: #888; padding: 4px 8px; font-size: 11px; }");
    mainLayout->addWidget(m_titleLabel);

    // File list
    m_fileList = new QTreeWidget();
    m_fileList->setHeaderLabels({"File", "Size"});
    m_fileList->setColumnWidth(0, 350);
    m_fileList->setRootIsDecorated(false);
    m_fileList->setAlternatingRowColors(true);
    mainLayout->addWidget(m_fileList, 1);

    // Log output
    m_logOutput = new QTextEdit();
    m_logOutput->setReadOnly(true);
    m_logOutput->setMaximumHeight(80);
    m_logOutput->setStyleSheet(
        "QTextEdit { background: #141414; color: #aaa; font-family: monospace; font-size: 10px; }");
    mainLayout->addWidget(m_logOutput);

    connect(newBtn,     &QPushButton::clicked, this, &AssetPackPanel::onNewPack);
    connect(openBtn,    &QPushButton::clicked, this, &AssetPackPanel::onOpenPack);
    connect(addBtn,     &QPushButton::clicked, this, &AssetPackPanel::onAddFiles);
    connect(extractBtn, &QPushButton::clicked, this, &AssetPackPanel::onExtract);
    connect(packBtn,    &QPushButton::clicked, this, &AssetPackPanel::onPackAll);
}

void AssetPackPanel::setProjectRoot(const QString& root) {
    m_projectRoot = root;
}

void AssetPackPanel::openPack(const QString& path) {
    m_currentPackPath = path;
    m_titleLabel->setText("Pack: " + QFileInfo(path).fileName());
    appendLog("Opened: " + path);

    // Try to list contents using konpak
    QString konpak = findKonpak();
    if (konpak.isEmpty()) {
        appendLog("konpak not found -- cannot list pack contents.");
        return;
    }

    m_fileList->clear();
    m_process = new QProcess(this);
    m_process->setProgram(konpak);
    m_process->setArguments({"list", path});
    connect(m_process, &QProcess::readyReadStandardOutput, [this]{
        QString out = m_process->readAllStandardOutput();
        for (const auto& line : out.split('\n', Qt::SkipEmptyParts)) {
            auto* item = new QTreeWidgetItem();
            item->setText(0, line.trimmed());
            m_fileList->addTopLevelItem(item);
        }
    });
    connect(m_process, &QProcess::readyReadStandardError, [this]{
        appendLog("[err] " + m_process->readAllStandardError());
    });
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &AssetPackPanel::onProcessFinished);
    m_process->start();
}

void AssetPackPanel::newPack(const QString& directory) {
    m_sourceDirectory = directory;
    m_currentPackPath.clear();
    m_titleLabel->setText("New pack from: " + QFileInfo(directory).fileName() + "/");
    refreshFileList();
}

void AssetPackPanel::onNewPack() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Directory to Pack",
        m_projectRoot.isEmpty() ? QString() : m_projectRoot);
    if (!dir.isEmpty())
        newPack(dir);
}

void AssetPackPanel::onOpenPack() {
    QString path = QFileDialog::getOpenFileName(this, "Open Asset Pack", m_projectRoot,
        "KonPak Files (*.konpak);;All Files (*)");
    if (!path.isEmpty())
        openPack(path);
}

void AssetPackPanel::onAddFiles() {
    if (m_sourceDirectory.isEmpty()) {
        QMessageBox::information(this, "Add Files",
            "Create or open a pack first, then add files to the source directory.");
        return;
    }

    QStringList files = QFileDialog::getOpenFileNames(this, "Add Files to Pack");
    if (files.isEmpty()) return;

    QDir destDir(m_sourceDirectory);
    for (const auto& src : files) {
        QString destPath = destDir.absoluteFilePath(QFileInfo(src).fileName());
        QFile::copy(src, destPath);
    }
    appendLog(QString("Added %1 file(s) to %2").arg(files.size()).arg(m_sourceDirectory));
    refreshFileList();
}

void AssetPackPanel::onExtract() {
    if (m_currentPackPath.isEmpty()) {
        appendLog("No pack file open to extract.");
        return;
    }

    QString destDir = QFileDialog::getExistingDirectory(this, "Extract To");
    if (destDir.isEmpty()) return;

    QString konpak = findKonpak();
    if (konpak.isEmpty()) { appendLog("konpak not found."); return; }

    m_process = new QProcess(this);
    m_process->setProgram(konpak);
    m_process->setArguments({"extract", m_currentPackPath, destDir});
    connect(m_process, &QProcess::readyReadStandardOutput, [this]{
        appendLog(m_process->readAllStandardOutput());
    });
    connect(m_process, &QProcess::readyReadStandardError, [this]{
        appendLog("[err] " + m_process->readAllStandardError());
    });
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &AssetPackPanel::onProcessFinished);
    m_process->start();
}

void AssetPackPanel::onPackAll() {
    if (m_sourceDirectory.isEmpty()) {
        appendLog("No source directory set. Use 'New Pack' first.");
        return;
    }

    QString konpak = findKonpak();
    if (konpak.isEmpty()) { appendLog("konpak not found."); return; }

    QString outPath = m_currentPackPath;
    if (outPath.isEmpty()) {
        outPath = QFileDialog::getSaveFileName(this, "Save Pack As",
            m_projectRoot, "KonPak Files (*.konpak)");
        if (outPath.isEmpty()) return;
        m_currentPackPath = outPath;
    }

    appendLog("Packing " + m_sourceDirectory + " -> " + outPath);

    m_process = new QProcess(this);
    m_process->setProgram(konpak);
    m_process->setArguments({m_sourceDirectory, outPath});
    connect(m_process, &QProcess::readyReadStandardOutput, [this]{
        appendLog(m_process->readAllStandardOutput());
    });
    connect(m_process, &QProcess::readyReadStandardError, [this]{
        appendLog("[err] " + m_process->readAllStandardError());
    });
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &AssetPackPanel::onProcessFinished);
    m_process->start();
}

void AssetPackPanel::onProcessFinished(int exitCode) {
    if (exitCode == 0)
        appendLog("Done (success).");
    else
        appendLog(QString("Process exited with code %1").arg(exitCode));

    if (m_process) {
        m_process->deleteLater();
        m_process = nullptr;
    }
}

void AssetPackPanel::appendLog(const QString& text) {
    m_logOutput->append(text);
}

QString AssetPackPanel::findKonpak() const {
    QStringList candidates = {
        QApplication::applicationDirPath() + "/konpak",
        QApplication::applicationDirPath() + "/../KonPaktor/build/konpak",
        QApplication::applicationDirPath() + "/../../KonPaktor/build/konpak",
    };
    for (const auto& c : candidates)
        if (QFile::exists(c)) return c;
    return {};
}

void AssetPackPanel::refreshFileList() {
    m_fileList->clear();
    if (m_sourceDirectory.isEmpty()) return;

    QDir dir(m_sourceDirectory);
    if (!dir.exists()) return;

    QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
    for (const auto& fi : files) {
        auto* item = new QTreeWidgetItem();
        item->setText(0, fi.fileName());
        qint64 sz = fi.size();
        QString sizeStr = sz < 1024 ? QString("%1 B").arg(sz)
            : sz < 1048576 ? QString("%1 KB").arg(sz / 1024.0, 0, 'f', 1)
            : QString("%1 MB").arg(sz / 1048576.0, 0, 'f', 2);
        item->setText(1, sizeStr);
        m_fileList->addTopLevelItem(item);
    }
    appendLog(QString("Listed %1 files from %2").arg(files.size()).arg(m_sourceDirectory));
}
