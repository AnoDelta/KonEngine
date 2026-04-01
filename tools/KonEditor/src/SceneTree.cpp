#include "ScriptDiffDialog.hpp"
#include <QFileDialog>
#include "SceneTree.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QInputDialog>
#include <QDialogButtonBox>
#include "KonScriptEditor.hpp"
#include <QMessageBox>
#include <QLineEdit>
#include <QFile>
#include <QJsonDocument>
#include <QDialog>
#include <QListWidget>
#include <QGroupBox>

// ── Node type registry ────────────────────────────────────────────────────
struct NodeTypeInfo {
    QString type;
    QString category;
    QString icon;
    QString desc;
};

static const QList<NodeTypeInfo> NODE_TYPES = {
    // Base
    { "Node",          "Base",       "📦", "Base node — parent for anything" },
    { "Node2D",        "Base",       "📍", "2D transform node" },

    // Rendering
    { "CameraNode2D",  "Rendering",  "📷", "2D camera — auto-activates in scene" },
    { "Sprite2D",      "Rendering",  "🖼", "Displays a texture in 2D" },
    { "AnimatedSprite2D", "Rendering","🎞", "Sprite with animation frames" },
    { "Label",         "Rendering",  "🔤", "Draws text in the scene" },
    { "TileMap",       "Rendering",  "🗺", "Grid of tiles for levels" },

    // Physics
    { "RigidBody2D",   "Physics",    "⚡", "Physics-driven body (gravity, forces)" },
    { "StaticBody2D",  "Physics",    "🧱", "Immovable physics body (walls, floors)" },
    { "KinematicBody2D","Physics",   "🏃", "Script-controlled body (player, enemies)" },
    { "Area2D",        "Physics",    "🔵", "Detects overlaps, no physics response" },
    { "CollisionShape2D","Physics",  "⬜", "Defines collision shape for a body" },

    // Joints
    { "PinJoint2D",    "Physics",    "📌", "Pins two bodies at a point" },
    { "SpringJoint2D", "Physics",    "🌀", "Spring constraint between bodies" },

    // Audio
    { "AudioPlayer",   "Audio",      "🔊", "Plays sound effects or music" },

    // Logic
    { "Timer",         "Logic",      "⏱", "Fires signal after a duration" },
    { "Tween",         "Logic",      "🎢", "Animates properties over time" },
    { "ScriptNode",    "Logic",      "📝", "Node with attached KonScript" },

    // Light
    { "PointLight2D",  "Light",      "💡", "Emits light in all directions" },
    { "DirectionalLight2D","Light",  "☀", "Parallel light (sun-like)" },
};

