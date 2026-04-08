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
#include <QSet>
#include <QRegularExpression>

// ── Node type registry ────────────────────────────────────────────────────
struct NodeTypeInfo {
    QString type;
    QString category;
    QString icon;
    QString desc;
};

static const QList<NodeTypeInfo> NODE_TYPES = {
    { "Node",              "Base",       "📦", "Base node — parent for anything" },
    { "Node2D",            "Base",       "📍", "2D transform node" },
    { "CameraNode2D",      "Rendering",  "📷", "2D camera — auto-activates in scene" },
    { "Sprite2D",          "Rendering",  "🖼", "Displays a texture in 2D" },
    { "AnimatedSprite2D",  "Rendering",  "🎞", "Sprite with animation frames" },
    { "Label",             "Rendering",  "🔤", "Draws text in the scene" },
    { "TileMap",           "Rendering",  "🗺", "Grid of tiles for levels" },
    { "RigidBody2D",       "Physics",    "⚡", "Physics-driven body (gravity, forces)" },
    { "StaticBody2D",      "Physics",    "🧱", "Immovable physics body (walls, floors)" },
    { "KinematicBody2D",   "Physics",    "🏃", "Script-controlled body (player, enemies)" },
    { "Area2D",            "Physics",    "🔵", "Detects overlaps, no physics response" },
    { "Collider2D",        "Physics",    "⬜", "Collision shape for a body" },
    { "PinJoint2D",        "Physics",    "📌", "Pins two bodies at a point" },
    { "SpringJoint2D",     "Physics",    "🌀", "Spring constraint between bodies" },
    { "AudioPlayer",       "Audio",      "🔊", "Plays sound effects or music" },
    { "Timer",             "Logic",      "⏱", "Fires signal after a duration" },
    { "Tween",             "Logic",      "🎢", "Animates properties over time" },
    { "ScriptNode",        "Logic",      "📝", "Node with attached KonScript" },
    { "PointLight2D",      "Light",      "💡", "Emits light in all directions" },
    { "DirectionalLight2D","Light",      "☀",  "Parallel light (sun-like)" },
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
        )");

        auto* layout = new QVBoxLayout(this);
        layout->setSpacing(8);
        m_search = new QLineEdit();
        m_search->setPlaceholderText("Search node types...");
        layout->addWidget(m_search);
        m_list = new QListWidget();
        layout->addWidget(m_list);
        m_desc = new QLabel("Select a node type");
        m_desc->setWordWrap(true);
        layout->addWidget(m_desc);

        auto* btns = new QDialogButtonBox();
        btns->addButton("Create", QDialogButtonBox::AcceptRole);
        btns->addButton("Cancel", QDialogButtonBox::RejectRole);
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
        m_filtered.clear(); m_list->clear();
        QString cur_cat;
        for (auto& n : NODE_TYPES) {
            if (n.category != cur_cat) {
                cur_cat = n.category;
                auto* sep = new QListWidgetItem("— " + cur_cat + " —");
                sep->setFlags(Qt::NoItemFlags);
                sep->setForeground(QColor("#555"));
                m_list->addItem(sep);
                m_filtered.append(NodeTypeInfo{});
            }
            auto* item = new QListWidgetItem(n.icon + "  " + n.type);
            item->setToolTip(n.desc);
            m_list->addItem(item);
            m_filtered.append(n);
        }
    }
    void filter(const QString& text) {
        m_filtered.clear(); m_list->clear();
        if (text.trimmed().isEmpty()) { populateAll(); return; }
        for (auto& n : NODE_TYPES) {
            if (n.type.contains(text, Qt::CaseInsensitive) ||
                n.desc.contains(text, Qt::CaseInsensitive)) {
                m_list->addItem(new QListWidgetItem(n.icon + "  " + n.type));
                m_filtered.append(n);
            }
        }
        if (m_list->count() > 0) m_list->setCurrentRow(0);
    }
    QLineEdit* m_search; QListWidget* m_list; QLabel* m_desc;
    QList<NodeTypeInfo> m_filtered;
};

// ── SceneTree ─────────────────────────────────────────────────────────────
SceneTree::SceneTree(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0,0,0,0); layout->setSpacing(0);

    auto* header = new QLabel("  Scene");
    header->setStyleSheet("background:#252525;color:#aaa;padding:5px 8px;"
                          "font-size:11px;font-weight:bold;");
    layout->addWidget(header);

    auto* bar = new QHBoxLayout();
    bar->setContentsMargins(6,3,6,3); bar->setSpacing(4);
    auto* addBtn = new QPushButton("+ Node");
    auto* delBtn = new QPushButton("✕");
    addBtn->setFixedHeight(22); delBtn->setFixedHeight(22); delBtn->setFixedWidth(28);
    delBtn->setToolTip("Delete selected node");
    delBtn->setStyleSheet("color:#f44336;");
    bar->addWidget(addBtn); bar->addStretch(); bar->addWidget(delBtn);
    layout->addLayout(bar);

    m_tree = new QTreeWidget();
    m_tree->setHeaderHidden(true); m_tree->setIndentation(16); m_tree->setAnimated(true);
    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_tree->setStyleSheet(
        "QTreeWidget{background:#1a1a1a;color:#ddd;border:none;}"
        "QTreeWidget::item{padding:3px 2px;}"
        "QTreeWidget::item:selected{background:#0078d7;}"
        "QTreeWidget::item:hover{background:#2a2a2a;}");
    layout->addWidget(m_tree);

    connect(addBtn, &QPushButton::clicked, this, &SceneTree::onAddNode);
    connect(delBtn, &QPushButton::clicked, this, &SceneTree::onDeleteNode);
    connect(m_tree, &QTreeWidget::itemClicked, this, &SceneTree::onItemClicked);
    connect(m_tree, &QTreeWidget::itemDoubleClicked,
            [this](QTreeWidgetItem* item, int) {
                if (!item->parent()) return;
                bool ok;
                QString oldName = item->data(0, Qt::UserRole+1).toString();
                QString newName = QInputDialog::getText(this, "Rename Node", "New name:",
                    QLineEdit::Normal, oldName, &ok);
                if (!ok || newName.trimmed().isEmpty() || newName == oldName) return;
                item->setText(0, item->text(0).left(4) + "  " + newName);
                item->setData(0, Qt::UserRole+1, newName);
                autoSaveScene(); emit sceneChanged();
            });
    connect(m_tree, &QTreeWidget::customContextMenuRequested,
            this, &SceneTree::onContextMenu);
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
    m_loading = true;
    m_tree->clear();
    auto* root = new QTreeWidgetItem(m_tree, {"🎬  Main"});
    root->setData(0, Qt::UserRole, "Scene");
    root->setData(0, Qt::UserRole+1, "Main");
    root->setExpanded(true);
    m_sceneLoaded = true;
    m_loading = false;
}

