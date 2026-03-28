#include "konpak.hpp"
#include "MainWindow.hpp"

#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPixmap>
#include <QProcess>
#include <QProgressDialog>
#include <QPushButton>
#include <QScrollArea>
#include <QSlider>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTextEdit>
#include <QUrl>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QDialog>
#include <QMediaPlayer>
#include <filesystem>

namespace fs = std::filesystem;

// -----------------------------------------------------------------------
// Password dialog
// -----------------------------------------------------------------------
class PasswordDialog : public QDialog {
public:
    PasswordDialog(const QString& title, QWidget* parent = nullptr)
        : QDialog(parent) {
        setWindowTitle(title);
        setFixedWidth(320);
        auto* layout = new QVBoxLayout(this);
        auto* form   = new QFormLayout;
        m_edit = new QLineEdit;
        m_edit->setEchoMode(QLineEdit::Password);
        m_edit->setPlaceholderText("Enter password...");
        form->addRow("Password:", m_edit);
        layout->addLayout(form);
        auto* btns = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
        connect(m_edit, &QLineEdit::returnPressed, this, &QDialog::accept);
        layout->addWidget(btns);
        m_edit->setFocus();
    }
    std::string password() const { return m_edit->text().toStdString(); }
private:
    QLineEdit* m_edit;
};

// -----------------------------------------------------------------------
// Pack path dialog
// -----------------------------------------------------------------------
class PackPathDialog : public QDialog {
public:
    PackPathDialog(const QString& suggested, QWidget* parent = nullptr)
        : QDialog(parent) {
        setWindowTitle("Add File");
        setFixedWidth(400);
        auto* layout = new QVBoxLayout(this);
        auto* form   = new QFormLayout;
        m_edit = new QLineEdit(suggested);
        form->addRow("Pack path:", m_edit);
        auto* hint = new QLabel(
            "<small>Path inside the archive, e.g. sprites/player.png</small>");
        hint->setWordWrap(true);
        form->addRow("", hint);
        layout->addLayout(form);
        auto* btns = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
        layout->addWidget(btns);
        m_edit->setFocus();
        m_edit->selectAll();
    }
    QString packPath() const { return m_edit->text(); }
private:
    QLineEdit* m_edit;
};

// -----------------------------------------------------------------------
// PreviewPanel
// -----------------------------------------------------------------------
class PreviewPanel : public QWidget {
    Q_OBJECT
public:
    explicit PreviewPanel(QWidget* parent = nullptr) : QWidget(parent) {
        setMinimumWidth(220);
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(4, 4, 4, 4);

        m_stack = new QStackedWidget;

        // Page 0: empty / no selection
        auto* emptyPage = new QWidget;
        auto* emptyLayout = new QVBoxLayout(emptyPage);
        auto* emptyLabel = new QLabel("Select a file\nto preview.");
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet("color: gray;");
        emptyLayout->addWidget(emptyLabel);
        m_stack->addWidget(emptyPage); // index 0

        // Page 1: image preview
        auto* imagePage = new QWidget;
        auto* imageLayout = new QVBoxLayout(imagePage);
        m_imageLabel = new QLabel;
        m_imageLabel->setAlignment(Qt::AlignCenter);
        m_imageLabel->setMinimumHeight(180);
        m_imageInfo = new QLabel;
        m_imageInfo->setAlignment(Qt::AlignCenter);
        m_imageInfo->setWordWrap(true);
        m_imageInfo->setStyleSheet("font-size: 11px; color: gray;");
        imageLayout->addWidget(m_imageLabel);
        imageLayout->addWidget(m_imageInfo);
        imageLayout->addStretch();
        m_stack->addWidget(imagePage); // index 1

        // Page 2: audio preview
        auto* audioPage = new QWidget;
        auto* audioLayout = new QVBoxLayout(audioPage);
        auto* audioIcon = new QLabel("♪");
        audioIcon->setAlignment(Qt::AlignCenter);
        audioIcon->setStyleSheet("font-size: 48px;");
        m_audioLabel = new QLabel;
        m_audioLabel->setAlignment(Qt::AlignCenter);
        m_audioLabel->setWordWrap(true);
        m_playBtn = new QPushButton("Play");
        m_stopBtn = new QPushButton("Stop");
        m_stopBtn->setEnabled(false);
        auto* btnRow = new QHBoxLayout;
        btnRow->addWidget(m_playBtn);
        btnRow->addWidget(m_stopBtn);
        m_audioInfo = new QLabel;
        m_audioInfo->setAlignment(Qt::AlignCenter);
        m_audioInfo->setStyleSheet("font-size: 11px; color: gray;");
        audioLayout->addWidget(audioIcon);
        audioLayout->addWidget(m_audioLabel);
        audioLayout->addLayout(btnRow);
        audioLayout->addWidget(m_audioInfo);
        audioLayout->addStretch();
        m_stack->addWidget(audioPage); // index 2

        // Page 3: info (konani, binary, unknown)
        auto* infoPage = new QWidget;
        auto* infoLayout = new QVBoxLayout(infoPage);
        m_infoText = new QTextEdit;
        m_infoText->setReadOnly(true);
        m_infoText->setStyleSheet("font-family: monospace; font-size: 11px;");
        infoLayout->addWidget(m_infoText);
        m_stack->addWidget(infoPage); // index 3

        layout->addWidget(m_stack);

        // Audio player
        m_player = new QMediaPlayer(this);
        connect(m_playBtn, &QPushButton::clicked, this, &PreviewPanel::onPlay);
        connect(m_stopBtn, &QPushButton::clicked, this, &PreviewPanel::onStop);
        connect(m_player, &QMediaPlayer::stateChanged,
                this, &PreviewPanel::onPlayerStateChanged);

        m_stack->setCurrentIndex(0);
    }

