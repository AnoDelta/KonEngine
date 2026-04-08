#include "AssetBrowser.hpp"
#include <QVBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>
#include <QInputDialog>
#include <QFile>

AssetBrowser::AssetBrowser(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto* header = new QLabel("  Files");
    header->setStyleSheet("background: #252525; color: #aaa; padding: 4px 8px; "
                          "font-size: 11px; font-weight: bold;");
    layout->addWidget(header);

    m_model = new QFileSystemModel(this);
    m_model->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);

    m_tree = new QTreeView();
    m_tree->setModel(m_model);
    m_tree->setStyleSheet(
        "QTreeView { background: #1a1a1a; color: #ddd; border: none; }"
        "QTreeView::item { padding: 2px; }"
        "QTreeView::item:selected { background: #0078d7; }"
        "QTreeView::item:hover { background: #252525; }");
    m_tree->hideColumn(1);
    m_tree->hideColumn(2);
    m_tree->hideColumn(3);
    m_tree->setHeaderHidden(true);
    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    layout->addWidget(m_tree);

    connect(m_tree, &QTreeView::doubleClicked, [this](const QModelIndex& idx) {
        QString path = m_model->filePath(idx);
        if (!m_model->isDir(idx))
            emit fileDoubleClicked(path);
    });

    connect(m_tree, &QTreeView::customContextMenuRequested,
            this, &AssetBrowser::onContextMenu);
}

void AssetBrowser::setRoot(const QString& path) {
    m_root = path;
    m_model->setRootPath(path);
    m_tree->setRootIndex(m_model->index(path));
}

void AssetBrowser::onContextMenu(const QPoint& pos) {
    QModelIndex idx = m_tree->indexAt(pos);
    QString path = idx.isValid() ? m_model->filePath(idx) : m_root;
    bool isDir = idx.isValid() && m_model->isDir(idx);
    bool isFile = idx.isValid() && !isDir;

    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { background: #2a2a2a; color: #ddd; border: 1px solid #444; }"
        "QMenu::item:selected { background: #0078d7; }"
        "QMenu::separator { background: #3a3a3a; height: 1px; }");

    auto* openAct    = menu.addAction("Open");
    auto* revealAct  = menu.addAction("Show in File Manager");
    menu.addSeparator();
    auto* newFileAct = menu.addAction("New File...");
    auto* newAnimAct = menu.addAction("New Animation File...");
    auto* newPackAct = menu.addAction("New Asset Pack...");
    auto* newDirAct  = menu.addAction("New Folder...");
    menu.addSeparator();
    auto* renameAct  = menu.addAction("Rename...");
    auto* deleteAct  = menu.addAction("Delete");

    openAct->setEnabled(isFile);
    renameAct->setEnabled(idx.isValid());
    deleteAct->setEnabled(idx.isValid());
    deleteAct->setIcon(QIcon());

    // Style delete red
    deleteAct->setData("delete");

    auto* chosen = menu.exec(m_tree->viewport()->mapToGlobal(pos));
    if (!chosen) return;

    if (chosen == openAct) {
        emit fileDoubleClicked(path);

    } else if (chosen == revealAct) {
        QString dir = isDir ? path : QFileInfo(path).absolutePath();
        QDesktopServices::openUrl(QUrl::fromLocalFile(dir));

    } else if (chosen == newFileAct) {
        QString dir = isDir ? path : QFileInfo(path).absolutePath();
        bool ok;
        QString name = QInputDialog::getText(this, "New File", "File name:",
            QLineEdit::Normal, "new_file.ks", &ok);
        if (ok && !name.isEmpty()) {
            QFile f(dir + "/" + name);
            if (f.open(QIODevice::WriteOnly)) {
                if (name.endsWith(".ks"))
                    f.write("#include <engine>\n");
                f.close();
                emit fileDoubleClicked(dir + "/" + name);
            }
        }

    } else if (chosen == newAnimAct) {
        QString dir = isDir ? path : QFileInfo(path).absolutePath();
        bool ok;
        QString name = QInputDialog::getText(this, "New Animation", "File name:",
            QLineEdit::Normal, "new_anim.anim", &ok);
        if (ok && !name.isEmpty()) {
            if (!name.endsWith(".anim")) name += ".anim";
            QString fullPath = dir + "/" + name;
            QFile f(fullPath);
            if (f.open(QIODevice::WriteOnly)) {
                f.write("# New animation\n\nanim idle\n\tdisplay 32 32 1\nend\n");
                f.close();
                emit fileDoubleClicked(fullPath);
            }
        }

    } else if (chosen == newPackAct) {
        QString dir = isDir ? path : QFileInfo(path).absolutePath();
        emit newAssetPackRequested(dir);

    } else if (chosen == newDirAct) {
        QString dir = isDir ? path : QFileInfo(path).absolutePath();
        bool ok;
        QString name = QInputDialog::getText(this, "New Folder", "Folder name:",
            QLineEdit::Normal, "new_folder", &ok);
        if (ok && !name.isEmpty())
            QDir(dir).mkdir(name);

    } else if (chosen == renameAct) {
        QString oldName = QFileInfo(path).fileName();
        bool ok;
        QString newName = QInputDialog::getText(this, "Rename", "New name:",
            QLineEdit::Normal, oldName, &ok);
        if (ok && !newName.isEmpty() && newName != oldName) {
            QString newPath = QFileInfo(path).absolutePath() + "/" + newName;
            QDir().rename(path, newPath);
        }

    } else if (chosen == deleteAct) {
        QString name = QFileInfo(path).fileName();
        QString type = isDir ? "folder" : "file";
        auto btn = QMessageBox::warning(this, "Delete " + type,
            "Delete \"" + name + "\"?" +
            (isDir ? "\n\nThis will delete all contents." : ""),
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
        if (btn != QMessageBox::Yes) return;

        if (isDir)
            QDir(path).removeRecursively();
        else
            QFile::remove(path);
    }
}