QTreeWidgetItem* SceneTree::addNode(QTreeWidgetItem* parent,
                                    const QString& name, const QString& type) {
    QString icon = "📦";
    for (auto& n : NODE_TYPES) if (n.type == type) { icon = n.icon; break; }
    auto* item = new QTreeWidgetItem(parent, {icon + "  " + name});
    item->setData(0, Qt::UserRole, type);
    item->setData(0, Qt::UserRole+1, name);
    item->setToolTip(0, type);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    parent->setExpanded(true);
    return item;
}

void SceneTree::onAddNode() {
    if (m_tree->topLevelItemCount() == 0) newScene();
    NodePickerDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;
    QString type = dlg.selectedType();
    if (type.isEmpty()) return;

    bool ok;
    QString name = QInputDialog::getText(this, "Node Name", "Name:",
        QLineEdit::Normal, type, &ok);
    if (!ok || name.trimmed().isEmpty()) return;
    name = name.trimmed().replace(' ','_').replace('-','_')
               .replace('.','_').replace('/','_');

    QTreeWidgetItem* parent = m_tree->currentItem();
    if (!parent) parent = m_tree->topLevelItem(0);
    auto* newNode = addNode(parent, name, type);

    static const QStringList builtinTypes = {
        "Node","Node2D","Sprite2D","AnimatedSprite2D",
        "KinematicBody2D","StaticBody2D","RigidBody2D",
        "Collider2D","Area2D","CameraNode2D","AudioStreamPlayer","Timer","Label"
    };

    // Generate the child node's script file
    QString childScriptPath;
    if (!m_projectRoot.isEmpty() && !builtinTypes.contains(name)) {
        childScriptPath = m_projectRoot + "/src/" + name.toLower() + ".ks";
        if (!QFile::exists(childScriptPath)) {
            ScriptAnalyzer sa;
            QFile f(childScriptPath);
            if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream(&f) << sa.generateScript(name, type);
                newNode->setData(0, Qt::UserRole+2, childScriptPath);
                newNode->setToolTip(0, type + " — script: src/" + name.toLower() + ".ks");
            }
        }
    }

    // ── If the parent node has a script, inject the child field into it ──
    // e.g. adding "col" under "player" inserts into player.ks:
    //   let mut col_node: col = this.add(col, "col");
    QString parentScript = parent->data(0, Qt::UserRole + 2).toString();
    bool parentIsRoot    = (parent->data(0, Qt::UserRole).toString() == "Scene");
    if (!parentScript.isEmpty() && QFile::exists(parentScript) && !parentIsRoot) {
        // Resolve the child's KonScript type name from its script
        QString useType = type;
        if (!childScriptPath.isEmpty() && QFile::exists(childScriptPath)) {
            QFile sf(childScriptPath);
            if (sf.open(QIODevice::ReadOnly)) {
                QString src = QTextStream(&sf).readAll();
                QRegularExpression re(R"(node\s+(\w+)\s*:)");
                auto m = re.match(src);
                if (m.hasMatch()) useType = m.captured(1);
            }
        }
        QString varName = name.toLower();
        if (varName == useType.toLower()) varName += "_node";

        // Update the tree item with the resolved var name as display name
        // so the scene file and positions stay consistent
        newNode->setData(0, Qt::UserRole + 1, varName);
        newNode->setText(0, newNode->text(0).left(4) + "  " + varName);

        // Open parent script and inject field after opening brace of node body
        QFile pf(parentScript);
        if (pf.open(QIODevice::ReadOnly)) {
            QString src = QTextStream(&pf).readAll();
            pf.close();
            // Find the opening { of the node body
            QRegularExpression reBody(R"(node\s+\w+\s*:\s*\w+\s*\{)");
            auto bm = reBody.match(src);
            if (bm.hasMatch()) {
                QString field = "\n    let mut " + varName + ": " + useType +
                                " = this.add(" + useType + ", \"" + varName + "\");\n";
                int insertPos = bm.capturedEnd();
                src.insert(insertPos, field);
                if (pf.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                    QTextStream(&pf) << src;
                    pf.close();
                }
            }
        }
        // Also include the child script in parent script if not already there
        if (!childScriptPath.isEmpty()) {
            QFile pf2(parentScript);
            if (pf2.open(QIODevice::ReadOnly)) {
                QString src2 = QTextStream(&pf2).readAll();
                pf2.close();
                QString relPath = QDir(QFileInfo(parentScript).absolutePath())
                                      .relativeFilePath(childScriptPath);
                QString includeStr = "#include \"" + relPath + "\"";
                if (!src2.contains(includeStr)) {
                    // Insert after last #include line
                    int lastInclude = src2.lastIndexOf(QRegularExpression(R"(#include[^\n]+\n)"));
                    if (lastInclude >= 0) {
                        int end = src2.indexOf('\n', lastInclude) + 1;
                        src2.insert(end, includeStr + "\n");
                    } else {
                        src2.prepend(includeStr + "\n");
                    }
                    if (pf2.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                        QTextStream(&pf2) << src2;
                        pf2.close();
                    }
                }
            }
        }
    }

    // ── Node helpers ──────────────────────────────────────────────────────
    static const QStringList physicsTypes = {
        "RigidBody2D","KinematicBody2D","StaticBody2D","Area2D"
    };
    if (physicsTypes.contains(type)) {
        if (QMessageBox::question(this, "Add Collider2D?",
            QString("Add a Collider2D child to \"%1\"?\n\nNeeded for collision.").arg(name),
            QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes) {
            auto* col = addNode(newNode, name+"_col", "Collider2D");
            col->setData(0, Qt::UserRole+3, 0.0f);
            col->setData(0, Qt::UserRole+4, 0.0f);
            newNode->setExpanded(true);
        }
    }
    if (type == "Sprite2D" || type == "AnimatedSprite2D") {
        if (QMessageBox::question(this, "Add AnimationPlayer?",
            QString("Add an AnimationPlayer child to \"%1\"?").arg(name),
            QMessageBox::Yes|QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
            addNode(newNode, name+"_anim", "AnimationPlayer");
            newNode->setExpanded(true);
        }
    }
    if (type == "CameraNode2D" && !m_projectRoot.isEmpty()) {
        if (QMessageBox::question(this, "Follow Camera?",
            QString("Generate a follow-camera script for \"%1\"?").arg(name),
            QMessageBox::Yes|QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
            QString sp = m_projectRoot + "/src/" + name.toLower() + ".ks";
            if (!QFile::exists(sp)) {
                QFile f(sp);
                if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    QTextStream(&f) <<
                        "# " + name + ".ks\n#include <engine>\n\n"
                        "node " + name + " : CameraNode2D {\n"
                        "    let mut smoothSpeed: F64 = 5.0;\n\n"
                        "    func Ready() {\n        x = 0.0;\n        y = 0.0;\n    }\n\n"
                        "    func Update(dt: F64) {\n"
                        "        # x += (target.x - x) * smoothSpeed * dt;\n"
                        "        # y += (target.y - y) * smoothSpeed * dt;\n"
                        "    }\n}\n";
                    newNode->setData(0, Qt::UserRole+2, sp);
                }
            }
        }
    }
    // ─────────────────────────────────────────────────────────────────────

    if (!m_projectRoot.isEmpty()) autoSaveScene();
    m_tree->setCurrentItem(newNode);
    onItemClicked(newNode);
    emit nodeAdded(name, type);
    emit sceneChanged();
}

void SceneTree::onDeleteNode() {
    auto* item = m_tree->currentItem();
    if (!item || !item->parent()) return;
    QString nodeName = item->data(0, Qt::UserRole+1).toString();

    if (QMessageBox::question(this, "Delete Node",
        "Delete \"" + nodeName + "\" and all children?",
        QMessageBox::Yes|QMessageBox::Cancel) != QMessageBox::Yes) return;

    // ── Clean up the file where this node was declared ───────────────────
    // If parent has a script → remove from parent script
    // Otherwise → remove from scene file
    auto* parentItem = item->parent();
    QString targetFile;
    if (parentItem) {
        QString parentScript = parentItem->data(0, Qt::UserRole+2).toString();
        if (!parentScript.isEmpty() && QFile::exists(parentScript))
            targetFile = parentScript;
    }
    if (targetFile.isEmpty() && !m_scenePath.isEmpty())
        targetFile = m_scenePath;

    if (!targetFile.isEmpty()) {
        QFile tf(targetFile);
        if (tf.open(QIODevice::ReadOnly)) {
            QString src = QTextStream(&tf).readAll();
            tf.close();

            // Remove: let mut nodeName: Type = anything.add(...);
            QRegularExpression reField(
                QString(R"(\n?[ \t]*let\s+mut\s+%1\s*:[^;]+;)")
                    .arg(QRegularExpression::escape(nodeName)));
            src.remove(reField);

            // Remove: nodeName.x = ...; nodeName.y = ...;
            QRegularExpression rePos(
                QString(R"(\n?[ \t]*%1\.[xy]\s*=[^;]+;)")
                    .arg(QRegularExpression::escape(nodeName)));
            src.remove(rePos);

            // Remove #include for this node's own script
            QString childScript = item->data(0, Qt::UserRole+2).toString();
            if (!childScript.isEmpty()) {
                QString relPath = QDir(QFileInfo(targetFile).absolutePath())
                                      .relativeFilePath(childScript);
                QRegularExpression reInc(
                    QString(R"(\n?[ \t]*#include\s*"%1"[ \t]*)")
                        .arg(QRegularExpression::escape(relPath)));
                src.remove(reInc);
            }

            if (tf.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                QTextStream(&tf) << src;
                tf.close();
            }
        }
    }
    // ─────────────────────────────────────────────────────────────────────

    delete item;
    emit sceneChanged();
}

void SceneTree::onItemClicked(QTreeWidgetItem* item) {
    emit nodeSelected(item->data(0,Qt::UserRole+1).toString(),
                      item->data(0,Qt::UserRole).toString());
    emit nodeSelectedWithScript(item->data(0,Qt::UserRole+1).toString(),
                                item->data(0,Qt::UserRole).toString(),
                                item->data(0,Qt::UserRole+2).toString());
}

void SceneTree::onContextMenu(const QPoint& pos) {
    auto* item = m_tree->itemAt(pos);
    QMenu menu(this);
    menu.setStyleSheet("QMenu{background:#2a2a2a;color:#ddd;border:1px solid #444;}"
                       "QMenu::item:selected{background:#0078d7;}");
    auto* addAct      = menu.addAction("+ Add Child Node");
    menu.addSeparator();
    auto* dupAct      = menu.addAction("Duplicate");
    auto* delAct      = menu.addAction("Delete");
    auto* attachAct   = menu.addAction("Attach Script");
    menu.addSeparator();
    auto* viewTextAct = menu.addAction("View Scene as Text");
    if (!item || !item->parent()) { delAct->setEnabled(false); dupAct->setEnabled(false); }
    if (!item) attachAct->setEnabled(false);

    auto* chosen = menu.exec(m_tree->mapToGlobal(pos));
    if      (chosen == addAct)      { if (item) m_tree->setCurrentItem(item); onAddNode(); }
    else if (chosen == delAct)      { m_tree->setCurrentItem(item); onDeleteNode(); }
    else if (chosen == attachAct && item) onAttachScript();
    else if (chosen == viewTextAct) onViewAsText();
    else if (chosen == dupAct && item) {
        auto* p = item->parent();
        if (p) { addNode(p, item->data(0,Qt::UserRole+1).toString()+"_copy",
                            item->data(0,Qt::UserRole).toString()); emit sceneChanged(); }
    }
}

void SceneTree::attachScriptToSelected() { onAttachScript(); }

void SceneTree::onViewAsText() {
    autoSaveScene();
    if (m_scenePath.isEmpty()) { QMessageBox::information(this,"View as Text","Save first."); return; }
    QFile f(m_scenePath);
    if (!f.open(QIODevice::ReadOnly)) { QMessageBox::warning(this,"Error","Cannot open: "+m_scenePath); return; }
    QString text = QTextStream(&f).readAll(); f.close();

    auto* dlg = new QDialog(this);
    auto* layout = new QVBoxLayout(dlg);
    auto* editor = new KonScriptEditor(dlg);
    editor->setPlainText(text); editor->setReadOnly(false);
    layout->addWidget(editor); layout->setContentsMargins(0,0,0,4);
    dlg->setWindowTitle("Scene: " + QFileInfo(m_scenePath).fileName());
    dlg->resize(640,520);
    auto* bar = new QHBoxLayout();
    auto* saveBtn = new QPushButton("Save & Reload");
    auto* closeBtn = new QPushButton("Close");
    bar->addStretch(); bar->addWidget(saveBtn); bar->addWidget(closeBtn);
    layout->addLayout(bar);
    QString sp = m_scenePath;
    connect(saveBtn, &QPushButton::clicked, [this,editor,sp,dlg]{
        QFile f2(sp);
        if (f2.open(QIODevice::WriteOnly|QIODevice::Text)) { QTextStream(&f2) << editor->toPlainText(); f2.close(); }
        loadScene(sp); dlg->close();
    });
    connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::close);
    dlg->show();
}

