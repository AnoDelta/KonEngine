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
#include <mutex>
#include <atomic>
#include <string>
#include "konpak.hpp"

class PreviewPanel;


#include <QThread>

class SaveWorker : public QThread {
    Q_OBJECT
public:
    std::atomic<int>  current{0};
    std::atomic<int>  total{0};
    std::atomic<bool> finished{false};
    std::string       currentName;
    std::string       errorMsg;
    std::mutex        nameMutex;

    SaveWorker(const KonPak::Pack* pack, const QString& path, QObject* parent = nullptr)
        : QThread(parent), m_pack(pack), m_path(path) {}

    void run() override {
        try {
            total = (int)m_pack->entries.size();
            m_pack->save(m_path.toStdString(),
                [this](int cur, int tot, const std::string& name) {
                    { std::lock_guard<std::mutex> lk(nameMutex); currentName = name; }
                    current = cur;
                    total   = tot;
                });
            finished = true;
        } catch (std::exception& ex) {
            errorMsg = ex.what();
            finished = true;
        }
    }

private:
    const KonPak::Pack* m_pack;
    QString             m_path;
};

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
