#pragma once
#include <QWidget>
#include <QTreeView>
#include <QFileSystemModel>

class AssetBrowser : public QWidget {
    Q_OBJECT
public:
    explicit AssetBrowser(QWidget* parent = nullptr);
    void setRoot(const QString& path);
signals:
    void fileDoubleClicked(const QString& path);
    void newAnimRequested(const QString& dirPath);
    void newAssetPackRequested(const QString& dirPath);
    void openKonpakRequested(const QString& path);

private slots:
    void onContextMenu(const QPoint& pos);

private:
    QString m_root;
private:
    QTreeView*        m_tree  = nullptr;
    QFileSystemModel* m_model = nullptr;
};