void SceneTree::selectNodeByName(const QString& name) {
    QList<QTreeWidgetItem*> stack;
    if (!m_tree->topLevelItemCount()) return;
    stack.append(m_tree->topLevelItem(0));
    while (!stack.isEmpty()) {
        auto* item = stack.takeLast();
        if (item->data(0,Qt::UserRole+1).toString() == name) {
            m_tree->setCurrentItem(item); onItemClicked(item); return;
        }
        for (int i = 0; i < item->childCount(); i++) stack.append(item->child(i));
    }
}

void SceneTree::updateNodePosition(const QString& name, float x, float y) {
    std::function<QTreeWidgetItem*(QTreeWidgetItem*)> find;
    find = [&](QTreeWidgetItem* item) -> QTreeWidgetItem* {
        if (item->data(0,Qt::UserRole+1).toString() == name) return item;
        for (int i = 0; i < item->childCount(); i++) if (auto* r = find(item->child(i))) return r;
        return nullptr;
    };
    if (!m_tree->topLevelItemCount()) return;
    if (auto* found = find(m_tree->topLevelItem(0))) {
        found->setData(0,Qt::UserRole+3,x); found->setData(0,Qt::UserRole+4,y);
        // Don't autoSave here — called on every mouse move during drag.
        // Scene is saved on drag release via nodeMovedFinal → writeInstancePosition.
    }
}