    void showEmpty() {
        onStop();
        m_stack->setCurrentIndex(0);
    }

    void showImage(const std::vector<uint8_t>& data, const QString& name) {
        onStop();
        QPixmap px;
        px.loadFromData(data.data(), (uint)data.size());
        if (px.isNull()) {
            showInfo(name, "Cannot decode image.");
            return;
        }
        m_imageLabel->setPixmap(
            px.scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        m_imageInfo->setText(
            QString("%1 x %2 px\n%3")
                .arg(px.width()).arg(px.height()).arg(name));
        m_stack->setCurrentIndex(1);
    }

    void showAudio(const std::vector<uint8_t>& data,
                   const QString& name, const QString& tmpPath) {
        onStop();
        // Write to temp file for QMediaPlayer
        m_tempAudioPath = tmpPath;
        QFile f(tmpPath);
        if (f.open(QIODevice::WriteOnly)) {
            f.write(reinterpret_cast<const char*>(data.data()), data.size());
            f.close();
        }
        m_audioLabel->setText(name);
        m_audioInfo->setText(QString("%1 bytes").arg(data.size()));
        m_playBtn->setEnabled(true);
        m_stopBtn->setEnabled(false);
        m_stack->setCurrentIndex(2);
    }

    void showInfo(const QString& name, const QString& info) {
        onStop();
        m_infoText->setPlainText(name + "\n\n" + info);
        m_stack->setCurrentIndex(3);
    }

private slots:
    void onPlay() {
        if (m_tempAudioPath.isEmpty()) return;
        m_player->setMedia(QUrl::fromLocalFile(m_tempAudioPath));
        m_player->play();
        m_playBtn->setEnabled(false);
        m_stopBtn->setEnabled(true);
    }

    void onStop() {
        m_player->stop();
        m_playBtn->setEnabled(true);
        m_stopBtn->setEnabled(false);
    }

    void onPlayerStateChanged(QMediaPlayer::State state) {
        if (state == QMediaPlayer::StoppedState) {
            m_playBtn->setEnabled(true);
            m_stopBtn->setEnabled(false);
        }
    }

private:
    QStackedWidget* m_stack;
    QLabel*         m_imageLabel;
    QLabel*         m_imageInfo;
    QLabel*         m_audioLabel;
    QLabel*         m_audioInfo;
    QPushButton*    m_playBtn;
    QPushButton*    m_stopBtn;
    QTextEdit*      m_infoText;
    QMediaPlayer*   m_player;
    QString         m_tempAudioPath;
};

#include "MainWindow.moc"

// -----------------------------------------------------------------------
// MainWindow
// -----------------------------------------------------------------------
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("KonPaktor");
    setMinimumSize(700, 480);
    resize(1000, 580);
    setAcceptDrops(true);
    setupMenuBar();
    setupToolbar();
    setupUI();
    findKonAnimator();
    updateStatus();
}

