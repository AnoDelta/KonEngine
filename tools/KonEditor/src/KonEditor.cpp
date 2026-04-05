#include "KonEditor.hpp"
#include <QMenuBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QSettings>
#include <QFile>
#include <QFileInfo>
#include <functional>
#include <QDialog>
#include <QFormLayout>
#include <QSpinBox>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QRegularExpression>
#include <QStatusBar>
#include <QGroupBox>
#include <QTextStream>
#include <QDir>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTextEdit>
#include <QTimer>

// ── Dark theme ────────────────────────────────────────────────────────────
static void applyDarkTheme() {
    qApp->setStyle("Fusion");
    QPalette p;
    p.setColor(QPalette::Window,          QColor(28,28,28));
    p.setColor(QPalette::WindowText,      QColor(220,220,220));
    p.setColor(QPalette::Base,            QColor(20,20,20));
    p.setColor(QPalette::AlternateBase,   QColor(34,34,34));
    p.setColor(QPalette::Text,            QColor(220,220,220));
    p.setColor(QPalette::Button,          QColor(44,44,44));
    p.setColor(QPalette::ButtonText,      QColor(220,220,220));
    p.setColor(QPalette::Highlight,       QColor(0,120,215));
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::ToolTipBase,     QColor(38,38,38));
    p.setColor(QPalette::ToolTipText,     QColor(220,220,220));
    qApp->setPalette(p);
    qApp->setStyleSheet(R"(
        QMainWindow, QWidget { background: #1c1c1c; }
        QMenuBar { background: #242424; color: #ccc; border-bottom: 1px solid #333; }
        QMenuBar::item:selected { background: #333; }
        QMenu { background: #262626; color: #ccc; border: 1px solid #3a3a3a; }
        QMenu::item:selected { background: #0078d7; color: #fff; }
        QMenu::separator { background: #3a3a3a; height: 1px; }
        QToolBar { background: #242424; border-bottom: 1px solid #333; spacing: 4px; padding: 2px; }
        QToolBar::separator { background: #3a3a3a; width: 1px; margin: 4px 2px; }
        QTabWidget::pane { border: 1px solid #333; }
        QTabBar::tab { background: #262626; color: #999; padding: 5px 12px;
                       border: 1px solid #333; border-bottom: none; min-width: 60px; }
        QTabBar::tab:selected { background: #1c1c1c; color: #fff;
                                border-bottom: 2px solid #0078d7; }
        QTabBar::tab:hover:!selected { background: #2e2e2e; color: #ccc; }
        QSplitter::handle { background: #2e2e2e; }
        QSplitter::handle:horizontal { width: 1px; }
        QSplitter::handle:vertical { height: 1px; }
        QStatusBar { background: #0e2a47; color: #7ab3d8; font-size: 11px; }
        QStatusBar::item { border: none; }
        QPushButton { background: #363636; color: #ddd; border: 1px solid #4a4a4a;
                      padding: 4px 12px; border-radius: 3px; }
        QPushButton:hover { background: #424242; border-color: #666; }
        QPushButton:pressed { background: #0078d7; color: #fff; }
        QPushButton:disabled { color: #555; border-color: #3a3a3a; }
        QComboBox { background: #363636; color: #ddd; border: 1px solid #4a4a4a;
                    padding: 3px 8px; border-radius: 3px; }
        QComboBox::drop-down { border: none; }
        QComboBox QAbstractItemView { background: #262626; color: #ddd;
                                       selection-background-color: #0078d7; }
        QLineEdit, QSpinBox, QDoubleSpinBox {
            background: #1e1e1e; color: #ddd; border: 1px solid #4a4a4a;
            padding: 3px 6px; border-radius: 3px; }
        QTreeWidget { background: #181818; color: #ddd; border: none; }
        QTreeWidget::item { padding: 2px; }
        QTreeWidget::item:selected { background: #0078d7; }
        QTreeWidget::item:hover { background: #2a2a2a; }
        QScrollBar:vertical { background: #1c1c1c; width: 8px; }
        QScrollBar::handle:vertical { background: #3a3a3a; border-radius: 4px; min-height: 20px; }
        QScrollBar::handle:vertical:hover { background: #555; }
        QScrollBar::add-line, QScrollBar::sub-line { height: 0; }
        QCheckBox { color: #ddd; }
        QCheckBox::indicator { width: 14px; height: 14px; background: #1e1e1e;
                               border: 1px solid #555; border-radius: 2px; }
        QCheckBox::indicator:checked { background: #0078d7; border-color: #0078d7; }
        QLabel { color: #ccc; }
        QGroupBox { color: #888; border: 1px solid #3a3a3a; border-radius: 4px;
                    margin-top: 8px; padding-top: 4px; }
        QGroupBox::title { subcontrol-origin: margin; left: 8px; }
    )");
}

// ── KonEditor ─────────────────────────────────────────────────────────────
KonEditor::KonEditor(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("KonEditor");
    setMinimumSize(1280, 720);
    resize(1440, 900);
    applyDarkTheme();

    m_project = new ProjectManager(this);
    setupMenuBar();
    setupToolBar();
    setupLayout();
    setupStatusBar();

    // Note: project is opened by main.cpp after the welcome screen.
    // Do NOT auto-open here — it would race with the welcome screen's openProject call.
}

KonEditor::~KonEditor() {
    if (m_gameProcess && m_gameProcess->state() != QProcess::NotRunning)
        m_gameProcess->kill();
}

void KonEditor::setupMenuBar() {
    // File
    auto* file = menuBar()->addMenu("&File");
    file->addAction("&New Project",   this, &KonEditor::onNewProject,   QKeySequence::New);
    file->addAction("&Open Project",  this, &KonEditor::onOpenProject,  QKeySequence::Open);
    file->addAction("&Save",          this, &KonEditor::onSaveProject,  QKeySequence::Save);
    file->addSeparator();
    file->addAction("&Quit", qApp, &QApplication::quit, QKeySequence::Quit);

    // Project
    auto* proj = menuBar()->addMenu("&Project");
    proj->addAction("⚙ Project Settings", this, &KonEditor::onProjectSettings);

    // Build
    auto* build = menuBar()->addMenu("&Build");
    build->addAction("Build",    this, &KonEditor::onBuild, QKeySequence("Ctrl+B"));
    build->addAction("Run",      this, &KonEditor::onRun,   QKeySequence("Ctrl+R"));
    build->addAction("Stop",     this, &KonEditor::onStop,  QKeySequence("Ctrl+."));
}

void KonEditor::setupToolBar() {
    auto* tb = addToolBar("Main");
    tb->setMovable(false);
    tb->setIconSize({16,16});

    // Target selector
    tb->addWidget(new QLabel("  Target: "));
    auto* target = new QComboBox();
    target->addItems({"linux64","windows64"});
    target->setFixedWidth(100);
    connect(target, &QComboBox::currentTextChanged,
            [this](const QString& t){ m_buildTarget = t; });
    tb->addWidget(target);
    tb->addSeparator();

    // Build
    auto* buildAct = tb->addAction("⚙  Build");
    connect(buildAct, &QAction::triggered, this, &KonEditor::onBuild);

    // Run
    m_runAction = tb->addAction("▶  Run");
    connect(m_runAction, &QAction::triggered, this, &KonEditor::onRun);

    // Stop
    m_stopAction = tb->addAction("■  Stop");
    m_stopAction->setEnabled(false);
    connect(m_stopAction, &QAction::triggered, this, &KonEditor::onStop);
}

void KonEditor::setupLayout() {
    auto* central = new QWidget(this);
    auto* rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(0,0,0,0);
    rootLayout->setSpacing(0);
    setCentralWidget(central);

    // Outer vertical splitter: [top area] / [bottom]
    m_mainSplitter = new QSplitter(Qt::Vertical);

    // Inner horizontal splitter: [left] / [center] / [right]
    m_rootSplitter = new QSplitter(Qt::Horizontal);

    // ── Left panel: Scene Tree + File Browser ────────────────────────────
    m_leftPanel = new QWidget();
    m_leftPanel->setMinimumWidth(200);
    m_leftPanel->setMaximumWidth(320);
    auto* leftLayout = new QVBoxLayout(m_leftPanel);
    leftLayout->setContentsMargins(0,0,0,0);
    leftLayout->setSpacing(0);

    // Tab switcher at top of left panel
    m_leftTabs = new QTabBar();
    m_leftTabs->addTab("Scene");
    m_leftTabs->addTab("Files");
    m_leftTabs->setStyleSheet(
        "QTabBar { background: #242424; }"
        "QTabBar::tab { background: #242424; color: #888; padding: 5px 16px; "
        "               border: none; border-bottom: 2px solid transparent; }"
        "QTabBar::tab:selected { color: #fff; border-bottom: 2px solid #0078d7; }"
        "QTabBar::tab:hover:!selected { color: #bbb; }");
    leftLayout->addWidget(m_leftTabs);

    m_leftStack = new QStackedWidget();
    m_sceneTree = new SceneTree();
    m_assetBrowser = new AssetBrowser();
    m_leftStack->addWidget(m_sceneTree);   // index 0
    m_leftStack->addWidget(m_assetBrowser); // index 1
    leftLayout->addWidget(m_leftStack);

    connect(m_leftTabs, &QTabBar::currentChanged,
            m_leftStack, &QStackedWidget::setCurrentIndex);

    m_rootSplitter->addWidget(m_leftPanel);

    // ── Center: Script / Viewport / Animation ───────────────────────────
    m_centerTabs = new QTabWidget();
    m_viewport     = new Viewport();
    m_scriptEditor = new ScriptEditor();
    m_centerTabs->addTab(m_viewport,     "Viewport");
    m_centerTabs->addTab(m_scriptEditor, "Script");

    // Animation tab — placeholder until user opens an .anim file
    m_animPlaceholder = new QLabel("Open an .anim file from the file browser to edit animations");
    m_animPlaceholder->setAlignment(Qt::AlignCenter);
    m_animPlaceholder->setStyleSheet("QLabel { color: #666; font-size: 13px; }");
    m_centerTabs->addTab(m_animPlaceholder, "Animation");

    m_rootSplitter->addWidget(m_centerTabs);

    // ── Right: Inspector ──────────────────────────────────────────────────
    m_inspector = new Inspector();
    m_inspector->setMinimumWidth(220);
    m_inspector->setMaximumWidth(340);
    m_rootSplitter->addWidget(m_inspector);

    // Wire scene tree → inspector now that inspector exists
    connect(m_sceneTree, &SceneTree::nodeSelected,
            m_inspector, &Inspector::showNode);

    // Reload script tab when inspector edits a file
    connect(m_inspector, &Inspector::scriptFileChanged,
            [this](const QString& path){
                m_scriptEditor->reloadFile(path);
                // Any script change → autosave scene + rebuild viewport
                m_sceneTree->autoSaveScene();
                QTimer::singleShot(50, this, [this]{ rebuildViewport(); });
            });

    // Wire inspector open/attach script signals
    connect(m_inspector, &Inspector::propertyChanged,
            [this](const QString& node, const QString& prop, const QString& val){
                if (prop == "__openScript__") {
                    m_scriptEditor->openFile(val);
                    m_centerTabs->setCurrentWidget(m_scriptEditor);
                } else if (prop == "__attachScript__") {
                    m_sceneTree->attachScriptToSelected();
                } else if (prop == "x" || prop == "y") {
                    // Inspector edits local position; update tree and viewport accordingly
                    bool ok;
                    float v = val.toFloat(&ok);
                    if (!ok) return;
                    // Update the LOCAL position in the tree item
                    float localX = (prop == "x") ? v : 0.0f;
                    float localY = (prop == "y") ? v : 0.0f;
                    // Read existing local values from tree
                    auto* tw = m_sceneTree->treeWidget();
                    QTreeWidgetItem* nodeItem = nullptr;
                    if (tw && tw->topLevelItemCount()) {
                        std::function<QTreeWidgetItem*(QTreeWidgetItem*, const QString&)> findIt;
                        findIt = [&](QTreeWidgetItem* item, const QString& n) -> QTreeWidgetItem* {
                            if (item->data(0, Qt::UserRole+1).toString() == n) return item;
                            for (int i = 0; i < item->childCount(); i++)
                                if (auto* r = findIt(item->child(i), n)) return r;
                            return nullptr;
                        };
                        nodeItem = findIt(tw->topLevelItem(0), node);
                    }
                    if (nodeItem) {
                        QVariant evx = nodeItem->data(0, Qt::UserRole+3);
                        QVariant evy = nodeItem->data(0, Qt::UserRole+4);
                        if (prop == "x") {
                            localX = v;
                            localY = evy.isValid() ? evy.toFloat() : 0.0f;
                        } else {
                            localX = evx.isValid() ? evx.toFloat() : 0.0f;
                            localY = v;
                        }
                    }
                    m_sceneTree->updateNodePosition(node, localX, localY);
                    // Compute world position for viewport display
                    float parentWorldX = 0.0f, parentWorldY = 0.0f;
                    QTreeWidgetItem* parentItem = nullptr;
                    if (nodeItem) {
                        parentItem = nodeItem->parent();
                        QList<QTreeWidgetItem*> ancestors;
                        for (auto* p = parentItem; p; p = p->parent()) {
                            QString ptype = p->data(0, Qt::UserRole).toString();
                            if (ptype != "Scene") ancestors.prepend(p);
                        }
                        for (auto* a : ancestors) {
                            QVariant avx = a->data(0, Qt::UserRole+3);
                            QVariant avy = a->data(0, Qt::UserRole+4);
                            parentWorldX += avx.isValid() ? avx.toFloat() : 0.0f;
                            parentWorldY += avy.isValid() ? avy.toFloat() : 0.0f;
                        }
                    }
                    float worldX = parentWorldX + localX;
                    float worldY = parentWorldY + localY;
                    auto nodes = m_viewport->nodes();
                    for (auto& vn : nodes) {
                        if (vn.name == node) {
                            vn.x = worldX;
                            vn.y = worldY;
                        }
                    }
                    m_viewport->setNodes(nodes);
                    // Write LOCAL position to the correct script file
                    QString scenePath = m_sceneTree->scenePath();
                    if (!scenePath.isEmpty()) {
                        QString targetScript = scenePath;
                        if (parentItem) {
                            QString parentScript = parentItem->data(0, Qt::UserRole+2).toString();
                            if (!parentScript.isEmpty())
                                targetScript = parentScript;
                        }
                        writeInstancePosition(targetScript, node.toLower(), localX, localY);
                    }
                } else {
                    // Any other property change — autosave scene and rebuild viewport
                    m_sceneTree->autoSaveScene();
                    QTimer::singleShot(50, this, [this]{ rebuildViewport(); });
                }
            });

    connect(m_sceneTree, &SceneTree::nodeSelected,
            m_viewport, &Viewport::selectNode);

    connect(m_sceneTree, &SceneTree::nodeSelectedWithScript,
            this, [this](const QString& name, const QString& type, const QString& script){
                m_selectedNode = name;  // remember for post-rebuild re-selection
                m_inspector->showNodeFromFile(name, type, script);
                syncInspectorPosition(name);
            });

    // Remember last loaded scene per project
    connect(m_sceneTree, &SceneTree::sceneLoaded,
            [this](const QString& path){
                if (m_project->isOpen())
                    QSettings("AnoDelta","KonEditor").setValue(
                        "lastScene/" + m_project->path(), path);
                m_statusLabel->setText("  Scene: " + QFileInfo(path).fileName()
                    + "  |  " + m_project->name());
                QTimer::singleShot(100, this, [this]{ rebuildViewport(); });
            });

    // Rebuild viewport when scene changes
    connect(m_sceneTree, &SceneTree::sceneChanged, [this]{
        rebuildViewport();
        // Mark title as dirty
        QString title = windowTitle();
            setWindowTitle("* " + title);
    });

    connect(m_sceneTree, &SceneTree::nodeAdded,
            [this](const QString& name, const QString& type) {
                // Add a default node to viewport
                ViewportNode vn;
                vn.name = name;
                vn.type = type;
                vn.x    = 0;
                vn.y    = 0;
                vn.w    = (type == "CollisionShape2D") ? 64 : 48;
                vn.h    = (type == "CollisionShape2D") ? 64 : 48;
                if (type == "Camera2D" || type == "CameraNode2D") {
                    vn.camW = 800; vn.camH = 600; vn.zoom = 1.0f;
                }
                auto nodes = m_viewport->nodes();
                nodes.append(vn);
                m_viewport->setNodes(nodes);
                m_centerTabs->setCurrentWidget(m_viewport);
            });

    connect(m_viewport, &Viewport::nodeMoved,
            this, [this](const QString& name, float x, float y) {
                m_inspector->updatePosition(name, x, y);
                // Viewport gives us WORLD position; tree items store LOCAL.
                // Convert world→local before storing.
                auto* treeWidget = m_sceneTree->treeWidget();
                if (!treeWidget || !treeWidget->topLevelItemCount()) return;
                std::function<QTreeWidgetItem*(QTreeWidgetItem*, const QString&)> findItem;
                findItem = [&](QTreeWidgetItem* item, const QString& n) -> QTreeWidgetItem* {
                    if (item->data(0, Qt::UserRole+1).toString() == n) return item;
                    for (int i = 0; i < item->childCount(); i++)
                        if (auto* r = findItem(item->child(i), n)) return r;
                    return nullptr;
                };
                auto* movedItem = findItem(treeWidget->topLevelItem(0), name);
                if (!movedItem) return;
                // Compute parent's world position by walking up the tree
                float parentWorldX = 0.0f, parentWorldY = 0.0f;
                {
                    // Collect ancestors (excluding the moved item itself)
                    QList<QTreeWidgetItem*> ancestors;
                    for (auto* p = movedItem->parent(); p; p = p->parent()) {
                        QString ptype = p->data(0, Qt::UserRole).toString();
                        if (ptype != "Scene") ancestors.prepend(p);
                    }
                    // Accumulate local positions along ancestor chain
                    float accX = 0.0f, accY = 0.0f;
                    for (auto* a : ancestors) {
                        QVariant avx = a->data(0, Qt::UserRole+3);
                        QVariant avy = a->data(0, Qt::UserRole+4);
                        accX += avx.isValid() ? avx.toFloat() : 0.0f;
                        accY += avy.isValid() ? avy.toFloat() : 0.0f;
                    }
                    parentWorldX = accX;
                    parentWorldY = accY;
                }
                float localX = x - parentWorldX;
                float localY = y - parentWorldY;
                m_sceneTree->updateNodePosition(name, localX, localY);
                // Update children world positions in viewport nodes list
                auto nodes = m_viewport->nodes();
                std::function<void(QTreeWidgetItem*, float, float)> updateChildren;
                updateChildren = [&](QTreeWidgetItem* item, float pwx, float pwy) {
                    for (int i = 0; i < item->childCount(); i++) {
                        auto* child = item->child(i);
                        QString cname = child->data(0, Qt::UserRole+1).toString();
                        QVariant vx = child->data(0, Qt::UserRole+3);
                        QVariant vy = child->data(0, Qt::UserRole+4);
                        float lx = vx.isValid() ? vx.toFloat() : 0.0f;
                        float ly = vy.isValid() ? vy.toFloat() : 0.0f;
                        float wx = pwx + lx, wy = pwy + ly;
                        for (auto& vn : nodes)
                            if (vn.name == cname) { vn.x = wx; vn.y = wy; break; }
                        updateChildren(child, wx, wy);
                    }
                };
                updateChildren(movedItem, x, y);
                m_viewport->updateNodePositions(nodes);
            });

    // Write to script only when drag ends (not every frame)
    connect(m_viewport, &Viewport::nodeMovedFinal,
            this, [this](const QString& name, float x, float y) {
                QString scenePath = m_sceneTree->scenePath();
                if (scenePath.isEmpty()) return;
                // Viewport gives WORLD position; we must save LOCAL position
                // relative to parent, and write to the correct script file.
                auto* treeWidget = m_sceneTree->treeWidget();
                if (!treeWidget || !treeWidget->topLevelItemCount()) return;
                std::function<QTreeWidgetItem*(QTreeWidgetItem*, const QString&)> findItem;
                findItem = [&](QTreeWidgetItem* item, const QString& n) -> QTreeWidgetItem* {
                    if (item->data(0, Qt::UserRole+1).toString() == n) return item;
                    for (int i = 0; i < item->childCount(); i++)
                        if (auto* r = findItem(item->child(i), n)) return r;
                    return nullptr;
                };
                auto* movedItem = findItem(treeWidget->topLevelItem(0), name);
                if (!movedItem) return;
                // Compute parent's world position
                float parentWorldX = 0.0f, parentWorldY = 0.0f;
                QTreeWidgetItem* parentItem = movedItem->parent();
                {
                    QList<QTreeWidgetItem*> ancestors;
                    for (auto* p = parentItem; p; p = p->parent()) {
                        QString ptype = p->data(0, Qt::UserRole).toString();
                        if (ptype != "Scene") ancestors.prepend(p);
                    }
                    for (auto* a : ancestors) {
                        QVariant avx = a->data(0, Qt::UserRole+3);
                        QVariant avy = a->data(0, Qt::UserRole+4);
                        parentWorldX += avx.isValid() ? avx.toFloat() : 0.0f;
                        parentWorldY += avy.isValid() ? avy.toFloat() : 0.0f;
                    }
                }
                float localX = x - parentWorldX;
                float localY = y - parentWorldY;
                // Determine the correct script file to write to:
                // If the parent has a script (UserRole+2), write there; else scene file.
                QString targetScript = scenePath;
                if (parentItem) {
                    QString parentScript = parentItem->data(0, Qt::UserRole+2).toString();
                    if (!parentScript.isEmpty())
                        targetScript = parentScript;
                }
                writeInstancePosition(targetScript, name.toLower(), localX, localY);
            });

    connect(m_viewport, &Viewport::nodeSelected,
            this, [this](const QString& name){
                if (name.isEmpty()) return;
                m_selectedNode = name;
                m_sceneTree->selectNodeByName(name);
                syncInspectorPosition(name);
            });

    m_rootSplitter->setStretchFactor(0, 0);
    m_rootSplitter->setStretchFactor(1, 1);
    m_rootSplitter->setStretchFactor(2, 0);
    m_rootSplitter->setSizes({240, 900, 280});
    m_mainSplitter->addWidget(m_rootSplitter);

    // ── Bottom: Build + Output + Assets ─────────────────────────────────
    m_bottomTabs = new QTabWidget();
    m_bottomTabs->setMaximumHeight(200);
    m_buildPanel   = new BuildPanel();
    m_debugConsole = new DebugConsole();
    m_bottomTabs->addTab(m_buildPanel,   "Build");
    m_bottomTabs->addTab(m_debugConsole, "Output");

    // Assets tab — directory listing + Pack button
    {
        m_assetsTab = new QWidget();
        auto* al = new QVBoxLayout(m_assetsTab);
        al->setContentsMargins(4,4,4,4);
        al->setSpacing(4);

        auto* topRow = new QHBoxLayout();
        auto* refreshBtn = new QPushButton("Refresh");
        auto* packBtn = new QPushButton("Pack Assets (.konpak)");
        topRow->addWidget(refreshBtn);
        topRow->addWidget(packBtn);
        topRow->addStretch();
        al->addLayout(topRow);

        m_assetTree = new QTreeWidget();
        m_assetTree->setHeaderLabels({"File", "Size"});
        m_assetTree->setColumnWidth(0, 300);
        m_assetTree->setRootIsDecorated(false);
        m_assetTree->setAlternatingRowColors(true);
        al->addWidget(m_assetTree, 1);

        m_packOutput = new QTextEdit();
        m_packOutput->setReadOnly(true);
        m_packOutput->setMaximumHeight(60);
        m_packOutput->setStyleSheet("QTextEdit { background: #141414; color: #aaa; font-family: monospace; font-size: 10px; }");
        al->addWidget(m_packOutput);

        connect(refreshBtn, &QPushButton::clicked, [this]{
            if (!m_project->isOpen()) return;
            m_assetTree->clear();
            QString assetsDir = m_project->rootDir() + "/assets";
            QDir dir(assetsDir);
            if (!dir.exists()) { m_packOutput->setText("No assets/ directory found."); return; }
            QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
            for (const auto& fi : files) {
                auto* item = new QTreeWidgetItem();
                item->setText(0, fi.fileName());
                qint64 sz = fi.size();
                QString sizeStr = sz < 1024 ? QString("%1 B").arg(sz)
                    : sz < 1048576 ? QString("%1 KB").arg(sz / 1024.0, 0, 'f', 1)
                    : QString("%1 MB").arg(sz / 1048576.0, 0, 'f', 2);
                item->setText(1, sizeStr);
                m_assetTree->addTopLevelItem(item);
            }
            m_packOutput->setText(QString("Found %1 files in assets/").arg(files.size()));
        });
        connect(packBtn, &QPushButton::clicked, this, &KonEditor::onPackAssets);
        m_bottomTabs->addTab(m_assetsTab, "Assets");
    }

    m_mainSplitter->addWidget(m_bottomTabs);

    m_mainSplitter->setStretchFactor(0, 1);
    m_mainSplitter->setStretchFactor(1, 0);
    m_mainSplitter->setSizes({680, 180});

    rootLayout->addWidget(m_mainSplitter);

    // Wire asset browser → open file in script editor
    connect(m_assetBrowser, &AssetBrowser::fileDoubleClicked,
            [this](const QString& path){
                if (path.endsWith(".konscene") ||
                    (path.endsWith(".ks") && path.contains("/scenes/"))) {
                    // Load scene into scene tree
                    m_sceneTree->loadScene(path);
                    m_leftTabs->setCurrentIndex(0);
                    m_statusLabel->setText("  Scene: " + QFileInfo(path).fileName());
                } else if (path.endsWith(".anim") || path.endsWith(".konani")) {
                    // Open animation file in Animation tab
                    onOpenAnimFile(path);
                } else if (path.endsWith(".ks")) {
                    m_scriptEditor->openFile(path);
                    m_centerTabs->setCurrentWidget(m_scriptEditor);
                } else {
                    // Everything else — try to open as text
                    QStringList textExts = {
                        ".txt",".md",".json",".cfg",".ini",".konproj"
                    };
                    for (auto& ext : textExts) {
                        if (path.endsWith(ext)) {
                            m_scriptEditor->openFile(path);
                            m_centerTabs->setCurrentWidget(m_scriptEditor);
                            break;
                        }
                    }
                }
            });

    // Wire build panel game started
    connect(m_buildPanel, &BuildPanel::gameStarted,
            [this](QProcess* proc){
                m_gameProcess = proc;
                updateRunButtons(true);
                m_statusLabel->setText("  Running...");
                m_bottomTabs->setCurrentWidget(m_debugConsole);
                connect(proc, &QProcess::readyReadStandardOutput,
                        this, &KonEditor::onGameProcessOutput);
                connect(proc, &QProcess::readyReadStandardError,
                        this, &KonEditor::onGameProcessOutput);
                connect(proc,
                    static_cast<void(QProcess::*)(int,QProcess::ExitStatus)>(&QProcess::finished),
                    this, [this](int code){ onGameProcessFinished(code); });
            });
}

void KonEditor::setupStatusBar() {
    m_statusLabel = new QLabel("  No project open");
    statusBar()->addWidget(m_statusLabel);
}

void KonEditor::updateTitle() {
    setWindowTitle((m_project->isOpen() ? m_project->name() : "KonEditor") + " — KonEditor");
}

void KonEditor::updateRunButtons(bool running) {
    m_runAction->setEnabled(!running);
    m_stopAction->setEnabled(running);
}

void KonEditor::openProject(const QString& path) {
    // Detect if this is a standalone .ks file rather than a .konproj
    bool isKsFile = QFileInfo(path).suffix().toLower() == "ks";
    bool opened = isKsFile ? m_project->openKsFile(path) : m_project->open(path);
    if (!opened) {
        QMessageBox::warning(this, "Error", "Failed to open: " + path);
        return;
    }

    // For standalone .ks files, open directly in the script editor
    if (isKsFile) {
        updateTitle();
        QString dir = QFileInfo(path).absolutePath();
        m_assetBrowser->setRoot(dir);
        m_scriptEditor->setProjectRoot(dir);
        m_sceneTree->setProjectRoot(dir);
        m_statusLabel->setText("  " + m_project->name() + " (file)  |  " + dir);
        m_scriptEditor->openFile(QFileInfo(path).absoluteFilePath());
        m_centerTabs->setCurrentWidget(m_scriptEditor);
        return;
    }

    updateTitle();
    QString dir = QFileInfo(path).absolutePath();
    m_assetBrowser->setRoot(dir);
    m_scriptEditor->setProjectRoot(dir);
    m_sceneTree->setProjectRoot(dir);
    m_statusLabel->setText("  " + m_project->name() + "  |  " + dir);
    QSettings("AnoDelta","KonEditor").setValue("lastProject", path);

    // Load last scene — try .ks first, fall back to .konscene
    QString lastScene = QSettings("AnoDelta","KonEditor").value(
        "lastScene/" + path).toString();
    if (lastScene.isEmpty() || !QFile::exists(lastScene)) {
        QString ksScene   = dir + "/scenes/Main.ks";
        QString jsonScene = dir + "/scenes/Main.konscene";
        if      (QFile::exists(ksScene))   lastScene = ksScene;
        else if (QFile::exists(jsonScene)) lastScene = jsonScene;
    }
if (!lastScene.isEmpty() && QFile::exists(lastScene)) {
    // Set scene path immediately so autoSave works before timer fires
    m_sceneTree->setScenePath(lastScene);
    fprintf(stderr, "[Editor] Loading scene: %s\n", lastScene.toUtf8().constData());
    QTimer::singleShot(200, this, [this, lastScene]{
        m_sceneTree->loadScene(lastScene);
        QTimer::singleShot(100, this, [this]{ rebuildViewport(); });
    });
} else {
    fprintf(stderr, "[Editor] No scene found, creating default Main.ks\n");
    // Create default scene file
    QString scenesDir = dir + "/scenes";
    QDir().mkpath(scenesDir);
    QString defaultScene = scenesDir + "/Main.ks";
    QFile sf(defaultScene);
    if (sf.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream(&sf) << "# Main.ks\n#include <engine>\n\nnode Main : Node2D {\n    func Ready() {\n    }\n}\n";
        sf.flush(); sf.close();
    }
    m_sceneTree->setScenePath(defaultScene);
    m_sceneTree->newScene();
}
}

void KonEditor::onNewProject() {
    QString dir = QFileDialog::getExistingDirectory(this, "Choose Folder");
    if (dir.isEmpty()) return;
    if (m_project->create(dir)) {
        updateTitle();
        openProject(m_project->path());
    }
}

void KonEditor::onOpenProject() {
    QString f = QFileDialog::getOpenFileName(this, "Open Project", "",
        "KonScript Project (*.konproj)");
    if (!f.isEmpty()) openProject(f);
}

void KonEditor::onSaveProject() {
    if (!m_project->isOpen()) return;
    m_scriptEditor->saveAll();
    m_project->save();
    // Also save scene
    m_sceneTree->saveCurrentScene();
    m_statusLabel->setText("  Saved  |  " + m_project->name());
    updateTitle();  // clear dirty marker
}

void KonEditor::onProjectSettings() {
    if (!m_project->isOpen()) {
        QMessageBox::information(this, "Project Settings", "Open a project first.");
        return;
    }

    QDialog dlg(this);
    dlg.setWindowTitle("Project Settings");
    dlg.setMinimumWidth(420);
    dlg.setStyleSheet("QDialog { background: #1e1e1e; }");

    auto* layout = new QVBoxLayout(&dlg);

    // General
    auto* genGroup = new QGroupBox("General");
    auto* genForm  = new QFormLayout(genGroup);
    auto* nameEdit = new QLineEdit(m_project->name());
    auto* verEdit  = new QLineEdit(m_project->json().value("version").toString("0.1.0"));
    genForm->addRow("Project Name:", nameEdit);
    genForm->addRow("Version:", verEdit);
    layout->addWidget(genGroup);

    // Window
    auto* winGroup = new QGroupBox("Window");
    auto* winForm  = new QFormLayout(winGroup);
    auto* widthSpin  = new QSpinBox(); widthSpin->setRange(320,7680); widthSpin->setValue(m_project->json().value("width").toInt(800));
    auto* heightSpin = new QSpinBox(); heightSpin->setRange(240,4320); heightSpin->setValue(m_project->json().value("height").toInt(600));
    auto* fpsSpin    = new QSpinBox(); fpsSpin->setRange(0,999); fpsSpin->setValue(m_project->json().value("fps").toInt(60));
    fpsSpin->setToolTip("0 = uncapped (no SetTargetFPS call emitted)");
    fpsSpin->setSpecialValueText("Uncapped");
    auto* vsyncCheck = new QCheckBox(); vsyncCheck->setChecked(m_project->json().value("vsync").toBool(true));
    auto* resizableCheck = new QCheckBox(); resizableCheck->setChecked(m_project->json().value("resizable").toBool(false));
    auto* titleEdit  = new QLineEdit(m_project->json().value("windowTitle").toString(m_project->name()));
    winForm->addRow("Width:",  widthSpin);
    winForm->addRow("Height:", heightSpin);
    winForm->addRow("FPS:",    fpsSpin);
    auto* fpsHint = new QLabel("0 = uncapped framerate");
    fpsHint->setStyleSheet("QLabel { color: #666; font-size: 10px; }");
    winForm->addRow("", fpsHint);
    winForm->addRow("VSync:",  vsyncCheck);
    winForm->addRow("Resizable:", resizableCheck);
    winForm->addRow("Title:",  titleEdit);
    layout->addWidget(winGroup);

    // Debug
    auto* debugGroup = new QGroupBox("Debug");
    auto* debugForm  = new QFormLayout(debugGroup);
    auto* debugCheck = new QCheckBox("Enable debug overlays");
    debugCheck->setChecked(m_project->json().value("debug").toBool(false));
    debugForm->addRow("Debug Mode:", debugCheck);
    layout->addWidget(debugGroup);

    // Entry point
    auto* sceneGroup = new QGroupBox("Entry Point");
    auto* sceneForm  = new QFormLayout(sceneGroup);
    auto* entryEdit  = new QLineEdit(m_project->json().value("entry").toString("src/main.ks"));
    auto* browseBtn  = new QPushButton("Browse...");
    browseBtn->setFixedWidth(80);
    auto* entryRow = new QHBoxLayout();
    entryRow->addWidget(entryEdit);
    entryRow->addWidget(browseBtn);
    sceneForm->addRow("Entry file:", entryRow);
    connect(browseBtn, &QPushButton::clicked, [&]{
        QString f = QFileDialog::getOpenFileName(&dlg, "Entry File",
            m_project->rootDir(), "KonScript (*.ks);;All (*)");
        if (!f.isEmpty()) {
            entryEdit->setText(QDir(m_project->rootDir()).relativeFilePath(f));
        }
    });
    layout->addWidget(sceneGroup);

    // Scan main.ks on open to populate actual values from code
    QString mainKsPathInit = m_project->rootDir() + "/" + entryEdit->text();
    if (QFile::exists(mainKsPathInit)) {
        QFile mf(mainKsPathInit);
        if (mf.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString src = QTextStream(&mf).readAll();
            mf.close();
            // Parse InitWindow(w, h, "title") or InitWindow(w, h, "title", resizable)
            QRegularExpression reInit(R"RE(InitWindow\s*\(\s*(\d+)\s*,\s*(\d+)\s*,\s*"([^"]*)"(?:\s*,\s*(true|false))?\s*\))RE");
            auto mi = reInit.match(src);
            if (mi.hasMatch()) {
                widthSpin->setValue(mi.captured(1).toInt());
                heightSpin->setValue(mi.captured(2).toInt());
                titleEdit->setText(mi.captured(3));
                if (!mi.captured(4).isEmpty())
                    resizableCheck->setChecked(mi.captured(4) == "true");
            }
            // Parse SetTargetFPS(n)
            QRegularExpression reFps(R"(SetTargetFPS\s*\(\s*(\d+)\s*\))");
            auto mfps = reFps.match(src);
            if (mfps.hasMatch()) {
                fpsSpin->setValue(mfps.captured(1).toInt());
            } else {
                fpsSpin->setValue(0); // no SetTargetFPS = uncapped
            }
            // Parse SetVsync(true/false)
            QRegularExpression reVs(R"(SetVsync\s*\(\s*(true|false)\s*\))");
            auto mvs = reVs.match(src);
            if (mvs.hasMatch()) {
                vsyncCheck->setChecked(mvs.captured(1) == "true");
            }
            // Parse DebugMode(true/false)
            QRegularExpression reDbg(R"(DebugMode\s*\(\s*(true|false)\s*\))");
            auto mdbg = reDbg.match(src);
            if (mdbg.hasMatch()) {
                debugCheck->setChecked(mdbg.captured(1) == "true");
            }
        }
    }

    auto* btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(btns);
    connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return;

    // Save settings to project json
    auto json = m_project->json();
    json["name"]        = nameEdit->text();
    json["version"]     = verEdit->text();
    json["width"]       = widthSpin->value();
    json["height"]      = heightSpin->value();
    json["fps"]         = fpsSpin->value();
    json["vsync"]       = vsyncCheck->isChecked();
    json["resizable"]   = resizableCheck->isChecked();
    json["debug"]       = debugCheck->isChecked();
    json["windowTitle"] = titleEdit->text();
    json["entry"]       = entryEdit->text();
    m_project->setJson(json);
    m_project->save();
    updateTitle();

    // Patch main.ks
    QString mainKsPath = m_project->rootDir() + "/" + entryEdit->text();
    fprintf(stderr, "[Settings] Patching main.ks: %s\n", mainKsPath.toUtf8().constData());
    fprintf(stderr, "[Settings] exists=%d\n", QFile::exists(mainKsPath));
    if (QFile::exists(mainKsPath)) {
        QFile f(mainKsPath);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString src = f.readAll();
            f.close();
            fprintf(stderr, "[Settings] Read %d chars\n", src.size());

            // Replace InitWindow args (including resizable 4th param)
            QRegularExpression re(R"(InitWindow\s*\([^)]+\))");
            QString resArg = resizableCheck->isChecked() ? ", true" : "";
            QString newCall = QString("InitWindow(%1, %2, \"%3\"%4)")
                .arg(widthSpin->value())
                .arg(heightSpin->value())
                .arg(titleEdit->text())
                .arg(resArg);
            src.replace(re, newCall);

            // Handle SetTargetFPS: if fps=0 (uncapped), remove the line; otherwise update/add
            QRegularExpression re2(R"(\n?[ \t]*SetTargetFPS\s*\([^)]+\)\s*;?[^\n]*)");
            if (fpsSpin->value() == 0) {
                // Remove SetTargetFPS line entirely
                src.remove(re2);
            } else {
                QRegularExpression re2match(R"(SetTargetFPS\s*\([^)]+\))");
                if (re2match.match(src).hasMatch()) {
                    src.replace(re2match, QString("SetTargetFPS(%1)").arg(fpsSpin->value()));
                } else {
                    // Insert after InitWindow line
                    QRegularExpression reAfterInit(R"(InitWindow\s*\([^)]+\)[^\n]*)");
                    auto m = reAfterInit.match(src);
                    if (m.hasMatch()) {
                        int insertPos = m.capturedEnd();
                        src.insert(insertPos, QString("\n    SetTargetFPS(%1)").arg(fpsSpin->value()));
                    }
                }
            }

            // Handle SetVsync: update if exists, insert if missing and enabled
            QString vsyncVal = vsyncCheck->isChecked() ? "true" : "false";
            QRegularExpression re3(R"(SetVsync\s*\([^)]+\))");
            if (re3.match(src).hasMatch()) {
                src.replace(re3, QString("SetVsync(%1)").arg(vsyncVal));
            } else if (vsyncCheck->isChecked()) {
                // Insert SetVsync after SetTargetFPS, or after InitWindow if no SetTargetFPS
                QRegularExpression reAfterFps(R"(SetTargetFPS\s*\([^)]+\)[^\n]*)");
                auto mFps = reAfterFps.match(src);
                if (mFps.hasMatch()) {
                    int insertPos = mFps.capturedEnd();
                    src.insert(insertPos, QString("\n    SetVsync(%1)").arg(vsyncVal));
                } else {
                    QRegularExpression reAfterInit(R"(InitWindow\s*\([^)]+\)[^\n]*)");
                    auto mInit = reAfterInit.match(src);
                    if (mInit.hasMatch()) {
                        int insertPos = mInit.capturedEnd();
                        src.insert(insertPos, QString("\n    SetVsync(%1)").arg(vsyncVal));
                    }
                }
            }

            // Handle DebugMode: update if exists, insert if enabled, remove if disabled
            QRegularExpression re4(R"(\n?[ \t]*DebugMode\s*\([^)]+\)\s*;?[^\n]*)");
            QRegularExpression re4match(R"(DebugMode\s*\([^)]+\))");
            if (debugCheck->isChecked()) {
                if (re4match.match(src).hasMatch()) {
                    src.replace(re4match, "DebugMode(true)");
                } else {
                    // Insert after InitWindow line
                    QRegularExpression reAfterInit(R"(InitWindow\s*\([^)]+\)[^\n]*)");
                    auto mInit = reAfterInit.match(src);
                    if (mInit.hasMatch()) {
                        int insertPos = mInit.capturedEnd();
                        src.insert(insertPos, "\n    DebugMode(true)");
                    }
                }
            } else {
                // Remove DebugMode line if present
                src.remove(re4);
            }

            if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream ts(&f);
                ts << src;
                ts.flush();
                f.close();
                fprintf(stderr, "[Settings] Wrote %d chars to %s\n", src.size(), mainKsPath.toUtf8().constData());
            } else {
                fprintf(stderr, "[Settings] FAILED to open for writing: %s\n", mainKsPath.toUtf8().constData());
            }
        } else {
            fprintf(stderr, "[Settings] FAILED to open for reading: %s\n", mainKsPath.toUtf8().constData());
        }
        m_scriptEditor->reloadFile(mainKsPath);
        m_centerTabs->setCurrentWidget(m_scriptEditor);
    } else {
        fprintf(stderr, "[Settings] main.ks not found at: %s\n", mainKsPath.toUtf8().constData());
    }
}

void KonEditor::onBuild() {
    if (m_project->isOpen()) { m_scriptEditor->saveAll(); m_sceneTree->saveCurrentScene(); }
    if (!m_project->isOpen()) {
        m_buildPanel->appendLog("No project open.");
        m_bottomTabs->setCurrentWidget(m_buildPanel);
        return;
    }
    m_bottomTabs->setCurrentWidget(m_buildPanel);
    m_buildPanel->build(m_project->entryFile(), m_buildTarget, m_project->outDir());
}

void KonEditor::onRun() {
    if (!m_project->isOpen()) return;
    m_scriptEditor->saveAll();
    m_sceneTree->saveCurrentScene();  // save scene before run
    m_bottomTabs->setCurrentWidget(m_buildPanel);
    m_buildPanel->build(m_project->entryFile(), m_buildTarget,
                        m_project->outDir(), true);
}

void KonEditor::onStop() {
    if (m_gameProcess && m_gameProcess->state() != QProcess::NotRunning) {
        m_gameProcess->kill();
        m_debugConsole->appendOutput("[Editor] Game stopped.");
        updateRunButtons(false);
        m_statusLabel->setText("  Stopped");
    }
}

void KonEditor::onGameProcessOutput() {
    if (!m_gameProcess) return;
    QString out = m_gameProcess->readAllStandardOutput();
    QString err = m_gameProcess->readAllStandardError();
    if (!out.isEmpty()) m_debugConsole->appendOutput(out);
    if (!err.isEmpty()) m_debugConsole->appendOutput("[err] " + err);
    m_bottomTabs->setCurrentWidget(m_debugConsole);
}

void KonEditor::syncInspectorPosition(const QString& name) {
    auto* tw = m_sceneTree->treeWidget();
    if (!tw || tw->topLevelItemCount() == 0) return;
    // Iterative search — no std::function needed
    QList<QTreeWidgetItem*> stack;
    stack.append(tw->topLevelItem(0));
    while (!stack.isEmpty()) {
        auto* item = stack.takeLast();
        if (item->data(0, Qt::UserRole+1).toString() == name) {
            QVariant vx = item->data(0, Qt::UserRole+3);
            QVariant vy = item->data(0, Qt::UserRole+4);
            m_inspector->updatePosition(name,
                vx.isValid() ? vx.toFloat() : 0.0f,
                vy.isValid() ? vy.toFloat() : 0.0f);
            return;
        }
        for (int i = 0; i < item->childCount(); i++)
            stack.append(item->child(i));
    }
}

void KonEditor::rebuildViewport() {
    QList<ViewportNode> nodes;

    // Resolve the engine base type for a node.
    // Stored type may be a KonScript class name ("cam") not the engine base ("CameraNode2D").
    auto resolveBaseType = [&](const QString& type, const QString& scriptPath) -> QString {
        static const QStringList engineTypes = {
            "Node","Node2D","Sprite2D","AnimatedSprite2D",
            "KinematicBody2D","StaticBody2D","RigidBody2D",
            "Collider2D","Area2D","CameraNode2D","Camera2D",
            "AudioStreamPlayer","Timer","Label"
        };
        if (engineTypes.contains(type)) return type;
        if (scriptPath.isEmpty() || !QFile::exists(scriptPath)) return type;
        QFile f(scriptPath);
        if (!f.open(QIODevice::ReadOnly)) return type;
        QString src = QTextStream(&f).readAll();
        QRegularExpression re(R"(node\s+\w+\s*:\s*(\w+))");
        auto m = re.match(src);
        return m.hasMatch() ? m.captured(1) : type;
    };

    // Walk with parent world position so children are offset correctly
    std::function<void(QTreeWidgetItem*, float, float)> walk;
    walk = [&](QTreeWidgetItem* item, float parentWorldX, float parentWorldY) {
        if (!item) return;
        QString name   = item->data(0, Qt::UserRole + 1).toString();
        QString type   = item->data(0, Qt::UserRole).toString();
        QString script = item->data(0, Qt::UserRole + 2).toString();
        if (type == "Scene" || name.isEmpty()) {
            for (int i = 0; i < item->childCount(); i++)
                walk(item->child(i), parentWorldX, parentWorldY);
            return;
        }

        QString baseType = resolveBaseType(type, script);

        QVariant vx = item->data(0, Qt::UserRole + 3);
        QVariant vy = item->data(0, Qt::UserRole + 4);
        float localX = vx.isValid() ? vx.toFloat() : 0.0f;
        float localY = vy.isValid() ? vy.toFloat() : 0.0f;
        float worldX = parentWorldX + localX;
        float worldY = parentWorldY + localY;

        ViewportNode vn;
        vn.name = name;
        vn.type = baseType;
        vn.x    = worldX;
        vn.y    = worldY;

        // Default sizes
        vn.w = (baseType == "CameraNode2D" || baseType == "Camera2D") ? 32 : 48;
        vn.h = vn.w;

        // For colliders, read actual width/height from the script's Ready() block
        if (baseType == "Collider2D" && !script.isEmpty() && QFile::exists(script)) {
            QFile sf(script);
            if (sf.open(QIODevice::ReadOnly)) {
                QString src = QTextStream(&sf).readAll();
                QRegularExpression reReady(R"(func\s+Ready\s*\(\s*\)\s*\{)");
                auto rm = reReady.match(src);
                if (rm.hasMatch()) {
                    int start = rm.capturedEnd(), depth = 1, pos = start;
                    while (pos < src.size() && depth > 0) {
                        if (src[pos] == '{') depth++;
                        else if (src[pos] == '}') depth--;
                        if (depth > 0) pos++;
                    }
                    QString body = src.mid(start, pos - start);
                    QRegularExpression rwRe(R"(width\s*=\s*([\d.]+))");
                    QRegularExpression rhRe(R"(height\s*=\s*([\d.]+))");
                    auto mw = rwRe.match(body); auto mh = rhRe.match(body);
                    if (mw.hasMatch()) vn.w = mw.captured(1).toFloat();
                    if (mh.hasMatch()) vn.h = mh.captured(1).toFloat();
                    if (!mw.hasMatch() && !mh.hasMatch()) { vn.w = 32; vn.h = 32; }
                }
            }
        }
        if (baseType == "Camera2D" || baseType == "CameraNode2D") {
            vn.camW = m_project->isOpen() ?
                m_project->json().value("width").toInt(800) : 800;
            vn.camH = m_project->isOpen() ?
                m_project->json().value("height").toInt(600) : 600;
            vn.zoom = 1.0f;
        }
        nodes.append(vn);
        for (int i = 0; i < item->childCount(); i++)
            walk(item->child(i), worldX, worldY);
    };

    auto* sceneTree = m_sceneTree->treeWidget();
    if (sceneTree && sceneTree->topLevelItemCount() > 0)
        walk(sceneTree->topLevelItem(0), 0.0f, 0.0f);

    m_viewport->setNodes(nodes);
    m_viewport->setGameResolution(
        m_project->isOpen() ? m_project->json().value("width").toInt(800) : 800,
        m_project->isOpen() ? m_project->json().value("height").toInt(600) : 600);
    // Re-select the previously selected node so inspector edits don't lose selection
    if (!m_selectedNode.isEmpty())
        m_viewport->selectNode(m_selectedNode);
}

void KonEditor::writeInstancePosition(const QString& scenePath, const QString& varName, float x, float y) {
    QFile f(scenePath);
    if (!f.open(QIODevice::ReadOnly)) return;
    QString src = QTextStream(&f).readAll();
    f.close();

    // Resolve actual var name from scene source — display name may differ from var name.
    // e.g. display "cam" → var "cam_node" when name==type
    QString actualVar = varName;
    QRegularExpression reFind(
        QString(R"(let\s+mut\s+(\w+)\s*:\s*\w+\s*=\s*\w+\.add\s*\([^,]+,\s*"%1"\s*\))")
            .arg(QRegularExpression::escape(varName)));
    auto fm = reFind.match(src);
    if (fm.hasMatch()) actualVar = fm.captured(1);

    // Find Ready() block
    QRegularExpression reReady(R"(func\s+Ready\s*\(\s*\)\s*\{)");
    auto rm = reReady.match(src);
    if (!rm.hasMatch()) return;

    int start = rm.capturedEnd();
    int depth = 1, pos = start;
    while (pos < src.size() && depth > 0) {
        if (src[pos] == QLatin1Char('{')) depth++;
        else if (src[pos] == QLatin1Char('}')) depth--;
        if (depth > 0) pos++;
    }
    QString body = src.mid(start, pos - start);

    QRegularExpression rxX(QString(R"(\n?[ \t]*%1\.x\s*=[^;]+;)").arg(actualVar));
    QRegularExpression rxY(QString(R"(\n?[ \t]*%1\.y\s*=[^;]+;)").arg(actualVar));
    body.remove(rxX);
    body.remove(rxY);

    body.prepend(QString("\n        %1.y = %2;").arg(actualVar).arg(y, 0, 'f', 1));
    body.prepend(QString("\n        %1.x = %2;").arg(actualVar).arg(x, 0, 'f', 1));

    src.replace(start, pos - start, body);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        QTextStream(&f) << src;
}

void KonEditor::onGameProcessFinished(int code) {
    updateRunButtons(false);
    m_statusLabel->setText(QString("  Finished (exit %1)").arg(code));
    m_gameProcess = nullptr;
}

// ── Animation tab — launch KonAnimator as external process ───────────
void KonEditor::onOpenAnimFile(const QString& path) {
    m_animPlaceholder->setText(QString("Opening: %1\n\nLaunching KonAnimator...")
                               .arg(QFileInfo(path).fileName()));
    m_centerTabs->setCurrentWidget(m_animPlaceholder);

    // Try to find KonAnimator binary relative to this executable
    QString animatorPath;
    QStringList candidates = {
        QApplication::applicationDirPath() + "/KonAnimator",
        QApplication::applicationDirPath() + "/../KonAnimator/build/KonAnimator",
        QApplication::applicationDirPath() + "/../../KonAnimator/build/KonAnimator",
    };
    for (const auto& c : candidates) {
        if (QFile::exists(c)) { animatorPath = c; break; }
    }

    if (animatorPath.isEmpty()) {
        m_animPlaceholder->setText(
            QString("Animation file: %1\n\n"
                    "KonAnimator not found.\n"
                    "Build KonAnimator and place it alongside KonEditor,\n"
                    "or open this file directly with KonAnimator.")
            .arg(QFileInfo(path).fileName()));
        m_statusLabel->setText("  KonAnimator not found");
        return;
    }

    auto* proc = new QProcess(this);
    proc->setProgram(animatorPath);
    proc->setArguments({path});
    connect(proc, static_cast<void(QProcess::*)(int,QProcess::ExitStatus)>(&QProcess::finished),
            this, [this, proc, path](int code, QProcess::ExitStatus) {
                m_animPlaceholder->setText(
                    QString("KonAnimator closed (exit %1).\n\n"
                            "Open an .anim file from the file browser to edit animations.")
                    .arg(code));
                m_statusLabel->setText(QString("  KonAnimator exited (%1)").arg(code));
                proc->deleteLater();
            });
    proc->start();
    if (!proc->waitForStarted(3000)) {
        m_animPlaceholder->setText(
            QString("Failed to launch KonAnimator.\n\n"
                    "Path: %1\nFile: %2")
            .arg(animatorPath, path));
        proc->deleteLater();
    } else {
        m_animPlaceholder->setText(
            QString("Editing: %1\n\nKonAnimator is running in a separate window.\n"
                    "Close it to return here.")
            .arg(QFileInfo(path).fileName()));
        m_statusLabel->setText("  KonAnimator running — " + QFileInfo(path).fileName());
    }
}

// ── Assets tab — run konpak to create .konpak ────────────────────────
void KonEditor::onPackAssets() {
    if (!m_project->isOpen()) {
        m_packOutput->setText("No project open.");
        return;
    }

    QString assetsDir = m_project->rootDir() + "/assets";
    if (!QDir(assetsDir).exists()) {
        m_packOutput->setText("No assets/ directory found in project root.");
        return;
    }

    // Try to find konpak binary
    QString konpakPath;
    QStringList candidates = {
        QApplication::applicationDirPath() + "/konpak",
        QApplication::applicationDirPath() + "/../KonPaktor/build/konpak",
        QApplication::applicationDirPath() + "/../../KonPaktor/build/konpak",
    };
    for (const auto& c : candidates) {
        if (QFile::exists(c)) { konpakPath = c; break; }
    }

    if (konpakPath.isEmpty()) {
        m_packOutput->setText("konpak not found. Build KonPaktor first.");
        return;
    }

    QString outFile = m_project->rootDir() + "/" + m_project->name() + ".konpak";
    m_packOutput->setText("Packing assets...\n");
    m_bottomTabs->setCurrentWidget(m_assetsTab);

    m_packProcess = new QProcess(this);
    m_packProcess->setProgram(konpakPath);
    m_packProcess->setArguments({assetsDir, outFile});
    m_packProcess->setWorkingDirectory(m_project->rootDir());
    connect(m_packProcess, &QProcess::readyReadStandardOutput, [this]{
        m_packOutput->append(m_packProcess->readAllStandardOutput());
    });
    connect(m_packProcess, &QProcess::readyReadStandardError, [this]{
        m_packOutput->append("[err] " + m_packProcess->readAllStandardError());
    });
    connect(m_packProcess,
        static_cast<void(QProcess::*)(int,QProcess::ExitStatus)>(&QProcess::finished),
        this, [this, outFile](int code, QProcess::ExitStatus) {
            if (code == 0) {
                QFileInfo fi(outFile);
                qint64 sz = fi.size();
                QString sizeStr = sz < 1048576
                    ? QString("%1 KB").arg(sz / 1024.0, 0, 'f', 1)
                    : QString("%1 MB").arg(sz / 1048576.0, 0, 'f', 2);
                m_packOutput->append(QString("\nDone! Created %1 (%2)")
                                     .arg(fi.fileName(), sizeStr));
                m_statusLabel->setText("  Packed: " + fi.fileName());
            } else {
                m_packOutput->append(QString("\nkonpak exited with code %1").arg(code));
                m_statusLabel->setText("  Pack failed");
            }
            m_packProcess->deleteLater();
            m_packProcess = nullptr;
        });
    m_packProcess->start();
}