void SceneTree::autoSaveScene() {
    if (m_readOnly) return;  // NEVER overwrite monolithic .ks files
    if (m_loading || m_scenePath.isEmpty()) return;
    auto* root = m_tree->topLevelItem(0);
    if (!root || root->childCount() == 0) return;
    saveScene(m_scenePath);
}

void SceneTree::saveCurrentScene() {
    if (m_readOnly) return;  // NEVER overwrite monolithic .ks files
    if (m_scenePath.isEmpty()) return;
    auto* root = m_tree->topLevelItem(0);
    if (!root || root->childCount() == 0) return;
    saveScene(m_scenePath);
}

void SceneTree::onAttachScript() {
    auto* item = m_tree->currentItem();
    if (!item) return;
    QString nodeName = item->data(0,Qt::UserRole+1).toString();
    QString nodeType = item->data(0,Qt::UserRole).toString();
    QString suggested = m_projectRoot.isEmpty() ? nodeName.toLower()+".ks"
        : m_projectRoot+"/src/"+nodeName.toLower()+".ks";
    QString path = QFileDialog::getSaveFileName(this,"Attach Script",suggested,"KonScript (*.ks)");
    if (path.isEmpty()) return;
    ScriptAnalyzer sa;
    if (!QFile::exists(path)) {
        QFile f(path);
        if (f.open(QIODevice::WriteOnly|QIODevice::Text)) QTextStream(&f) << sa.generateScript(nodeName,nodeType);
    }
    item->setData(0,Qt::UserRole+2,path);
    item->setToolTip(0,nodeType+" — script: "+QDir(m_projectRoot).relativeFilePath(path));
    emit scriptRequested(nodeName,path); emit sceneChanged();
}