void MainWindow::setupUI() {
    m_splitter = new QSplitter(Qt::Horizontal);

    // Left: file tree
    m_tree = new QTreeWidget;
    m_tree->setColumnCount(3);
    m_tree->setHeaderLabels({"Path", "Raw size", "Packed size"});
    m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_tree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_tree->setAlternatingRowColors(true);
    m_tree->setSortingEnabled(true);
    m_tree->setContextMenuPolicy(Qt::ActionsContextMenu);

    auto* actExtract = new QAction("Extract selected...", m_tree);
    auto* actRemove  = new QAction("Remove from pack",    m_tree);
    auto* actOpenAnim = new QAction("Open in KonAnimator", m_tree);
    m_tree->addAction(actExtract);
    m_tree->addAction(actRemove);
    m_tree->addAction(actOpenAnim);
    connect(actExtract,  &QAction::triggered, this, &MainWindow::onExtractSelected);
    connect(actRemove,   &QAction::triggered, this, &MainWindow::onRemoveSelected);
    connect(actOpenAnim, &QAction::triggered, [this]() {
        auto selected = m_tree->selectedItems();
        if (!selected.isEmpty())
            openInKonAnimator(
                selected.first()->data(0, Qt::UserRole).toString().toStdString());
    });
    connect(m_tree, &QTreeWidget::itemDoubleClicked,
            this,   &MainWindow::onItemDoubleClicked);
    connect(m_tree, &QTreeWidget::itemSelectionChanged,
            this,   &MainWindow::onItemSelectionChanged);

    // Right: preview panel
    m_preview = new PreviewPanel;

    m_splitter->addWidget(m_tree);
    m_splitter->addWidget(m_preview);
    m_splitter->setStretchFactor(0, 3);
    m_splitter->setStretchFactor(1, 1);

    auto* central = new QWidget;
    auto* layout  = new QVBoxLayout(central);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->addWidget(m_splitter);

    m_statusLabel = new QLabel("No pack open.  Drag files here or use File > Open.");
    statusBar()->addWidget(m_statusLabel);

    setCentralWidget(central);
}

void MainWindow::setupToolbar() {
    m_toolbar = addToolBar("Main");
    m_toolbar->setMovable(false);

    auto* actNew    = m_toolbar->addAction("New");
    auto* actOpen   = m_toolbar->addAction("Open");
    auto* actSave   = m_toolbar->addAction("Save");
    m_toolbar->addSeparator();
    auto* actAdd    = m_toolbar->addAction("Add Files");
    auto* actRemove = m_toolbar->addAction("Remove");
    m_toolbar->addSeparator();
    auto* actExtAll = m_toolbar->addAction("Extract All");

    connect(actNew,    &QAction::triggered, this, &MainWindow::onNewPack);
    connect(actOpen,   &QAction::triggered, this, &MainWindow::onOpenPack);
    connect(actSave,   &QAction::triggered, this, &MainWindow::onSavePack);
    connect(actAdd,    &QAction::triggered, this, &MainWindow::onAddFiles);
    connect(actRemove, &QAction::triggered, this, &MainWindow::onRemoveSelected);
    connect(actExtAll, &QAction::triggered, this, &MainWindow::onExtractAll);
}

