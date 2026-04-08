#include "WelcomeScreen.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QInputDialog>
#include <QStandardPaths>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QApplication>
#include <QLineEdit>
#include "ProjectManager.hpp"

WelcomeScreen::WelcomeScreen(QWidget* parent) : QDialog(parent) {
    setWindowTitle("KonEditor");
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setFixedSize(860, 540);
    setModal(true);

    setStyleSheet(R"(
        WelcomeScreen, QDialog {
            background: #1a1a1a;
            border: 1px solid #333;
        }
        QPushButton {
            background: #2a2a2a;
            color: #ddd;
            border: 1px solid #444;
            padding: 8px 20px;
            border-radius: 4px;
            font-size: 12px;
            text-align: left;
        }
        QPushButton:hover { background: #333; border-color: #0078d7; }
        QPushButton:pressed { background: #0078d7; color: #fff; }
        QPushButton#primary {
            background: #0078d7;
            color: #fff;
            border-color: #0078d7;
            font-weight: bold;
            text-align: center;
        }
        QPushButton#primary:hover { background: #1084e0; }
        QListWidget {
            background: #141414;
            color: #ccc;
            border: none;
            outline: none;
            font-size: 12px;
        }
        QListWidget::item {
            padding: 10px 12px;
            border-bottom: 1px solid #1e1e1e;
            border-radius: 0px;
        }
        QListWidget::item:selected {
            background: #0078d7;
            color: #fff;
        }
        QListWidget::item:hover {
            background: #1e3a5f;
        }
        QListWidget::item:selected:hover {
            background: #0078d7;
            color: #fff;
        }
        QLabel#title {
            color: #ffffff;
            font-size: 22px;
            font-weight: bold;
        }
        QLabel#subtitle { color: #555; font-size: 11px; }
        QLabel#section  { color: #666; font-size: 10px; font-weight: bold; letter-spacing: 1px; }
    )");

    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Sidebar ─────────────────────────────────────────────────────────
    auto* sidebar = new QWidget();
    sidebar->setFixedWidth(220);
    sidebar->setStyleSheet("background: #111111; border-right: 1px solid #2a2a2a;");
    auto* side = new QVBoxLayout(sidebar);
    side->setContentsMargins(20, 28, 20, 20);
    side->setSpacing(0);

    // Logo — try filesystem logo.png first, then Qt resource, then fallback
    auto* logo = new QLabel();
    QPixmap px;
    // Try filesystem paths (logo.png in repo root)
    QStringList logoPaths = {
        QCoreApplication::applicationDirPath() + "/logo.png",
        QCoreApplication::applicationDirPath() + "/../logo.png",
        QCoreApplication::applicationDirPath() + "/../../logo.png",
        QCoreApplication::applicationDirPath() + "/../../../logo.png",
        "logo.png",
        ":/logo.png",  // Qt resource fallback
    };
    for (auto& p : logoPaths) {
        px = QPixmap(p);
        if (!px.isNull()) break;
    }
    if (px.isNull()) {
        logo->setText(QString::fromUtf8("\xe2\xac\xa1")); // ⬡ fallback
        logo->setStyleSheet("color: #0078d7; font-size: 36px; background: transparent;");
    } else {
        logo->setPixmap(px.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    side->addWidget(logo);
    side->addSpacing(6);

    auto* title = new QLabel("KonEditor");
    title->setObjectName("title");
    title->setStyleSheet("background: transparent; color: #fff; font-size: 22px; font-weight: bold;");
    side->addWidget(title);

    auto* ver = new QLabel("v0.1.0");
    ver->setObjectName("subtitle");
    ver->setStyleSheet("background: transparent; color: #555; font-size: 11px;");
    side->addWidget(ver);

    side->addSpacing(32);

    auto* newBtn = new QPushButton("  + New Project");
    newBtn->setObjectName("primary");
    newBtn->setFixedHeight(38);
    newBtn->setCursor(Qt::PointingHandCursor);
    side->addWidget(newBtn);
    side->addSpacing(8);

    auto* openBtn = new QPushButton("  \xe2\x86\x97 Open Project");
    openBtn->setFixedHeight(38);
    openBtn->setCursor(Qt::PointingHandCursor);
    side->addWidget(openBtn);
    side->addSpacing(8);

    auto* openFileBtn = new QPushButton("  \xf0\x9f\x93\x84 Open File (.ks)");
    openFileBtn->setFixedHeight(38);
    openFileBtn->setCursor(Qt::PointingHandCursor);
    side->addWidget(openFileBtn);

    side->addStretch();

    // Close button (top-right corner of dialog)
    auto* closeBtn = new QPushButton("x", this);
    closeBtn->setFixedSize(26, 26);
    closeBtn->move(860 - 32, 6);
    closeBtn->setStyleSheet(
        "QPushButton { background: transparent; color: #555; border: none; font-size: 13px; font-weight: bold; }"
        "QPushButton:hover { color: #f44336; background: #2a2a2a; border-radius: 4px; }");
    closeBtn->raise();
    connect(closeBtn, &QPushButton::clicked, this, &WelcomeScreen::reject);

    auto* links = new QLabel(
        "<a style='color:#444; text-decoration:none;' "
        "href='https://github.com/AnoDelta/KonEngine'>GitHub</a>"
        "  &middot;  "
        "<a style='color:#444; text-decoration:none;' href='#'>Docs</a>");
    links->setTextFormat(Qt::RichText);
    links->setOpenExternalLinks(true);
    links->setStyleSheet("background: transparent; font-size: 11px;");
    side->addWidget(links);

    root->addWidget(sidebar);

    // ── Right panel ──────────────────────────────────────────────────────
    auto* right = new QWidget();
    right->setStyleSheet("background: #1a1a1a;");
    auto* rl = new QVBoxLayout(right);
    rl->setContentsMargins(28, 24, 28, 20);
    rl->setSpacing(10);

    auto* hdr = new QHBoxLayout();
    auto* recentLabel = new QLabel("RECENT PROJECTS");
    recentLabel->setObjectName("section");
    hdr->addWidget(recentLabel);
    hdr->addStretch();

    auto* removeBtn = new QPushButton("Remove from list");
    removeBtn->setFixedHeight(22);
    removeBtn->setStyleSheet("font-size: 10px; padding: 2px 8px; text-align: center;");
    hdr->addWidget(removeBtn);
    rl->addLayout(hdr);

    // List
    m_recentList = new QListWidget();
    m_recentList->setMouseTracking(true);
    m_recentList->viewport()->setMouseTracking(true);
    m_recentList->viewport()->installEventFilter(this);
    rl->addWidget(m_recentList);

    // Trash overlay button
    m_trashBtn = new QPushButton("\xf0\x9f\x97\x91", m_recentList->viewport());
    m_trashBtn->setFixedSize(26, 26);
    m_trashBtn->setToolTip("Delete project and files");
    m_trashBtn->setStyleSheet(
        "QPushButton { background: #c0392b; color: white; border: none; "
        "border-radius: 4px; font-size: 12px; }"
        "QPushButton:hover { background: #e74c3c; }");
    m_trashBtn->hide();
    connect(m_trashBtn, &QPushButton::clicked, this, &WelcomeScreen::onDeleteProject);

    connect(m_recentList, &QListWidget::itemEntered, [this](QListWidgetItem* item) {
        m_hoveredItem = item;
        QRect r = m_recentList->visualItemRect(item);
        m_trashBtn->move(m_recentList->viewport()->width() - 32,
                         r.top() + (r.height() - 26) / 2);
        m_trashBtn->show();
        m_trashBtn->raise();
    });

    // Empty state
    auto* empty = new QLabel("No recent projects.\nCreate or open a project to get started.");
    empty->setObjectName("emptyState");
    empty->setAlignment(Qt::AlignCenter);
    empty->setStyleSheet("color: #3a3a3a; font-size: 13px;");
    empty->setVisible(false);
    rl->addWidget(empty);

    auto* bot = new QHBoxLayout();
    bot->addStretch();
    auto* openSel = new QPushButton("Open Selected");
    openSel->setObjectName("primary");
    openSel->setFixedHeight(34);
    openSel->setFixedWidth(140);
    bot->addWidget(openSel);
    rl->addLayout(bot);

    root->addWidget(right);

    // Connections
    connect(newBtn,      &QPushButton::clicked, this, &WelcomeScreen::onNewProject);
    connect(openBtn,     &QPushButton::clicked, this, &WelcomeScreen::onOpenProject);
    connect(openFileBtn, &QPushButton::clicked, this, &WelcomeScreen::onOpenFile);
    connect(removeBtn,   &QPushButton::clicked, this, &WelcomeScreen::onRemoveRecent);
    connect(openSel,   &QPushButton::clicked, this, &WelcomeScreen::onOpenSelected);
    connect(m_recentList, &QListWidget::itemDoubleClicked,
            this, &WelcomeScreen::onRecentDoubleClicked);

    loadRecents();

    // Show empty state if needed
    bool hasItems = m_recentList->count() > 0;
    m_recentList->setVisible(hasItems);
    empty->setVisible(!hasItems);
}

bool WelcomeScreen::eventFilter(QObject* obj, QEvent* event) {
    if (obj == m_recentList->viewport()) {
        if (event->type() == QEvent::Leave) {
            m_trashBtn->hide();
            m_hoveredItem = nullptr;
        }
        if (event->type() == QEvent::Resize) {
            if (m_hoveredItem && m_trashBtn->isVisible()) {
                QRect r = m_recentList->visualItemRect(m_hoveredItem);
                m_trashBtn->move(m_recentList->viewport()->width() - 32,
                                 r.top() + (r.height() - 26) / 2);
            }
        }
    }
    return QDialog::eventFilter(obj, event);
}

void WelcomeScreen::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton)
        m_dragPos = e->globalPos() - frameGeometry().topLeft();
    QDialog::mousePressEvent(e);
}