void SceneTree::loadScene(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return;
    QByteArray data = f.readAll(); f.close();

    m_loading = true;
    m_tree->clear();
    m_scenePath = path;

    // Legacy JSON
    auto doc = QJsonDocument::fromJson(data);
    if (doc.isObject()) {
        auto root = doc.object();
        QString rootName = root.value("name").toString("Scene");
        auto* rootItem = new QTreeWidgetItem(m_tree,{"🎬  "+rootName});
        rootItem->setData(0,Qt::UserRole,"Scene"); rootItem->setData(0,Qt::UserRole+1,rootName);
        rootItem->setExpanded(true);
        jsonToTree(rootItem, root.value("children").toArray());
        m_sceneLoaded = true; m_loading = false; emit sceneLoaded(path); return;
    }

    QString src = QString::fromUtf8(data);

    // Detect if this is a monolithic file (has func main()) vs a scene file
    bool isMonolithic = src.contains("func main()") || src.contains("func main()");;

    // For scene files: root = first node definition
    // For monolithic: root = filename, children = scene.add() calls
    QString rootName;
    if (!isMonolithic) {
        QRegularExpression reNode(R"(node\s+(\w+)\s*:\s*\w+\s*\{)");
        auto mNode = reNode.match(src);
        rootName = mNode.hasMatch() ? mNode.captured(1) : "Main";
    } else {
        rootName = QFileInfo(path).baseName();
    }

    auto* rootItem = new QTreeWidgetItem(m_tree,{"🎬  "+rootName});
    rootItem->setData(0,Qt::UserRole,"Scene"); rootItem->setData(0,Qt::UserRole+1,rootName);
    rootItem->setExpanded(true);

    if (isMonolithic) {
        // ── Monolithic file: parse node definitions and scene.add() calls ──

        // Step 1: Scan for const declarations and build a lookup map.
        // Matches patterns like: const screenWidth = 900;
        QMap<QString, float> constMap;
        {
            QRegularExpression reConst(R"(const\s+(\w+)\s*(?::\s*\w+\s*)?=\s*([^;]+);)");
            auto itConst = reConst.globalMatch(src);
            while (itConst.hasNext()) {
                auto cm = itConst.next();
                QString name = cm.captured(1).trimmed();
                QString val  = cm.captured(2).trimmed();
                bool ok = false;
                float v = val.toFloat(&ok);
                if (ok) constMap[name] = v;
            }
        }
        // Also extract InitWindow(w, h, ...) arguments as screenWidth/screenHeight
        {
            QRegularExpression reInit(R"(InitWindow\s*\(\s*(\d+)\s*,\s*(\d+)\s*,)");
            auto im = reInit.match(src);
            if (im.hasMatch()) {
                if (!constMap.contains("screenWidth"))
                    constMap["screenWidth"] = im.captured(1).toFloat();
                if (!constMap.contains("screenHeight"))
                    constMap["screenHeight"] = im.captured(2).toFloat();
                // Store game resolution on the root item (UserRole+8 = width, +9 = height)
                rootItem->setData(0, Qt::UserRole+8, im.captured(1).toInt());
                rootItem->setData(0, Qt::UserRole+9, im.captured(2).toInt());
            }
        }

        // First, collect all node definitions: node Name : Base { ... }
        QRegularExpression reAllNodes(R"(node\s+(\w+)\s*:\s*(\w+)\s*\{)");
        auto itNodes = reAllNodes.globalMatch(src);
        QMap<QString, QTreeWidgetItem*> nodeItems; // typeName → tree item

        // Helper: try to evaluate a value expression to a float.
        // Handles:
        //   - Plain numeric literals: "450" -> 450
        //   - Simple binary expressions with one const: "screenWidth / 2" -> 450
        //   - Simple binary expressions with two literals: "700 / 1.5" -> 466.67
        // Returns false if the expression is too complex.
        auto tryParseNumeric = [&constMap](const QString& val, float* out) -> bool {
            QString trimmed = val.trimmed();
            if (trimmed.isEmpty()) return false;

            // Try plain numeric literal first
            {
                bool ok = false;
                float v = trimmed.toFloat(&ok);
                if (ok) { if (out) *out = v; return true; }
            }

            // Try simple binary expression: operand op operand
            // where each operand is a number or a known const
            static QRegularExpression reBinOp(R"(^(\w[\w.]*)\s*([+\-\*/])\s*(\w[\w.]*)$)");
            auto m = reBinOp.match(trimmed);
            if (!m.hasMatch()) return false;

            QString lhs = m.captured(1).trimmed();
            QChar   op  = m.captured(2).at(0);
            QString rhs = m.captured(3).trimmed();

            // Resolve each operand to a float
            auto resolveOperand = [&constMap](const QString& s, float* v) -> bool {
                bool ok = false;
                float f = s.toFloat(&ok);
                if (ok) { *v = f; return true; }
                if (constMap.contains(s)) { *v = constMap[s]; return true; }
                return false;
            };

            float lVal = 0, rVal = 0;
            if (!resolveOperand(lhs, &lVal) || !resolveOperand(rhs, &rVal))
                return false;

            float result = 0;
            if      (op == '+') result = lVal + rVal;
            else if (op == '-') result = lVal - rVal;
            else if (op == '*') result = lVal * rVal;
            else if (op == '/') {
                if (rVal == 0.0f) return false;
                result = lVal / rVal;
            } else {
                return false;
            }

            if (out) *out = result;
            return true;
        };

        while (itNodes.hasNext()) {
            auto nm = itNodes.next();
            QString nodeName = nm.captured(1);
            QString baseType = nm.captured(2);
            auto* nodeItem = addNode(rootItem, nodeName, baseType);
            nodeItem->setData(0, Qt::UserRole+2, path);
            nodeItems[nodeName] = nodeItem;

            // Parse this.add() inside the node to find children
            // Find the node's body
            int braceStart = nm.capturedEnd();
            int depth = 1, pos = braceStart;
            while (pos < src.size() && depth > 0) {
                if (src[pos] == '{') depth++;
                else if (src[pos] == '}') depth--;
                if (depth > 0) pos++;
            }
            QString body = src.mid(braceStart, pos - braceStart);

            // Parse the node's own Ready() for x, y, width, height assignments
            {
                QRegularExpression reReady(R"(func\s+Ready\s*\(\s*\)\s*\{)");
                auto rm = reReady.match(body);
                if (rm.hasMatch()) {
                    int rStart = rm.capturedEnd();
                    int rDepth = 1, rPos = rStart;
                    while (rPos < body.size() && rDepth > 0) {
                        if (body[rPos] == QLatin1Char('{')) rDepth++;
                        else if (body[rPos] == QLatin1Char('}')) rDepth--;
                        if (rDepth > 0) rPos++;
                    }
                    QString readyBody = body.mid(rStart, rPos - rStart);

                    // Parse bare assignments: x = value; y = value; width = value; height = value;
                    // Only match assignments that are NOT prefixed by a variable name (i.e. not "foo.x = ...")
                    QRegularExpression reAssign(R"((?:^|[;\n\{])\s*(\w+)\s*=\s*([^;]+);)");
                    auto itAssign = reAssign.globalMatch(readyBody);
                    while (itAssign.hasNext()) {
                        auto am = itAssign.next();
                        QString prop = am.captured(1).trimmed();
                        QString val  = am.captured(2).trimmed();
                        float numVal = 0;
                        if (!tryParseNumeric(val, &numVal)) continue; // skip expressions
                        if (prop == "x")      nodeItem->setData(0, Qt::UserRole+3, numVal);
                        else if (prop == "y") nodeItem->setData(0, Qt::UserRole+4, numVal);
                        else if (prop == "width")  nodeItem->setData(0, Qt::UserRole+5, numVal);
                        else if (prop == "height") nodeItem->setData(0, Qt::UserRole+6, numVal);
                    }
                }
            }

            // Parse this.add(Type, "name") inside the body
            QRegularExpression reChildAdd(R"RE(let\s+mut\s+(\w+)\s*:\s*(\w+)\s*=\s*this\.add\(\s*\w+\s*,\s*"([^"]+)"\s*\))RE");
            auto itChildren = reChildAdd.globalMatch(body);
            while (itChildren.hasNext()) {
                auto cm = itChildren.next();
                QString childVar = cm.captured(1);
                QString childType = cm.captured(2);
                QString childName = cm.captured(3);
                auto* childItem = addNode(nodeItem, childName, childType);

                // Parse position from Ready() body
                QRegularExpression rxX(QString(R"(%1\s*[.=]\s*x\s*=\s*([\d.+\-]+))").arg(QRegularExpression::escape(childVar)));
                QRegularExpression rxY(QString(R"(%1\s*[.=]\s*y\s*=\s*([\d.+\-]+))").arg(QRegularExpression::escape(childVar)));
                auto mx = rxX.match(body); auto my = rxY.match(body);
                if (mx.hasMatch()) childItem->setData(0, Qt::UserRole+3, mx.captured(1).toFloat());
                if (my.hasMatch()) childItem->setData(0, Qt::UserRole+4, my.captured(1).toFloat());
            }
        }

        // Parse scene.add(Type, "name") in func main() for the top-level scene hierarchy
        QRegularExpression reSceneAdd(R"RE((\w+)\.add\(\s*(\w+)\s*,\s*"([^"]+)"\s*\))RE");
        // Find func main() body
        QRegularExpression reMain(R"(func\s+main\s*\([^)]*\)\s*(?:->\s*\w+\s*)?\{)");
        auto mainMatch = reMain.match(src);
        if (mainMatch.hasMatch()) {
            int mainStart = mainMatch.capturedEnd();
            int depth = 1, pos = mainStart;
            while (pos < src.size() && depth > 0) {
                if (src[pos] == '{') depth++;
                else if (src[pos] == '}') depth--;
                if (depth > 0) pos++;
            }
            QString mainBody = src.mid(mainStart, pos - mainStart);

            // Parse variable.x = value assignments in main()
            QMap<QString, QString> varToType; // varName → typeName
            QRegularExpression reVarAdd(R"RE(let\s+mut\s+(\w+)\s*:\s*(\w+)\s*=\s*\w+\.add\(\s*\w+\s*,\s*"[^"]+"\s*\))RE");
            auto itVars = reVarAdd.globalMatch(mainBody);
            while (itVars.hasNext()) {
                auto vm = itVars.next();
                varToType[vm.captured(1)] = vm.captured(2);
            }

            // Parse position/size assignments: varName.x = value; varName.width = value; etc.
            for (auto it = varToType.constBegin(); it != varToType.constEnd(); ++it) {
                QString var = it.key();
                QString type = it.value();
                // Find the node item for this type
                QTreeWidgetItem* item = nodeItems.value(type, nullptr);
                if (!item) {
                    // It's an instance, find or create under root
                    for (int i = 0; i < rootItem->childCount(); i++) {
                        if (rootItem->child(i)->data(0, Qt::UserRole+1).toString() == type)
                            item = rootItem->child(i);
                    }
                }
                if (!item) continue;

                // Parse x, y, width, height assignments from main()
                QRegularExpression rxProp(QString(R"(%1\.(\w+)\s*=\s*([^;]+);)").arg(QRegularExpression::escape(var)));
                auto itProps = rxProp.globalMatch(mainBody);
                while (itProps.hasNext()) {
                    auto pm = itProps.next();
                    QString prop = pm.captured(1).trimmed();
                    QString val  = pm.captured(2).trimmed();
                    float numVal = 0;
                    if (!tryParseNumeric(val, &numVal)) continue; // skip expressions
                    if (prop == "x")           item->setData(0, Qt::UserRole+3, numVal);
                    else if (prop == "y")      item->setData(0, Qt::UserRole+4, numVal);
                    else if (prop == "width")  item->setData(0, Qt::UserRole+5, numVal);
                    else if (prop == "height") item->setData(0, Qt::UserRole+6, numVal);
                }
            }

            // Parse LoadTexture / SetTexture calls in main() to associate textures with nodes
            // Step 1: let mut varName: Texture = LoadTexture("path") → texVarToPath
            QMap<QString, QString> texVarToPath;
            {
                QRegularExpression reTex(R"RE(let\s+mut\s+(\w+)\s*:\s*Texture\s*=\s*LoadTexture\s*\(\s*"([^"]+)"\s*\))RE");
                auto itTex = reTex.globalMatch(mainBody);
                while (itTex.hasNext()) {
                    auto tm = itTex.next();
                    texVarToPath[tm.captured(1)] = tm.captured(2);
                    fprintf(stderr, "[SceneTree] Texture var: %s → %s\n",
                            tm.captured(1).toUtf8().constData(), tm.captured(2).toUtf8().constData());
                }
                if (texVarToPath.isEmpty())
                    fprintf(stderr, "[SceneTree] No LoadTexture calls found in main()\n");
            }
            // Step 2: nodeVar.SetTexture(texVar) → associate node with texture path
            {
                QRegularExpression reSetTex(R"((\w+)\.SetTexture\s*\(\s*(\w+)\s*\))");
                auto itSetTex = reSetTex.globalMatch(mainBody);
                while (itSetTex.hasNext()) {
                    auto sm = itSetTex.next();
                    QString nodeVar = sm.captured(1);
                    QString texVar  = sm.captured(2);
                    fprintf(stderr, "[SceneTree] SetTexture: %s.SetTexture(%s)\n",
                            nodeVar.toUtf8().constData(), texVar.toUtf8().constData());
                    if (!texVarToPath.contains(texVar)) {
                        fprintf(stderr, "[SceneTree]   texVar '%s' not in texVarToPath\n", texVar.toUtf8().constData());
                        continue;
                    }
                    // Find the tree item for this nodeVar
                    QString nodeType = varToType.value(nodeVar);
                    fprintf(stderr, "[SceneTree]   nodeVar '%s' → type '%s'\n",
                            nodeVar.toUtf8().constData(), nodeType.toUtf8().constData());
                    QTreeWidgetItem* item = nodeItems.value(nodeType, nullptr);
                    if (!item) {
                        for (int i = 0; i < rootItem->childCount(); i++) {
                            if (rootItem->child(i)->data(0, Qt::UserRole+1).toString() == nodeType)
                                item = rootItem->child(i);
                        }
                    }
                    if (item) {
                        item->setData(0, Qt::UserRole+7, texVarToPath[texVar]);
                        fprintf(stderr, "[SceneTree]   → set texture '%s' on '%s'\n",
                                texVarToPath[texVar].toUtf8().constData(),
                                item->data(0, Qt::UserRole+1).toString().toUtf8().constData());
                    } else {
                        fprintf(stderr, "[SceneTree]   → no tree item found for type '%s'\n",
                                nodeType.toUtf8().constData());
                    }
                }
            }
        }

        rootItem->setExpanded(true);
        m_sceneLoaded = true;
        emit sceneLoaded(path);
        m_loading = false;
        return;
    }

    // ── Scene file parsing (non-monolithic) ──────────────────────────────

    // Parse all .add() calls — handles both this.add() and parentVar.add()
    // Pattern: let mut varName: Type = parentExpr.add(Type, "name")
    QRegularExpression reAdd(
        R"RE(let\s+mut\s+(\w+)\s*:\s*(\w+)\s*=\s*(\w+)\.add\(\s*\w+\s*,\s*"([^"]+)"\s*\))RE");

    // Map varName → tree item so nested children find their parent
    QMap<QString,QTreeWidgetItem*> varToItem;
    varToItem["this"] = rootItem;

    struct AddCall { QString varName, typeName, parentVar, displayName; };

    // Recursively parse a source file and add its nodes to the tree
    // sourceFile is the path of the .ks being parsed (for resolving x/y positions)
    std::function<void(const QString&, const QString&, QTreeWidgetItem*)> parseSource;
    parseSource = [&](const QString& source, const QString& sourceFile,
                      QTreeWidgetItem* /*unused — varToItem handles parenting*/) {
        QList<AddCall> calls;
        auto it2 = reAdd.globalMatch(source);
        while (it2.hasNext()) {
            auto m = it2.next();
            calls.append({m.captured(1),m.captured(2),m.captured(3),m.captured(4)});
        }

        for (auto& call : calls) {
            QTreeWidgetItem* parentItem = varToItem.value(call.parentVar, rootItem);
            // Skip if we already have this var (e.g. scene file already parsed it)
            if (varToItem.contains(call.varName)) continue;

            auto* child = addNode(parentItem, call.varName, call.typeName);
            varToItem[call.varName] = child;

            // Find script for this child
            QString childScript;
            if (!m_projectRoot.isEmpty()) {
                QString byVar  = m_projectRoot+"/src/"+call.varName.toLower()+".ks";
                QString byType = m_projectRoot+"/src/"+call.typeName.toLower()+".ks";
                if      (QFile::exists(byVar))  childScript = byVar;
                else if (QFile::exists(byType)) childScript = byType;
            }
            if (!childScript.isEmpty())
                child->setData(0, Qt::UserRole+2, childScript);

            // Parse x/y positions from the source file
            QRegularExpression rxX(QString(R"(%1\.x\s*=\s*([\d.+\-]+))").arg(call.varName));
            QRegularExpression rxY(QString(R"(%1\.y\s*=\s*([\d.+\-]+))").arg(call.varName));
            auto mx = rxX.match(source); auto my = rxY.match(source);
            if (mx.hasMatch()) child->setData(0,Qt::UserRole+3,mx.captured(1).toFloat());
            if (my.hasMatch()) child->setData(0,Qt::UserRole+4,my.captured(1).toFloat());

            // Parse SetTexture / LoadTexture in the source or child script for texture association
            {
                // Check the current source for varName.SetTexture(texVar) patterns
                // First build a LoadTexture map from the source
                QMap<QString, QString> localTexVars;
                QRegularExpression reLoadTex(R"RE(let\s+mut\s+(\w+)\s*:\s*Texture\s*=\s*LoadTexture\s*\(\s*"([^"]+)"\s*\))RE");
                auto itLT = reLoadTex.globalMatch(source);
                while (itLT.hasNext()) {
                    auto ltm = itLT.next();
                    localTexVars[ltm.captured(1)] = ltm.captured(2);
                }
                // Now find varName.SetTexture(texVar)
                QRegularExpression reSetTex(QString(R"(%1\.SetTexture\s*\(\s*(\w+)\s*\))").arg(QRegularExpression::escape(call.varName)));
                auto stm = reSetTex.match(source);
                if (stm.hasMatch()) {
                    QString texVar = stm.captured(1);
                    if (localTexVars.contains(texVar))
                        child->setData(0, Qt::UserRole+7, localTexVars[texVar]);
                }
            }

            // If this child has a script, parse that script too for its children
            if (!childScript.isEmpty()) {
                QFile sf(childScript);
                if (sf.open(QIODevice::ReadOnly)) {
                    QString childSrc = QTextStream(&sf).readAll();
                    sf.close();

                    // Check child script for texture property or SetTexture call
                    if (!child->data(0, Qt::UserRole+7).isValid()) {
                        QRegularExpression reScriptTex(R"RE(LoadTexture\s*\(\s*"([^"]+)"\s*\))RE");
                        auto stxm = reScriptTex.match(childSrc);
                        if (stxm.hasMatch())
                            child->setData(0, Qt::UserRole+7, stxm.captured(1));
                    }

                    // Add "this" mapping for this child so its children parent correctly
                    varToItem["this"] = child;
                    parseSource(childSrc, childScript, child);
                    // Restore "this" to root for siblings
                    varToItem["this"] = rootItem;
                }
            }
        }
    };

    // First parse the scene file itself (non-monolithic scene files)
    parseSource(src, path, rootItem);

    rootItem->setExpanded(true);
    m_sceneLoaded = true;
    // Keep m_loading true during the signal emission so autoSave is blocked.
    // Reset it after the signal chain completes.
    emit sceneLoaded(path);
    m_loading = false;
}

void SceneTree::saveScene(const QString& path) {
    if (m_readOnly) return;  // NEVER overwrite monolithic .ks files
    auto* root = m_tree->topLevelItem(0);
    if (!root) return;
    if (m_projectRoot.isEmpty()) return;

    QString sceneName = root->data(0,Qt::UserRole+1).toString();
    if (sceneName.isEmpty()) sceneName = "Main";

    QString ks;
    ks += "// " + sceneName + ".ks — generated by KonEditor\n";

    QStringList includes;
    std::function<void(QTreeWidgetItem*)> collectIncludes;
    collectIncludes = [&](QTreeWidgetItem* item) {
        if (!item) return;
        QString script = item->data(0,Qt::UserRole+2).toString();
        if (!script.isEmpty()) {
            QString rel = QDir(m_projectRoot).relativeFilePath(script);
            if (!includes.contains(rel)) includes.append(rel);
        }
        for (int i = 0; i < item->childCount(); i++) collectIncludes(item->child(i));
    };
    for (int i = 0; i < root->childCount(); i++) collectIncludes(root->child(i));

    QString sceneDir = QFileInfo(path).absolutePath();
    for (auto& inc : includes) {
        QString absInc = QDir(m_projectRoot).absoluteFilePath(inc);
        ks += "#include \"" + QDir(sceneDir).relativeFilePath(absInc) + "\"\n";
    }
    ks += "\n";
    ks += "node " + sceneName + " : Node2D {\n";

    static const QStringList builtinTypes = {
        "Node","Node2D","Sprite2D","AnimatedSprite2D",
        "KinematicBody2D","StaticBody2D","RigidBody2D",
        "Collider2D","Area2D","CameraNode2D","AudioStreamPlayer","Timer","Label"
    };

    // nameToVar: shared between writeNode and writePositions
    QMap<QString,QString> nameToVar;

    // writeNode passes parentVar so nested children use parentVar.add() not this.add()
    std::function<void(QTreeWidgetItem*,int,const QString&)> writeNode;
    writeNode = [&](QTreeWidgetItem* item, int depth, const QString& parentVar) {
        if (!item) return;
        QString name = item->data(0,Qt::UserRole+1).toString();
        QString type = item->data(0,Qt::UserRole).toString();
        if (type == "Scene" || name.isEmpty()) {
            for (int i = 0; i < item->childCount(); i++)
                writeNode(item->child(i), depth, parentVar);
            return;
        }
        QString script = item->data(0,Qt::UserRole+2).toString();
        if (script.isEmpty() && !builtinTypes.contains(type)) {
            QString guessed = m_projectRoot+"/src/"+type.toLower()+".ks";
            if (QFile::exists(guessed)) script = guessed;
        }
        if (!script.isEmpty()) {
            QString rel = QDir(m_projectRoot).relativeFilePath(script);
            if (!includes.contains(rel)) includes.append(rel);
        }

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

        QString varName = name.toLower();
        if (varName == useType.toLower()) varName += "_node";
        nameToVar[name] = varName;

        QString indent(depth*4,' ');
        QString addTarget = (depth == 1) ? "this" : parentVar;
        ks += indent + "let mut " + varName + ": " + useType +
              " = " + addTarget + ".add(" + useType + ", \"" + name + "\");\n";

        // If this node has a script, its children are managed inside that script —
        // don't write them to the scene file.
        if (script.isEmpty()) {
            for (int i = 0; i < item->childCount(); i++)
                writeNode(item->child(i), depth+1, varName);
        }
    };
    for (int i = 0; i < root->childCount(); i++)
        writeNode(root->child(i), 1, "this");

    ks += "\n    func Ready() {\n";

    // writePositions: looks up nameToVar — never re-derives independently
    std::function<void(QTreeWidgetItem*)> writePositions;
    writePositions = [&](QTreeWidgetItem* item) {
        if (!item) return;
        QString name = item->data(0,Qt::UserRole+1).toString();
        QString type = item->data(0,Qt::UserRole).toString();
        if (type == "Scene" || name.isEmpty()) {
            for (int i = 0; i < item->childCount(); i++) writePositions(item->child(i));
            return;
        }
        QVariant vx = item->data(0,Qt::UserRole+3);
        QVariant vy = item->data(0,Qt::UserRole+4);
        float x = vx.isValid() ? vx.toFloat() : 0.0f;
        float y = vy.isValid() ? vy.toFloat() : 0.0f;
        QString varName = nameToVar.value(name, name.toLower());
        ks += "        " + varName + ".x = " + QString::number(x,'f',1) + ";\n";
        ks += "        " + varName + ".y = " + QString::number(y,'f',1) + ";\n";
        // Don't write positions for children of scripted nodes — they're in the script
        QString script2 = item->data(0,Qt::UserRole+2).toString();
        if (script2.isEmpty()) {
            for (int i = 0; i < item->childCount(); i++) writePositions(item->child(i));
        }
    };
    for (int i = 0; i < root->childCount(); i++) writePositions(root->child(i));

    ks += "    }\n}\n";

    QFile f(path);
    QDir().mkpath(QFileInfo(path).absolutePath());
    if (f.open(QIODevice::WriteOnly|QIODevice::Text)) {
        QTextStream(&f) << ks; f.flush(); f.close();
    }
    m_scenePath = path;
}

QJsonObject SceneTree::treeToJson(QTreeWidgetItem* item) const {
    QJsonObject obj;
    obj["name"] = item->data(0,Qt::UserRole+1).toString();
    obj["type"] = item->data(0,Qt::UserRole).toString();
    QString script = item->data(0,Qt::UserRole+2).toString();
    if (!script.isEmpty()) obj["script"] = script;
    QVariant vx = item->data(0,Qt::UserRole+3);
    QVariant vy = item->data(0,Qt::UserRole+4);
    if (vx.isValid()) obj["x"] = vx.toDouble();
    if (vy.isValid()) obj["y"] = vy.toDouble();
    QJsonArray children;
    for (int i = 0; i < item->childCount(); i++) children.append(treeToJson(item->child(i)));
    if (!children.isEmpty()) obj["children"] = children;
    return obj;
}

void SceneTree::jsonToTree(QTreeWidgetItem* parent, const QJsonArray& nodes) {
    for (auto v : nodes) {
        auto o = v.toObject();
        auto* item = addNode(parent, o.value("name").toString("Node"), o.value("type").toString("Node"));
        QString script = o.value("script").toString();
        if (!script.isEmpty()) {
            item->setData(0,Qt::UserRole+2,script);
            item->setToolTip(0, o.value("type").toString()+" — script: "+QFileInfo(script).fileName());
        }
        if (o.contains("x")) item->setData(0,Qt::UserRole+3,o["x"].toDouble());
        if (o.contains("y")) item->setData(0,Qt::UserRole+4,o["y"].toDouble());
        jsonToTree(item, o.value("children").toArray());
    }
}