void MainWindow::setupMenuBar() {
    auto* file = menuBar()->addMenu("File");
    file->addAction("New Pack",     this, &MainWindow::onNewPack,    QKeySequence::New);
    file->addAction("Open Pack...", this, &MainWindow::onOpenPack,   QKeySequence::Open);
    file->addSeparator();
    file->addAction("Save",         this, &MainWindow::onSavePack,   QKeySequence::Save);
    file->addAction("Save As...",   this, &MainWindow::onSavePackAs, QKeySequence::SaveAs);
    file->addSeparator();
    file->addAction("Exit", this, &QWidget::close);

    auto* pack = menuBar()->addMenu("Pack");
    pack->addAction("Add Files...",        this, &MainWindow::onAddFiles);
    pack->addAction("Remove Selected",     this, &MainWindow::onRemoveSelected);
    pack->addSeparator();
    pack->addAction("Extract Selected...", this, &MainWindow::onExtractSelected);
    pack->addAction("Extract All...",      this, &MainWindow::onExtractAll);
}

// -----------------------------------------------------------------------
// KonAnimator integration
// -----------------------------------------------------------------------
void MainWindow::findKonAnimator() {
    // Look for KonAnimator relative to this executable and common build paths
    QStringList candidates = {
        QDir::current().filePath("../KonAnimator/build/KonAnimator"),
        QDir::current().filePath("../../KonAnimator/build/KonAnimator"),
        QDir::current().filePath("KonAnimator"),
        "/usr/local/bin/KonAnimator",
        "/usr/bin/KonAnimator"
    };
    for (auto& c : candidates) {
        if (QFileInfo::exists(c)) { m_konAnimatorPath = c; break; }
    }
}

void MainWindow::openInKonAnimator(const std::string& packPath) {
    QString ext = QFileInfo(QString::fromStdString(packPath)).suffix().toLower();
    bool isImage = (ext == "png" || ext == "jpg" || ext == "jpeg" ||
                    ext == "bmp" || ext == "tga");
    bool isAnim  = (ext == "konani" || ext == "anim");

    if (!isImage && !isAnim) {
        QMessageBox::information(this, "KonPaktor",
            "KonAnimator can only open image or animation files.");
        return;
    }

    if (m_konAnimatorPath.isEmpty()) {
        QMessageBox::warning(this, "KonPaktor",
            "KonAnimator not found.\n"
            "Build it with ./build-tools.sh and make sure it's in the PATH.");
        return;
    }

    // Extract to temp
    QString tmpDir = QDir::temp().filePath("KonPaktor_edit");
    fs::create_directories(tmpDir.toStdString());
    fs::path outPath = fs::path(tmpDir.toStdString()) / packPath;
    fs::create_directories(outPath.parent_path());

    try {
        m_pack.extractFile(packPath, outPath.string());
    } catch (std::exception& e) {
        QMessageBox::critical(this, "KonPaktor",
            QString("Failed to extract for editing:\n%1").arg(e.what()));
        return;
    }

    QString filePath = QString::fromStdString(outPath.string());
    QProcess::startDetached(m_konAnimatorPath, {filePath});

    QMessageBox::information(this, "KonPaktor",
        QString("Opened in KonAnimator:\n%1\n\n"
                "When done, use Pack > Add Files to update the pack.")
            .arg(filePath));
}

// -----------------------------------------------------------------------
// Preview
// -----------------------------------------------------------------------
void MainWindow::previewEntry(const std::string& packPath) {
    const KonPak::Entry* e = m_pack.find(packPath);
    if (!e) { m_preview->showEmpty(); return; }

    QString name = QString::fromStdString(packPath);
    QString ext  = QFileInfo(name).suffix().toLower();

    // Image
    if (ext == "png" || ext == "jpg" || ext == "jpeg" ||
        ext == "bmp" || ext == "tga" || ext == "gif") {
        m_preview->showImage(e->data, name);
        return;
    }

    // Audio
    if (ext == "wav" || ext == "ogg" || ext == "mp3" || ext == "flac") {
        QString tmpPath = QDir::temp().filePath(
            "KonPaktor_preview_" + QFileInfo(name).fileName());
        m_preview->showAudio(e->data, name, tmpPath);
        return;
    }

    // KonAni info
    if (ext == "konani") {
        QString info = QString("Binary animation file\nSize: %1 bytes\n\n"
                               "Double-click or right-click > Open in KonAnimator\n"
                               "to edit this animation.")
                           .arg(e->sizeRaw);
        m_preview->showInfo(name, info);
        return;
    }

    // Generic info
    QString info = QString("Size: %1 bytes (raw)\nPacked: %2 bytes\nType: %3")
                       .arg(e->sizeRaw)
                       .arg(e->sizePacked)
                       .arg(ext.isEmpty() ? "unknown" : ext);
    m_preview->showInfo(name, info);
}