// ── Node picker dialog ────────────────────────────────────────────────────
class NodePickerDialog : public QDialog {
public:
    explicit NodePickerDialog(QWidget* parent = nullptr) : QDialog(parent) {
        setWindowTitle("Add Node");
        setMinimumSize(420, 500);
        setStyleSheet(R"(
            QDialog { background: #1e1e1e; }
            QListWidget { background: #141414; color: #ddd; border: none; font-size: 12px; }
            QListWidget::item { padding: 7px 10px; }
            QListWidget::item:selected { background: #0078d7; color: #fff; }
            QListWidget::item:hover { background: #252525; }
            QGroupBox { color: #888; font-size: 10px; font-weight: bold;
                        border: 1px solid #333; margin-top: 6px; padding-top: 4px; }
            QLineEdit { background: #252525; color: #ddd; border: 1px solid #444;
                        padding: 5px; border-radius: 3px; font-size: 12px; }
            QLabel { color: #888; font-size: 11px; }
            QPushButton { background: #3a3a3a; color: #ddd; border: 1px solid #555;
                          padding: 6px 16px; border-radius: 3px; }
            QPushButton:hover { background: #4a4a4a; }
            QPushButton[text="Create"] { background: #0078d7; color: #fff;
                                         border-color: #0078d7; }
        )");

        auto* layout = new QVBoxLayout(this);
        layout->setSpacing(8);

        // Search
        m_search = new QLineEdit();
        m_search->setPlaceholderText("Search node types...");
        layout->addWidget(m_search);

        m_list = new QListWidget();
        layout->addWidget(m_list);

        m_desc = new QLabel("Select a node type");
        m_desc->setWordWrap(true);
        layout->addWidget(m_desc);

        auto* btns = new QDialogButtonBox();
        auto* createBtn = btns->addButton("Create", QDialogButtonBox::AcceptRole);
        btns->addButton("Cancel", QDialogButtonBox::RejectRole);
        createBtn->setProperty("text", "Create");
        layout->addWidget(btns);

        connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
        connect(m_search, &QLineEdit::textChanged, this, &NodePickerDialog::filter);
        connect(m_list, &QListWidget::currentRowChanged, [this](int row) {
            if (row < 0 || row >= m_filtered.size()) return;
            m_desc->setText("<b>" + m_filtered[row].type + "</b> — " + m_filtered[row].desc);
        });
        connect(m_list, &QListWidget::itemDoubleClicked, this, &QDialog::accept);

        populateAll();
    }

    QString selectedType() const {
        int row = m_list->currentRow();
        if (row < 0 || row >= m_filtered.size()) return "";
        return m_filtered[row].type;
    }

private:
    void populateAll() {
        m_filtered.clear();
        m_list->clear();

        QString cur_cat;
        for (auto& n : NODE_TYPES) {
            if (n.category != cur_cat) {
                cur_cat = n.category;
                auto* sep = new QListWidgetItem("— " + cur_cat + " —");
                sep->setFlags(Qt::NoItemFlags);
                sep->setForeground(QColor("#555"));
                m_list->addItem(sep);
                m_filtered.append(NodeTypeInfo{}); // placeholder for separator
            }
            auto* item = new QListWidgetItem(n.icon + "  " + n.type);
            item->setToolTip(n.desc);
            m_list->addItem(item);
            m_filtered.append(n);
        }
    }

    void filter(const QString& text) {
        m_filtered.clear();
        m_list->clear();
        if (text.trimmed().isEmpty()) { populateAll(); return; }
        for (auto& n : NODE_TYPES) {
            if (n.type.contains(text, Qt::CaseInsensitive) ||
                n.desc.contains(text, Qt::CaseInsensitive)) {
                auto* item = new QListWidgetItem(n.icon + "  " + n.type);
                m_list->addItem(item);
                m_filtered.append(n);
            }
        }
        if (m_list->count() > 0) m_list->setCurrentRow(0);
    }

    QLineEdit*           m_search;
    QListWidget*         m_list;
    QLabel*              m_desc;
    QList<NodeTypeInfo>  m_filtered;
};

// ── SceneTree ─────────────────────────────────────────────────────────────
SceneTree::SceneTree(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Header
    auto* header = new QLabel("  Scene");
    header->setStyleSheet("background: #252525; color: #aaa; padding: 5px 8px; "
                          "font-size: 11px; font-weight: bold;");
    layout->addWidget(header);

    // Toolbar
    auto* bar = new QHBoxLayout();
    bar->setContentsMargins(6, 3, 6, 3);
    bar->setSpacing(4);

    auto* addBtn = new QPushButton("+ Node");
    auto* delBtn = new QPushButton("✕");
    addBtn->setFixedHeight(22);
    delBtn->setFixedHeight(22);
    delBtn->setFixedWidth(28);
    delBtn->setToolTip("Delete selected node");
    delBtn->setStyleSheet("color: #f44336;");
    bar->addWidget(addBtn);
    bar->addStretch();
    bar->addWidget(delBtn);
    layout->addLayout(bar);

    // Tree
    m_tree = new QTreeWidget();
    m_tree->setHeaderHidden(true);
    m_tree->setIndentation(16);
    m_tree->setAnimated(true);
    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_tree->setStyleSheet(
        "QTreeWidget { background: #1a1a1a; color: #ddd; border: none; }"
        "QTreeWidget::item { padding: 3px 2px; }"
        "QTreeWidget::item:selected { background: #0078d7; }"
        "QTreeWidget::item:hover { background: #2a2a2a; }");
    layout->addWidget(m_tree);

    connect(addBtn, &QPushButton::clicked, this, &SceneTree::onAddNode);
    connect(delBtn, &QPushButton::clicked, this, &SceneTree::onDeleteNode);
    connect(m_tree, &QTreeWidget::itemClicked, this, &SceneTree::onItemClicked);
    connect(m_tree, &QTreeWidget::itemDoubleClicked,
            [this](QTreeWidgetItem* item, int) {
                if (!item->parent()) return; // don't rename root
                bool ok;
                QString oldName = item->data(0, Qt::UserRole + 1).toString();
                QString newName = QInputDialog::getText(
                    this, "Rename Node", "New name:",
                    QLineEdit::Normal, oldName, &ok);
                if (!ok || newName.trimmed().isEmpty() || newName == oldName) return;
                QString icon = item->text(0).left(4); // keep icon
                item->setText(0, icon + "  " + newName);
                item->setData(0, Qt::UserRole + 1, newName);
                autoSaveScene();
                emit sceneChanged();
            });
    connect(m_tree, &QTreeWidget::customContextMenuRequested,
            this, &SceneTree::onContextMenu);

    // Start with empty scene
    newScene();
}

void SceneTree::setProjectRoot(const QString& root) {
    m_projectRoot = root;
    if (!root.isEmpty() && m_scenePath.isEmpty()) {
        QString ks = root + "/scenes/Main.ks";
        if (QFile::exists(ks)) m_scenePath = ks;
    }
}

void SceneTree::newScene() {
    m_loading = true;  // prevent autoSave during init
    m_tree->clear();
    auto* root = new QTreeWidgetItem(m_tree, {"🎬  Main"});
    root->setData(0, Qt::UserRole, "Scene");
    root->setData(0, Qt::UserRole + 1, "Main");
    root->setExpanded(true);
    m_sceneLoaded = true;
    m_loading = false;
    // Don't emit sceneChanged here — no need to save a blank scene
}

QTreeWidgetItem* SceneTree::addNode(QTreeWidgetItem* parent,
                                    const QString& name, const QString& type) {
    // Find icon
    QString icon = "📦";
    for (auto& n : NODE_TYPES)
        if (n.type == type) { icon = n.icon; break; }

    auto* item = new QTreeWidgetItem(parent, {icon + "  " + name});
    item->setData(0, Qt::UserRole, type);
    item->setData(0, Qt::UserRole + 1, name);
    item->setToolTip(0, type);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    parent->setExpanded(true);
    return item;
}

void SceneTree::onAddNode() {
    // Need a scene root first
    if (m_tree->topLevelItemCount() == 0) newScene();

    NodePickerDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;
    QString type = dlg.selectedType();
    if (type.isEmpty()) return;

    // Ask for name
    bool ok;
    QString name = QInputDialog::getText(this, "Node Name", "Name:",
        QLineEdit::Normal, type, &ok);
    if (!ok || name.trimmed().isEmpty()) return;
    // Sanitize: replace spaces/special chars with underscores
    name = name.trimmed().replace(' ', '_').replace('-', '_')
               .replace('.', '_').replace('/', '_');

    // Add under selected item, or root if nothing selected
    QTreeWidgetItem* parent = m_tree->currentItem();
    if (!parent) parent = m_tree->topLevelItem(0);

    auto* newNode = addNode(parent, name, type);

    // Auto-generate a starter .ks script file for this node
    if (!m_projectRoot.isEmpty()) {
        QString scriptName = name.toLower() + ".ks";
        QString scriptPath = m_projectRoot + "/src/" + scriptName;
        if (!QFile::exists(scriptPath)) {
            ScriptAnalyzer sa;
            QFile f(scriptPath);
            if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream(&f) << sa.generateScript(name, type);
                // Store script ref on node
                newNode->setData(0, Qt::UserRole + 2, scriptPath);
                newNode->setToolTip(0, type + " — script: src/" + scriptName);
            }
        }
        // Auto-save scene
        autoSaveScene();
    }

    // Auto-select the new node so inspector shows it immediately
    m_tree->setCurrentItem(newNode);
    onItemClicked(newNode);

    // Auto-select the new node so inspector shows it immediately
    m_tree->setCurrentItem(newNode);
    onItemClicked(newNode);

    emit nodeAdded(name, type);
    emit sceneChanged();
}

void SceneTree::onDeleteNode() {
    auto* item = m_tree->currentItem();
    if (!item || !item->parent()) return; // don't delete root
    QString nodeName = item->data(0, Qt::UserRole + 1).toString();
    QMessageBox::StandardButton r = QMessageBox::question(
        this, "Delete Node",
        "Delete \"" + nodeName + "\" and all children?",
        QMessageBox::Yes | QMessageBox::Cancel);
    if (r != QMessageBox::Yes) return;
    delete item;
    // Remove from scene file
    if (!m_scenePath.isEmpty() && QFile::exists(m_scenePath)) {
        QFile f(m_scenePath);
        if (f.open(QIODevice::ReadOnly)) {
            QString src = QTextStream(&f).readAll();
            f.close();
            QString var = nodeName.toLower();
            // Remove: let mut var: Type = this.add(...);
            QRegularExpression reLine(
                QString(R"(
[ 	]*let\s+mut\s+%1\s*:[^;]+;)").arg(
                    QRegularExpression::escape(var)));
            src.remove(reLine);
            // Remove: var.x = ...; var.y = ...;
            QRegularExpression rePos(
                QString(R"(
[ 	]*%1\.[xy]\s*=[^;]+;)").arg(
                    QRegularExpression::escape(var)));
            src.remove(rePos);
            if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                QTextStream(&f) << src;
                f.flush(); f.close();
            }
        }
    }
    emit sceneChanged();
}

void SceneTree::onItemClicked(QTreeWidgetItem* item) {
    QString name   = item->data(0, Qt::UserRole + 1).toString();
    QString type   = item->data(0, Qt::UserRole).toString();
    QString script = item->data(0, Qt::UserRole + 2).toString();
    emit nodeSelected(name, type);
    emit nodeSelectedWithScript(name, type, script);
}

void SceneTree::onContextMenu(const QPoint& pos) {
    auto* item = m_tree->itemAt(pos);
    QMenu menu(this);
    menu.setStyleSheet("QMenu { background: #2a2a2a; color: #ddd; border: 1px solid #444; }"
                       "QMenu::item:selected { background: #0078d7; }");
    auto* addAct = menu.addAction("+ Add Child Node");
    menu.addSeparator();
    auto* dupAct = menu.addAction("Duplicate");
    auto* delAct = menu.addAction("Delete");
    if (!item || !item->parent()) {
        delAct->setEnabled(false);
        dupAct->setEnabled(false);
    }

    auto* attachAct  = menu.addAction("Attach Script");
    menu.addSeparator();
    auto* viewTextAct = menu.addAction("View Scene as Text");
    if (!item) attachAct->setEnabled(false);

    auto* chosen = menu.exec(m_tree->mapToGlobal(pos));
    if (chosen == addAct) {
        if (item) m_tree->setCurrentItem(item);
        onAddNode();
    } else if (chosen == delAct) {
        m_tree->setCurrentItem(item);
        onDeleteNode();
    } else if (chosen == attachAct && item) {
        onAttachScript();
    } else if (chosen == viewTextAct) {
        onViewAsText();
    } else if (chosen == dupAct && item) {
        // Simple duplicate
        auto* parent = item->parent();
        if (parent) {
            QString name = item->data(0, Qt::UserRole + 1).toString() + "_copy";
            QString type = item->data(0, Qt::UserRole).toString();
            addNode(parent, name, type);
            emit sceneChanged();
        }
    }
}

void SceneTree::attachScriptToSelected() { onAttachScript(); }

void SceneTree::onViewAsText() {
    autoSaveScene();
    if (m_scenePath.isEmpty()) {
        QMessageBox::information(this, "View as Text", "Save the scene first.");
        return;
    }
    QFile f(m_scenePath);
    if (!f.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Error", "Could not open: " + m_scenePath);
        return;
    }
    QString text = QTextStream(&f).readAll();
    f.close();

    // Use ScriptEditor widget directly — it already has QPlainTextEdit
    // Fall back to a simple KonScriptEditor inside a dialog
    auto* dlg    = new QDialog(this);
    auto* layout = new QVBoxLayout(dlg);
    auto* editor = new KonScriptEditor(dlg);
    editor->setPlainText(text);
    editor->setReadOnly(false);
    layout->addWidget(editor);
    layout->setContentsMargins(0,0,0,4);

    dlg->setWindowTitle("Scene: " + QFileInfo(m_scenePath).fileName());
    dlg->resize(640, 520);

    auto* bar    = new QHBoxLayout();
    auto* saveBtn  = new QPushButton("Save & Reload");
    auto* closeBtn = new QPushButton("Close");
    bar->addStretch();
    bar->addWidget(saveBtn);
    bar->addWidget(closeBtn);
    layout->addLayout(bar);

    QString sp = m_scenePath;
    connect(saveBtn, &QPushButton::clicked, [this, editor, sp, dlg]{
        QFile f2(sp);
        if (f2.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream(&f2) << editor->toPlainText();
            f2.close();
        }
        loadScene(sp);
        dlg->close();
    });
    connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::close);
    dlg->show();
}

void SceneTree::selectNodeByName(const QString& name) {
    QList<QTreeWidgetItem*> stack;
    if (m_tree->topLevelItemCount() == 0) return;
    stack.append(m_tree->topLevelItem(0));
    while (!stack.isEmpty()) {
        auto* item = stack.takeLast();
        if (item->data(0, Qt::UserRole+1).toString() == name) {
            m_tree->setCurrentItem(item);
            onItemClicked(item);
            return;
        }
        for (int i = 0; i < item->childCount(); i++)
            stack.append(item->child(i));
    }
}

void SceneTree::updateNodePosition(const QString& name, float x, float y) {
    // Find node in tree and store position as UserRole+3/4
    std::function<QTreeWidgetItem*(QTreeWidgetItem*)> find;
    find = [&](QTreeWidgetItem* item) -> QTreeWidgetItem* {
        if (item->data(0, Qt::UserRole + 1).toString() == name) return item;
        for (int i = 0; i < item->childCount(); i++) {
            auto* r = find(item->child(i));
            if (r) return r;
        }
        return nullptr;
    };
    if (m_tree->topLevelItemCount() == 0) return;
    auto* found = find(m_tree->topLevelItem(0));
    if (found) {
        found->setData(0, Qt::UserRole + 3, x);
        found->setData(0, Qt::UserRole + 4, y);
        autoSaveScene();
    }
}

void SceneTree::autoSaveScene() {
    fprintf(stderr, "[SceneTree] autoSave: loading=%d path=%s\n", m_loading, m_scenePath.toUtf8().constData());
    if (m_loading) return;
    if (m_scenePath.isEmpty()) return;
    auto* root = m_tree->topLevelItem(0);
    if (!root || root->childCount() == 0) return;
    saveScene(m_scenePath);
}

void SceneTree::saveCurrentScene() {
    if (m_scenePath.isEmpty()) return;
    auto* root = m_tree->topLevelItem(0);
    if (!root || root->childCount() == 0) return;
    saveScene(m_scenePath);
}

void SceneTree::onAttachScript() {
    auto* item = m_tree->currentItem();
    if (!item) return;
    QString nodeName = item->data(0, Qt::UserRole + 1).toString();
    QString nodeType = item->data(0, Qt::UserRole).toString();

    // Suggest a script path
    QString suggested = m_projectRoot.isEmpty() ? nodeName.toLower() + ".ks"
        : m_projectRoot + "/src/" + nodeName.toLower() + ".ks";

    QString path = QFileDialog::getSaveFileName(
        this, "Attach Script", suggested, "KonScript (*.ks)");
    if (path.isEmpty()) return;

    // Generate starter script if file doesn't exist
    ScriptAnalyzer sa;
    if (!QFile::exists(path)) {
        QFile f(path);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text))
            QTextStream(&f) << sa.generateScript(nodeName, nodeType);
    }

    // Store script path on node
    item->setData(0, Qt::UserRole + 2, path);

    // Update display
    QString rel = m_projectRoot.isEmpty() ? path
        : QDir(m_projectRoot).relativeFilePath(path);
    item->setToolTip(0, nodeType + " — script: " + rel);

    emit scriptRequested(nodeName, path);
    emit sceneChanged();
}

void SceneTree::loadScene(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return;
    QByteArray data = f.readAll();
    f.close();

    m_loading = true;
    m_tree->clear();
    m_scenePath = path;

    // Try JSON first (legacy .konscene format)
    auto doc = QJsonDocument::fromJson(data);
    if (doc.isObject()) {
        auto root = doc.object();
        QString rootName = root.value("name").toString("Scene");
        if (rootName.isEmpty()) rootName = "Scene";
        auto* rootItem = new QTreeWidgetItem(m_tree, {"🎬  " + rootName});
        rootItem->setData(0, Qt::UserRole,     "Scene");
        rootItem->setData(0, Qt::UserRole + 1, rootName);
        rootItem->setExpanded(true);
        jsonToTree(rootItem, root.value("children").toArray());
        m_sceneLoaded = true;
        emit sceneLoaded(path);
        return;
    }

    // Parse .ks scene file
    QString src = QString::fromUtf8(data);

    // Find root node name: node NAME : SomeBase {
    QRegularExpression reNode(R"(node\s+(\w+)\s*:\s*\w+\s*\{)");
    auto mNode = reNode.match(src);
    QString rootName = mNode.hasMatch() ? mNode.captured(1) : "Main";

    auto* rootItem = new QTreeWidgetItem(m_tree, {"🎬  " + rootName});
    rootItem->setData(0, Qt::UserRole,     "Scene");
    rootItem->setData(0, Qt::UserRole + 1, rootName);
    rootItem->setExpanded(true);

    // Find child nodes: let mut varname: Type = this.add(...)
    QRegularExpression reAdd(R"(let\s+mut\s+(\w+)\s*:\s*(\w+)\s*=\s*this\.add\([^)]+\))");
    auto it = reAdd.globalMatch(src);
    while (it.hasNext()) {
        auto m = it.next();
        QString varName  = m.captured(1);
        QString typeName = m.captured(2);

        auto* child = addNode(rootItem, varName, typeName);

        // Find script — try varName first, then typeName
        if (!m_projectRoot.isEmpty()) {
            QString byVar  = m_projectRoot + "/src/" + varName.toLower()  + ".ks";
            QString byType = m_projectRoot + "/src/" + typeName.toLower() + ".ks";
            if      (QFile::exists(byVar))  child->setData(0, Qt::UserRole + 2, byVar);
            else if (QFile::exists(byType)) child->setData(0, Qt::UserRole + 2, byType);
        }

        // Parse x/y from Ready(): varname.x = 0.0;
        QRegularExpression rxX(QString(R"(%1\.x\s*=\s*([\d.+\-]+))").arg(varName));
        QRegularExpression rxY(QString(R"(%1\.y\s*=\s*([\d.+\-]+))").arg(varName));
        auto mx = rxX.match(src);
        auto my = rxY.match(src);
        if (mx.hasMatch()) child->setData(0, Qt::UserRole + 3, mx.captured(1).toFloat());
        if (my.hasMatch()) child->setData(0, Qt::UserRole + 4, my.captured(1).toFloat());
    }

    rootItem->setExpanded(true);
    m_sceneLoaded = true;
    m_loading = false;
    emit sceneLoaded(path);
}


void SceneTree::saveScene(const QString& path) {
    auto* root = m_tree->topLevelItem(0);
    if (!root) return;

    // Generate .ks scene file
    QString sceneName = root->data(0, Qt::UserRole + 1).toString();
    if (sceneName.isEmpty()) sceneName = "Main";

    QString ks;
    ks += "# " + sceneName + ".ks — generated by KonEditor\n";
    ks += "#include <engine>\n";

    // Collect includes from child scripts
    QStringList includes;
    std::function<void(QTreeWidgetItem*)> collectIncludes;
    collectIncludes = [&](QTreeWidgetItem* item) {
        QString script = item->data(0, Qt::UserRole + 2).toString();
        if (!script.isEmpty()) {
            QString rel = QDir(m_projectRoot).relativeFilePath(script);
            if (!includes.contains(rel)) includes.append(rel);
        }
        for (int i = 0; i < item->childCount(); i++)
            collectIncludes(item->child(i));
    };
    for (int i = 0; i < root->childCount(); i++)
        collectIncludes(root->child(i));
    // Make includes relative to the scene file location, not project root
    QString sceneDir = QFileInfo(path).absolutePath();
    for (auto& inc : includes) {
        // inc is already relative to project root — make it relative to scene dir
        QString absInc = QDir(m_projectRoot).absoluteFilePath(inc);
        QString relToScene = QDir(sceneDir).relativeFilePath(absInc);
        ks += "#include \"" + relToScene + "\"\n";
    }
    ks += "\n";

    // Write node class
    ks += "node " + sceneName + " : Node2D {\n";

    static const QStringList builtinTypes = {
        "Node","Node2D","Sprite2D","AnimatedSprite2D",
        "KinematicBody2D","StaticBody2D","RigidBody2D",
        "Collider2D","CollisionShape2D","Area2D",
        "CameraNode2D","AudioStreamPlayer","Timer","Label"
    };

    // Write field declarations for each child node
    std::function<void(QTreeWidgetItem*, int)> writeNode;
    writeNode = [&](QTreeWidgetItem* item, int depth) {
        QString name = item->data(0, Qt::UserRole + 1).toString();
        QString type = item->data(0, Qt::UserRole).toString();
        if (type == "Scene" || name.isEmpty()) {
            for (int i = 0; i < item->childCount(); i++)
                writeNode(item->child(i), depth);
            return;
        }
        // Auto-include script for user-defined types
        QString script = item->data(0, Qt::UserRole + 2).toString();
        if (script.isEmpty() && !builtinTypes.contains(type)) {
            QString guessed = m_projectRoot + "/src/" + type.toLower() + ".ks";
            if (QFile::exists(guessed)) script = guessed;
        }
        if (!script.isEmpty()) {
            QString rel = QDir(m_projectRoot).relativeFilePath(script);
            if (!includes.contains(rel)) includes.append(rel);
        }
        QString indent(depth * 4, ' ');
        // If node has a script, use the script's node name as type
        QString useType = type;
        if (!script.isEmpty()) {
            QFile sf(script);
            if (sf.open(QIODevice::ReadOnly)) {
                QString src = QTextStream(&sf).readAll();
                QRegularExpression re(R"(node\s+(\w+)\s*:)");
                auto m = re.match(src);
                if (m.hasMatch()) useType = m.captured(1);
            }
        }
        // Avoid var name == type name (C++ conflict)
        QString varName = name.toLower();
        if (varName == useType.toLower()) varName += "_node";
        ks += indent + "let mut " + varName + ": " + useType +
              " = this.add(" + useType + ", \"" + name + "\");\n";
        for (int i = 0; i < item->childCount(); i++)
            writeNode(item->child(i), depth);
    };
    for (int i = 0; i < root->childCount(); i++)
        writeNode(root->child(i), 1);

    ks += "\n    func Ready() {\n";

    // Write positions for each child
    std::function<void(QTreeWidgetItem*)> writePositions;
    writePositions = [&](QTreeWidgetItem* item) {
        QString name = item->data(0, Qt::UserRole + 1).toString();
        QString type = item->data(0, Qt::UserRole).toString();
        if (type == "Scene" || name.isEmpty()) {
            for (int i = 0; i < item->childCount(); i++)
                writePositions(item->child(i));
            return;
        }
        QVariant vx = item->data(0, Qt::UserRole + 3);
        QVariant vy = item->data(0, Qt::UserRole + 4);
        float x = vx.isValid() ? vx.toFloat() : 0.0f;
        float y = vy.isValid() ? vy.toFloat() : 0.0f;
        QString varName = name.toLower();
        QString typeName2 = item->data(0, Qt::UserRole).toString().toLower();
        if (varName == typeName2) varName += "_node";
        ks += "        " + varName + ".x = " + QString::number(x, 'f', 1) + ";\n";
        ks += "        " + varName + ".y = " + QString::number(y, 'f', 1) + ";\n";
        (void)typeName2; // suppress warning
        for (int i = 0; i < item->childCount(); i++)
            writePositions(item->child(i));
    };
    for (int i = 0; i < root->childCount(); i++)
        writePositions(root->child(i));

    ks += "    }\n}\n";

    QFile f(path);
    QDir().mkpath(QFileInfo(path).absolutePath());
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream(&f) << ks;
        f.flush();
        f.close();
    }
    m_scenePath = path;
}

