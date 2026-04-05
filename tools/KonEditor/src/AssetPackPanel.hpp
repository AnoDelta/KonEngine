#pragma once
#include <QWidget>
#include <QTreeWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QProcess>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

class AssetPackPanel : public QWidget {
    Q_OBJECT
public:
    explicit AssetPackPanel(QWidget* parent = nullptr);
    ~AssetPackPanel();

    void openPack(const QString& path);
    void newPack(const QString& directory);
    void setProjectRoot(const QString& root);

private slots:
    void onNewPack();
    void onOpenPack();
    void onAddFiles();
    void onExtract();
    void onPackAll();
    void onProcessFinished(int exitCode);

private:
    void setupUI();
    void appendLog(const QString& text);
    QString findKonpak() const;
    void refreshFileList();

    QString      m_projectRoot;
    QString      m_currentPackPath;
    QString      m_sourceDirectory;   // directory to pack

    QTreeWidget* m_fileList    = nullptr;
    QTextEdit*   m_logOutput   = nullptr;
    QLabel*      m_titleLabel  = nullptr;
    QProcess*    m_process     = nullptr;
};
