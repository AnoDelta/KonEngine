#pragma once
#include <QMainWindow>
#include <QTreeWidget>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QStackedWidget>
#include <QSlider>
#include <QPushButton>
#include <QSplitter>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QMediaPlayer>
#include <QString>
#include <string>
#include "konpak.hpp"

class PreviewPanel;

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
    void onItemSelectionChanged();

private:
    QTreeWidget*   m_tree;
    QLabel*        m_statusLabel;
    QToolBar*      m_toolbar;
    QSplitter*     m_splitter;
    PreviewPanel*  m_preview;

    KonPak::Pack   m_pack;
    QString        m_currentPath;
    bool           m_dirty = false;

    // KonAnimator integration
    QString        m_konAnimatorPath;
    void           findKonAnimator();
    void           openInKonAnimator(const std::string& packPath);

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
    void previewEntry(const std::string& packPath);
};
