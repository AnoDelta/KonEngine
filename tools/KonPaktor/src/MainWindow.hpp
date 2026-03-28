#pragma once
#include <QMainWindow>
#include <QTreeWidget>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QString>
#include <string>
#include "konpak.hpp"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void dragEnterEvent(QDragEnterEvent* e) override;
    void dropEvent(QDropEvent* e) override;
    void closeEvent(QCloseEvent* e) override;

private slots:
    void onNewPack();
    void onOpenPack();
    void onSavePack();
    void onSavePackAs();
    void onAddFiles();
    void onRemoveSelected();
    void onExtractSelected();
    void onExtractAll();
    void onItemDoubleClicked(QTreeWidgetItem* item, int col);

private:
    QTreeWidget*  m_tree;
    QLabel*       m_statusLabel;
    QToolBar*     m_toolbar;

    KonPak::Pack  m_pack;
    QString       m_currentPath;
    bool          m_dirty = false;

    void setupUI();
    void setupToolbar();
    void setupMenuBar();
    void refreshTree();
    void setDirty(bool dirty);
    void setCurrentFile(const QString& path);
    void updateStatus();
    bool promptPassword(const QString& title, std::string& outPassword);
    bool ensureSaved();
    void addFilesToPack(const QStringList& paths);
    QString askPackPath(const QString& diskPath);
};