QJsonObject SceneTree::treeToJson(QTreeWidgetItem* item) const {
    QJsonObject obj;
    obj["name"]   = item->data(0, Qt::UserRole + 1).toString();
    obj["type"]   = item->data(0, Qt::UserRole).toString();
    QString script = item->data(0, Qt::UserRole + 2).toString();
    if (!script.isEmpty()) obj["script"] = script;
    QVariant vx = item->data(0, Qt::UserRole + 3);
    QVariant vy = item->data(0, Qt::UserRole + 4);
    if (vx.isValid()) obj["x"] = vx.toDouble();
    if (vy.isValid()) obj["y"] = vy.toDouble();
    QJsonArray children;
    for (int i = 0; i < item->childCount(); i++)
        children.append(treeToJson(item->child(i)));
    if (!children.isEmpty()) obj["children"] = children;
    return obj;
}

void SceneTree::jsonToTree(QTreeWidgetItem* parent, const QJsonArray& nodes) {
    for (auto v : nodes) {
        auto o = v.toObject();
        auto* item = addNode(parent,
            o.value("name").toString("Node"),
            o.value("type").toString("Node"));
        // Restore script path
        QString script = o.value("script").toString();
        if (!script.isEmpty()) {
            item->setData(0, Qt::UserRole + 2, script);
            item->setToolTip(0, o.value("type").toString() +
                             " — script: " + QFileInfo(script).fileName());
        }
        if (o.contains("x")) item->setData(0, Qt::UserRole + 3, o["x"].toDouble());
        if (o.contains("y")) item->setData(0, Qt::UserRole + 4, o["y"].toDouble());
        jsonToTree(item, o.value("children").toArray());
    }
}