// -----------------------------------------------------------------------
// Tree
// -----------------------------------------------------------------------
void MainWindow::refreshTree() {
    m_tree->clear();
    for (auto& e : m_pack.entries) {
        auto* item = new QTreeWidgetItem(m_tree);
        item->setText(0, QString::fromStdString(e.path));
        item->setText(1, QString("%1 B").arg(e.sizeRaw));
        item->setText(2, e.sizePacked > 0
            ? QString("%1 B").arg(e.sizePacked) : "-");
        item->setData(0, Qt::UserRole, QString::fromStdString(e.path));
    }
    m_tree->sortByColumn(0, Qt::AscendingOrder);
    m_preview->showEmpty();
    updateStatus();
}

void MainWindow::updateStatus() {
    if (m_currentPath.isEmpty()) {
        m_statusLabel->setText(
            "No pack open.  Drag files here or use File > Open.");
        return;
    }
    uint64_t totalRaw = 0;
    for (auto& e : m_pack.entries) totalRaw += e.sizeRaw;
    m_statusLabel->setText(
        QString("%1  |  %2 file(s)  |  %3 bytes total")
            .arg(QFileInfo(m_currentPath).fileName())
            .arg((int)m_pack.entries.size())
            .arg(totalRaw));
}

void MainWindow::setDirty(bool dirty) {
    m_dirty = dirty;
    QString title = "KonPaktor";
    if (!m_currentPath.isEmpty())
        title += " -- " + QFileInfo(m_currentPath).fileName();
    if (dirty) title += " *";
    setWindowTitle(title);
}

void MainWindow::setCurrentFile(const QString& path) {
    m_currentPath = path;
    setDirty(false);
    updateStatus();
}

// -----------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------
bool MainWindow::promptPassword(const QString& title, std::string& out) {
    PasswordDialog dlg(title, this);
    if (dlg.exec() != QDialog::Accepted) return false;
    out = dlg.password();
    if (out.empty()) {
        QMessageBox::warning(this, "KonPaktor", "Password cannot be empty.");
        return false;
    }
    return true;
}

