#pragma once
#include <QWidget>
#include <QTreeWidget>
#include <QPushButton>
#include <QMenu>
#include <QJsonObject>
#include <QJsonArray>
#include "SceneFormat.hpp"

class SceneTree : public QWidget {
    Q_OBJECT
public:
    explicit SceneTree(QWidget* parent = nullptr);

    void newScene();
    void setProjectRoot(const QString& root);
    void loadScene(const QString& path);
    void saveScene(const QString& path);
    bool hasScene() const { return m_sceneLoaded; }
    QTreeWidget* treeWidget() const { return m_tree; }

signals:
    void nodeSelected(const QString& name, const QString& type);
    void nodeAdded(const QString& name, const QString& type);
    void scriptRequested(const QString& nodeName, const QString& scriptPath);
    void nodeSelectedWithScript(const QString& name, const QString& type, const QString& script);
    void sceneChanged();
    void sceneLoaded(const QString& path);

private slots:
    void autoSaveScene();
    void onAttachScript();
    void onViewAsText();
public slots:
    void attachScriptToSelected();
    void selectNodeByName(const QString& name);
    void updateNodePosition(const QString& name, float x, float y);
    void onAddNode();
    void onDeleteNode();
    void onItemClicked(QTreeWidgetItem* item);
    void onContextMenu(const QPoint& pos);

private:
    QTreeWidget* m_tree        = nullptr;
    bool         m_sceneLoaded = false;
    QString      m_scenePath;
    QString      m_projectRoot;
    SceneFile    m_scene;
    SceneFile    m_prevScene;

    QTreeWidgetItem* addNode(QTreeWidgetItem* parent,
                             const QString& name, const QString& type);
    void buildNodePickerMenu(QMenu* menu);
    QJsonObject treeToJson(QTreeWidgetItem* item) const;
    void jsonToTree(QTreeWidgetItem* parent, const QJsonArray& nodes);
};