void WelcomeScreen::mouseMoveEvent(QMouseEvent* e) {
    if (e->buttons() & Qt::LeftButton)
        move(e->globalPos() - m_dragPos);
    QDialog::mouseMoveEvent(e);
}

void WelcomeScreen::updateEmptyState() {
    bool has = m_recentList->count() > 0;
    m_recentList->setVisible(has);
    auto* empty = findChild<QLabel*>("emptyState");
    if (empty) empty->setVisible(!has);
}

void WelcomeScreen::loadRecents() {
    m_recentList->clear();
    QSettings s("AnoDelta", "KonEditor");
    for (const QString& path : s.value("recentProjects").toStringList()) {
        if (!QFile::exists(path)) continue;
        QFileInfo fi(path);
        QString name = fi.baseName();
        QFile f(path);
        if (f.open(QIODevice::ReadOnly)) {
            auto doc = QJsonDocument::fromJson(f.readAll());
            if (doc.isObject())
                name = doc.object().value("name").toString(name);
        }
        auto* item = new QListWidgetItem(name + "\n" + fi.absolutePath());
        item->setData(Qt::UserRole, path);
        item->setToolTip(path);
        m_recentList->addItem(item);
    }
    updateEmptyState();
}

void WelcomeScreen::addRecent(const QString& path) {
    QSettings s("AnoDelta", "KonEditor");
    QStringList r = s.value("recentProjects").toStringList();
    r.removeAll(path);
    r.prepend(path);
    if (r.size() > 12) r = r.mid(0, 12);
    s.setValue("recentProjects", r);
}