bool MainWindow::ensureSaved() {
    if (!m_dirty) return true;
    auto btn = QMessageBox::question(this, "KonPaktor",
        "You have unsaved changes. Save now?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (btn == QMessageBox::Cancel) return false;
    if (btn == QMessageBox::Save)   { onSavePack(); return !m_dirty; }
    return true;
}

QString MainWindow::askPackPath(const QString& diskPath) {
    PackPathDialog dlg(QFileInfo(diskPath).fileName(), this);
    if (dlg.exec() != QDialog::Accepted) return {};
    return dlg.packPath();
}

// -----------------------------------------------------------------------
// File actions
// -----------------------------------------------------------------------
void MainWindow::onNewPack() {
    if (!ensureSaved()) return;
    std::string pw;
    if (!promptPassword("New Pack -- Set Password", pw)) return;
    m_pack = KonPak::Pack();
    m_pack.password = pw;
    m_currentPath.clear();
    refreshTree();
    setDirty(true);
    setWindowTitle("KonPaktor -- Untitled *");
}

void MainWindow::onOpenPack() {
    if (!ensureSaved()) return;
    QString path = QFileDialog::getOpenFileName(
        this, "Open Pack", {},
        "KonPak Archives (*.konpak);;All Files (*)");
    if (path.isEmpty()) return;

    std::string pw;
    if (!promptPassword("Open Pack -- Enter Password", pw)) return;

    KonPak::Pack tmp;
    tmp.password = pw;
    try {
        tmp.load(path.toStdString());
    } catch (std::exception& e) {
        QMessageBox::critical(this, "KonPaktor",
            QString("Failed to open:\n%1\n\nWrong password?").arg(e.what()));
        return;
    }
    m_pack = std::move(tmp);
    setCurrentFile(path);
    refreshTree();
}

void MainWindow::onSavePack() {
    if (m_currentPath.isEmpty()) { onSavePackAs(); return; }
    try {
        m_pack.save(m_currentPath.toStdString());
        setDirty(false);
    } catch (std::exception& e) {
        QMessageBox::critical(this, "KonPaktor",
            QString("Failed to save:\n%1").arg(e.what()));
    }
}

void MainWindow::onSavePackAs() {
    QString path = QFileDialog::getSaveFileName(
        this, "Save Pack As", {},
        "KonPak Archives (*.konpak);;All Files (*)");
    if (path.isEmpty()) return;
    if (!path.endsWith(".konpak")) path += ".konpak";
    try {
        m_pack.save(path.toStdString());
        setCurrentFile(path);
    } catch (std::exception& e) {
        QMessageBox::critical(this, "KonPaktor",
            QString("Failed to save:\n%1").arg(e.what()));
    }
}

// -----------------------------------------------------------------------
// Add files
// -----------------------------------------------------------------------
void MainWindow::addFilesToPack(const QStringList& paths) {
    if (m_pack.password.empty()) {
        std::string pw;
        if (!promptPassword("Set Pack Password", pw)) return;
        m_pack.password = pw;
    }
    int added = 0;
    for (auto& diskPath : paths) {
        QString packPath = askPackPath(diskPath);
        if (packPath.isEmpty()) continue;
        try {
            m_pack.addFile(diskPath.toStdString(), packPath.toStdString());
            added++;
        } catch (std::exception& e) {
            QMessageBox::warning(this, "KonPaktor",
                QString("Failed to add %1:\n%2").arg(diskPath).arg(e.what()));
        }
    }
    if (added > 0) { refreshTree(); setDirty(true); }
}

void MainWindow::onAddFiles() {
    QStringList paths = QFileDialog::getOpenFileNames(
        this, "Add Files to Pack", {}, "All Files (*)");
    if (!paths.isEmpty()) addFilesToPack(paths);
}

// -----------------------------------------------------------------------
// Remove
// -----------------------------------------------------------------------
void MainWindow::onRemoveSelected() {
    auto selected = m_tree->selectedItems();
    if (selected.isEmpty()) return;
    if (QMessageBox::question(this, "KonPaktor",
            QString("Remove %1 file(s) from the pack?").arg(selected.size()),
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;
    for (auto* item : selected)
        m_pack.remove(item->data(0, Qt::UserRole).toString().toStdString());
    refreshTree();
    setDirty(true);
}

// -----------------------------------------------------------------------
// Extract
// -----------------------------------------------------------------------
void MainWindow::onExtractSelected() {
    auto selected = m_tree->selectedItems();
    if (selected.isEmpty()) return;
    QString outDir = QFileDialog::getExistingDirectory(this, "Extract To...");
    if (outDir.isEmpty()) return;
    int done = 0;
    for (auto* item : selected) {
        std::string packPath = item->data(0, Qt::UserRole)
                                   .toString().toStdString();
        fs::path outPath = fs::path(outDir.toStdString()) / packPath;
        fs::create_directories(outPath.parent_path());
        try {
            m_pack.extractFile(packPath, outPath.string());
            done++;
        } catch (std::exception& e) {
            QMessageBox::warning(this, "KonPaktor",
                QString("Failed to extract %1:\n%2")
                    .arg(QString::fromStdString(packPath)).arg(e.what()));
        }
    }
    QMessageBox::information(this, "KonPaktor",
        QString("Extracted %1 file(s) to:\n%2").arg(done).arg(outDir));
}

void MainWindow::onExtractAll() {
    if (m_pack.entries.empty()) {
        QMessageBox::information(this, "KonPaktor", "Pack is empty.");
        return;
    }
    QString outDir = QFileDialog::getExistingDirectory(
        this, "Extract All To...");
    if (outDir.isEmpty()) return;

    QProgressDialog progress("Extracting...", "Cancel",
                             0, (int)m_pack.entries.size(), this);
    progress.setWindowModality(Qt::WindowModal);
    int done = 0;
    for (auto& e : m_pack.entries) {
        if (progress.wasCanceled()) break;
        progress.setValue(done);
        progress.setLabelText(QString::fromStdString(e.path));
        QApplication::processEvents();
        fs::path outPath = fs::path(outDir.toStdString()) / e.path;
        fs::create_directories(outPath.parent_path());
        try {
            m_pack.extractFile(e.path, outPath.string());
            done++;
        } catch (std::exception& ex) {
            QMessageBox::warning(this, "KonPaktor",
                QString("Failed: %1\n%2")
                    .arg(QString::fromStdString(e.path)).arg(ex.what()));
        }
    }
    progress.setValue((int)m_pack.entries.size());
    QMessageBox::information(this, "KonPaktor",
        QString("Extracted %1 file(s) to:\n%2").arg(done).arg(outDir));
}

// -----------------------------------------------------------------------
// Selection changed -> update preview
// -----------------------------------------------------------------------
void MainWindow::onItemSelectionChanged() {
    auto selected = m_tree->selectedItems();
    if (selected.size() == 1) {
        std::string packPath = selected.first()
            ->data(0, Qt::UserRole).toString().toStdString();
        previewEntry(packPath);
    } else {
        m_preview->showEmpty();
    }
}

// -----------------------------------------------------------------------
// Double click -> open in KonAnimator if image/anim, else OS default
// -----------------------------------------------------------------------
void MainWindow::onItemDoubleClicked(QTreeWidgetItem* item, int) {
    std::string packPath = item->data(0, Qt::UserRole)
                               .toString().toStdString();
    QString ext = QFileInfo(QString::fromStdString(packPath))
                      .suffix().toLower();

    bool isImage = (ext == "png" || ext == "jpg" || ext == "jpeg" ||
                    ext == "bmp" || ext == "tga");
    bool isAnim  = (ext == "konani" || ext == "anim");

    if ((isImage || isAnim) && !m_konAnimatorPath.isEmpty()) {
        auto btn = QMessageBox::question(this, "KonPaktor",
            "Open in KonAnimator?",
            QMessageBox::Yes | QMessageBox::No);
        if (btn == QMessageBox::Yes) {
            openInKonAnimator(packPath);
            return;
        }
    }

    // Fallback: extract to temp and open with OS default
    QString tmpDir = QDir::temp().filePath("KonPaktor_preview");
    fs::create_directories(tmpDir.toStdString());
    fs::path outPath = fs::path(tmpDir.toStdString()) / packPath;
    fs::create_directories(outPath.parent_path());
    try {
        m_pack.extractFile(packPath, outPath.string());
        QDesktopServices::openUrl(
            QUrl::fromLocalFile(
                QString::fromStdString(outPath.string())));
    } catch (std::exception& e) {
        QMessageBox::warning(this, "KonPaktor",
            QString("Cannot open:\n%1").arg(e.what()));
    }
}

// -----------------------------------------------------------------------
// Drag and drop
// -----------------------------------------------------------------------
void MainWindow::dragEnterEvent(QDragEnterEvent* e) {
    if (e->mimeData()->hasUrls()) e->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* e) {
    QStringList paths;
    for (auto& url : e->mimeData()->urls())
        if (url.isLocalFile()) paths << url.toLocalFile();
    if (!paths.isEmpty()) addFilesToPack(paths);
}

void MainWindow::closeEvent(QCloseEvent* e) {
    if (ensureSaved()) e->accept();
    else e->ignore();
}
