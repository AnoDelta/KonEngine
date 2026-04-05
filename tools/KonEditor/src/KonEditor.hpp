#pragma once
#include <QMainWindow>
#include <QSplitter>
#include <QTabWidget>
#include <QTabBar>
#include <QToolBar>
#include <QProcess>
#include <QLabel>
#include <QAction>
#include <QStackedWidget>
#include "ProjectManager.hpp"
#include "SceneTree.hpp"
#include "Inspector.hpp"
#include "Viewport.hpp"
#include "ScriptEditor.hpp"
#include "AssetBrowser.hpp"
#include "BuildPanel.hpp"
#include "DebugConsole.hpp"
#include <QTextEdit>
#include <QTreeWidget>
#include <QPushButton>

class KonEditor : public QMainWindow {
    Q_OBJECT
public:
    explicit KonEditor(QWidget* parent = nullptr);
    ~KonEditor();
    void openProject(const QString& path);

private slots:
    void onNewProject();
    void onOpenProject();
    void onSaveProject();
    void onProjectSettings();
    void onBuild();
    void onRun();
    void onStop();
    void onGameProcessOutput();
    void onGameProcessFinished(int exitCode);
    void rebuildViewport();
    void onOpenAnimFile(const QString& path);
    void onPackAssets();
    void syncInspectorPosition(const QString& name);
    void writeInstancePosition(const QString& scenePath, const QString& varName, float x, float y);

private:
    void setupMenuBar();
    void setupToolBar();
    void setupLayout();
    void setupStatusBar();
    void updateTitle();
    void updateRunButtons(bool running);

    // Layout
    QSplitter*     m_rootSplitter  = nullptr;
    QSplitter*     m_mainSplitter  = nullptr;

    // Left panel — Scene tree + File browser (switchable)
    QWidget*       m_leftPanel     = nullptr;
    QTabBar*       m_leftTabs      = nullptr;
    QStackedWidget* m_leftStack    = nullptr;
    SceneTree*     m_sceneTree     = nullptr;
    AssetBrowser*  m_assetBrowser  = nullptr;

    // Center panel
    QTabWidget*    m_centerTabs    = nullptr;
    Viewport*      m_viewport      = nullptr;
    ScriptEditor*  m_scriptEditor  = nullptr;
    QLabel*        m_animPlaceholder = nullptr;  // Animation tab placeholder

    // Right panel
    Inspector*     m_inspector     = nullptr;

    // Bottom panel
    QTabWidget*    m_bottomTabs    = nullptr;
    BuildPanel*    m_buildPanel    = nullptr;
    DebugConsole*  m_debugConsole  = nullptr;
    QWidget*       m_assetsTab     = nullptr;     // Asset pack tab
    QTreeWidget*   m_assetTree     = nullptr;
    QTextEdit*     m_packOutput    = nullptr;
    QProcess*      m_packProcess   = nullptr;

    // Project
    ProjectManager* m_project      = nullptr;
    QString         m_buildTarget  = "linux64";

    // Run state
    QProcess*  m_gameProcess  = nullptr;
    QString    m_selectedNode;  // tracks current selection so rebuild preserves it
    QAction*   m_runAction    = nullptr;
    QAction*   m_stopAction   = nullptr;
    QLabel*    m_statusLabel  = nullptr;
};