void WelcomeScreen::onNewProject() {
    QString dir = QFileDialog::getExistingDirectory(this, "New Project",
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
    if (dir.isEmpty()) return;

    bool ok;
    QString name = QInputDialog::getText(this, "Project Name", "Name:",
        QLineEdit::Normal, QDir(dir).dirName(), &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    QString projDir = dir + "/" + name;

    ProjectManager pm;
    if (!pm.create(projDir)) {
        QMessageBox::critical(this, "Error", "Failed to create project in:\n" + projDir);
        return;
    }

    addRecent(pm.path());
    m_selectedProject = pm.path();
    accept();
}

void WelcomeScreen::onOpenProject() {
    QString path = QFileDialog::getOpenFileName(this, "Open Project",
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation),
        "KonScript Project (*.konproj);;All Files (*)");
    if (path.isEmpty()) return;
    addRecent(path);
    m_selectedProject = path;
    accept();
}

void WelcomeScreen::onOpenFile() {
    QString path = QFileDialog::getOpenFileName(this, "Open KonScript File",
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation),
        "KonScript Files (*.ks);;All Files (*)");
    if (path.isEmpty()) return;
    m_selectedProject = path;
    accept();
}

void WelcomeScreen::onRecentDoubleClicked(QListWidgetItem* item) {
    m_selectedProject = item->data(Qt::UserRole).toString();
    accept();
}

void WelcomeScreen::onOpenSelected() {
    auto* item = m_recentList->currentItem();
    if (!item) return;
    m_selectedProject = item->data(Qt::UserRole).toString();
    accept();
}

void WelcomeScreen::onRemoveRecent() {
    auto* item = m_recentList->currentItem();
    if (!item) return;
    QSettings s("AnoDelta", "KonEditor");
    QStringList r = s.value("recentProjects").toStringList();
    r.removeAll(item->data(Qt::UserRole).toString());
    s.setValue("recentProjects", r);
    delete item;
    updateEmptyState();
}

void WelcomeScreen::onDeleteProject() {
    QListWidgetItem* item = m_hoveredItem ? m_hoveredItem : m_recentList->currentItem();
    if (!item) return;
    m_trashBtn->hide();

    QString path = item->data(Qt::UserRole).toString();
    QString name = item->text().split('\n').first();
    QString dir  = QFileInfo(path).absolutePath();

    if (QMessageBox::warning(this, "Delete Project",
        "Delete \"" + name + "\" and all its files?\n\n" + dir + "\n\nThis cannot be undone.",
        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Yes)
        return;

    QSettings s("AnoDelta", "KonEditor");
    QStringList r = s.value("recentProjects").toStringList();
    r.removeAll(path);
    s.setValue("recentProjects", r);
    QDir(dir).removeRecursively();
    m_hoveredItem = nullptr;
    delete item;
    updateEmptyState();
}
