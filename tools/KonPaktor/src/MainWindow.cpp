#define MINIZ_IMPLEMENTATION
#include "konpak.hpp"
#include "MainWindow.hpp"

#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QProgressDialog>
#include <QStatusBar>
#include <QUrl>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QDialog>
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
// MainWindow
// -----------------------------------------------------------------------
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("KonPaktor");
    setMinimumSize(640, 480);
    resize(900, 580);
    setAcceptDrops(true);
    setupMenuBar();
    setupToolbar();
    setupUI();
    updateStatus();
}

void MainWindow::setupUI() {
    auto* central = new QWidget;
    auto* layout  = new QVBoxLayout(central);
    layout->setContentsMargins(4, 4, 4, 4);

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
    m_tree->setDragDropMode(QAbstractItemView::DropOnly);

    auto* actExtract = new QAction("Extract selected...", m_tree);
    auto* actRemove  = new QAction("Remove from pack",    m_tree);
    m_tree->addAction(actExtract);
    m_tree->addAction(actRemove);
    connect(actExtract, &QAction::triggered, this, &MainWindow::onExtractSelected);
    connect(actRemove,  &QAction::triggered, this, &MainWindow::onRemoveSelected);
    connect(m_tree, &QTreeWidget::itemDoubleClicked,
            this,   &MainWindow::onItemDoubleClicked);

    layout->addWidget(m_tree);

    m_statusLabel = new QLabel("No pack open.  Drag files here or use File > Open.");
    statusBar()->addWidget(m_statusLabel);

    setCentralWidget(central);
}

void MainWindow::setupToolbar() {
    m_toolbar = addToolBar("Main");
    m_toolbar->setMovable(false);
    m_toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

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

void MainWindow::onItemDoubleClicked(QTreeWidgetItem* item, int) {
    std::string packPath = item->data(0, Qt::UserRole)
                               .toString().toStdString();
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
            QString("Cannot preview:\n%1").arg(e.what()));
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
