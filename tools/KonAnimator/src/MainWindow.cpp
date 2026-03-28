#include "MainWindow.hpp"
#include "AnimIO.hpp"

#include <QMenuBar>
#include <QStatusBar>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QHeaderView>
#include <algorithm>
#include <cmath>

// -----------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("KonAnimator");
    setMinimumSize(1280, 720);
    resize(1440, 860);
    buildMenuBar();
    buildUI();
    statusBar()->showMessage("Ready  —  File → New or Open to start");
    onNew();
}

void MainWindow::openFile(const QString& path) {
    std::string err;
    bool ok = false;
    QString ext = QFileInfo(path).suffix().toLower();
    if (ext == "konani")
        ok = AnimIO::loadKonani(path.toStdString(), m_proj, err);
    else
        ok = AnimIO::load(path.toStdString(), m_proj, err);

    if (!ok) {
        QMessageBox::critical(this, "Load Error", QString::fromStdString(err));
        return;
    }

    if (ext == "konani") {
        m_proj.dirty = true;
        statusBar()->showMessage(
            "Opened (decompiled): " + path +
            "  --  Save as .anim to keep an editable copy.");
    }
    m_clipIdx = m_frameIdx = m_trackIdx = m_keyIdx = -1;
    refreshClipList();
    if (!m_proj.spritesheetPath.empty()) {
        QString sp = QString::fromStdString(m_proj.spritesheetPath);
        QPixmap px(sp);
        if (!px.isNull()) {
            m_sheetView->setPixmap(px);
            m_preview->setSpritesheetPath(sp);
        }
    }
    updateTitle();
    statusBar()->showMessage("Opened: " + path);
}

// -----------------------------------------------------------------------
// Menu bar
// -----------------------------------------------------------------------
void MainWindow::buildMenuBar() {
    auto* file = menuBar()->addMenu("&File");
    file->addAction("&New",              this, &MainWindow::onNew,             QKeySequence::New);
    file->addAction("&Open...",          this, &MainWindow::onOpen,            QKeySequence::Open);
    file->addSeparator();
    file->addAction("&Save",             this, &MainWindow::onSave,            QKeySequence::Save);
    file->addAction("Save &As...",       this, &MainWindow::onSaveAs,          QKeySequence::SaveAs);
    file->addSeparator();
    file->addAction("Load &Spritesheet...", this, &MainWindow::onLoadSpritesheet);
    file->addSeparator();
    file->addAction("&Compile .konani",  this, &MainWindow::onCompile,         QKeySequence("Ctrl+B"));
    file->addSeparator();
    file->addAction("&Quit",             this, &QWidget::close,                QKeySequence::Quit);

    auto* anim = menuBar()->addMenu("&Animation");
    anim->addAction("Add Clip",          this, &MainWindow::onAddClip);
    anim->addAction("Remove Clip",       this, &MainWindow::onRemoveClip);
    anim->addSeparator();
    anim->addAction("Add Frame (Manual)",this, &MainWindow::onAddFrameManual);
    anim->addAction("Remove Frame",      this, &MainWindow::onRemoveFrame);
    anim->addSeparator();
    anim->addAction("Add Track",         this, &MainWindow::onAddTrack);
    anim->addAction("Remove Track",      this, &MainWindow::onRemoveTrack);
    anim->addSeparator();
    anim->addAction("Add Keyframe",      this, &MainWindow::onAddKeyframe);
    anim->addAction("Remove Keyframe",   this, &MainWindow::onRemoveKeyframe);
}

// -----------------------------------------------------------------------
// UI layout
// -----------------------------------------------------------------------
void MainWindow::buildUI() {
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(4,4,4,4);
    root->setSpacing(4);

    auto* hSplit = new QSplitter(Qt::Horizontal);
    hSplit->addWidget(buildLeftPanel());
    hSplit->addWidget(buildCenterPanel());
    hSplit->addWidget(buildRightPanel());
    hSplit->setStretchFactor(0, 2);
    hSplit->setStretchFactor(1, 4);
    hSplit->setStretchFactor(2, 2);

    root->addWidget(hSplit, 1);
    root->addWidget(buildBottomPanel());
}

// -----------------------------------------------------------------------
// Left panel — clips + clip settings + frames
// -----------------------------------------------------------------------
QWidget* MainWindow::buildLeftPanel() {
    auto* w = new QWidget;
    auto* vl = new QVBoxLayout(w);
    vl->setContentsMargins(4,4,4,4);
    vl->setSpacing(6);

    // --- Clip list ---
    {
        auto* grp = new QGroupBox("Animations");
        auto* gl  = new QVBoxLayout(grp);
        m_clipList = new QListWidget;
        m_clipList->setMaximumHeight(150);
        connect(m_clipList, &QListWidget::currentRowChanged, this, &MainWindow::onClipSelected);
        gl->addWidget(m_clipList);

        auto* row = new QHBoxLayout;
        auto* add = new QPushButton("+"); add->setFixedWidth(30);
        auto* rem = new QPushButton("−"); rem->setFixedWidth(30);
        connect(add, &QPushButton::clicked, this, &MainWindow::onAddClip);
        connect(rem, &QPushButton::clicked, this, &MainWindow::onRemoveClip);
        row->addWidget(add); row->addWidget(rem); row->addStretch();
        gl->addLayout(row);
        vl->addWidget(grp);
    }

    // --- Clip settings ---
    {
        auto* grp = new QGroupBox("Clip Settings");
        auto* gl  = new QGridLayout(grp);
        int r = 0;

        gl->addWidget(new QLabel("Name:"), r, 0);
        m_clipName = new QLineEdit;
        connect(m_clipName, &QLineEdit::editingFinished, this, &MainWindow::onClipNameChanged);
        gl->addWidget(m_clipName, r++, 1);

        m_clipLoop = new QCheckBox("Loop");
        connect(m_clipLoop, &QCheckBox::toggled, this, &MainWindow::onClipSettingsChanged);
        gl->addWidget(m_clipLoop, r++, 0, 1, 2);

        auto spin = [](double min, double max, double val, double step=1.0) {
            auto* s = new QDoubleSpinBox;
            s->setRange(min,max); s->setValue(val); s->setSingleStep(step);
            return s;
        };

        gl->addWidget(new QLabel("Display W:"), r, 0);
        m_dispW = spin(1,4096,32);
        connect(m_dispW, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onClipSettingsChanged);
        gl->addWidget(m_dispW, r++, 1);

        gl->addWidget(new QLabel("Display H:"), r, 0);
        m_dispH = spin(1,4096,32);
        connect(m_dispH, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onClipSettingsChanged);
        gl->addWidget(m_dispH, r++, 1);

        gl->addWidget(new QLabel("Scale:"), r, 0);
        m_dispScale = spin(0.01,100,1.0,0.1);
        connect(m_dispScale, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onClipSettingsChanged);
        gl->addWidget(m_dispScale, r++, 1);

        vl->addWidget(grp);
    }

    // --- Frame list ---
    {
        auto* grp = new QGroupBox("Frames");
        auto* gl  = new QVBoxLayout(grp);

        m_frameTable = new QTableWidget(0, 5);
        m_frameTable->setHorizontalHeaderLabels({"X","Y","W","H","Dur"});
        m_frameTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        m_frameTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_frameTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_frameTable->setMaximumHeight(140);
        connect(m_frameTable, &QTableWidget::itemSelectionChanged, this, &MainWindow::onFrameTableSelected);
        gl->addWidget(m_frameTable);

        auto* row = new QHBoxLayout;
        auto* add = new QPushButton("+ Manual");
        auto* rem = new QPushButton("−"); rem->setFixedWidth(30);
        connect(add, &QPushButton::clicked, this, &MainWindow::onAddFrameManual);
        connect(rem, &QPushButton::clicked, this, &MainWindow::onRemoveFrame);
        row->addWidget(add); row->addWidget(rem); row->addStretch();
        gl->addLayout(row);

        // Frame property spinboxes
        auto* pg = new QGridLayout;
        int pr = 0;
        auto spin = [](double max){ auto* s = new QDoubleSpinBox; s->setRange(0,max); return s; };
        pg->addWidget(new QLabel("Src X:"), pr,0); m_fSrcX = spin(9999); pg->addWidget(m_fSrcX, pr++,1);
        pg->addWidget(new QLabel("Src Y:"), pr,0); m_fSrcY = spin(9999); pg->addWidget(m_fSrcY, pr++,1);
        pg->addWidget(new QLabel("Src W:"), pr,0); m_fSrcW = spin(9999); m_fSrcW->setMinimum(1); pg->addWidget(m_fSrcW, pr++,1);
        pg->addWidget(new QLabel("Src H:"), pr,0); m_fSrcH = spin(9999); m_fSrcH->setMinimum(1); pg->addWidget(m_fSrcH, pr++,1);
        pg->addWidget(new QLabel("Dur:"),  pr,0);
        m_fDur = new QDoubleSpinBox; m_fDur->setRange(0.001,60); m_fDur->setSingleStep(0.05); m_fDur->setValue(0.1);
        pg->addWidget(m_fDur, pr++,1);
        gl->addLayout(pg);

        for (auto* s : {m_fSrcX,m_fSrcY,m_fSrcW,m_fSrcH,m_fDur})
            connect(s, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onFramePropChanged);

        vl->addWidget(grp);
    }

    vl->addStretch();
    w->setMinimumWidth(230);
    return w;
}

// -----------------------------------------------------------------------
// Center panel — spritesheet view
// -----------------------------------------------------------------------
QWidget* MainWindow::buildCenterPanel() {
    auto* w  = new QWidget;
    auto* vl = new QVBoxLayout(w);
    vl->setContentsMargins(4,4,4,4);

    // Toolbar
    auto* toolbar = new QHBoxLayout;
    auto* loadBtn = new QPushButton("Load Spritesheet...");
    connect(loadBtn, &QPushButton::clicked, this, &MainWindow::onLoadSpritesheet);
    toolbar->addWidget(loadBtn);
    toolbar->addWidget(new QLabel("Zoom:"));
    m_zoomSlider = new QSlider(Qt::Horizontal);
    m_zoomSlider->setRange(1, 32); m_zoomSlider->setValue(4);
    m_zoomSlider->setMaximumWidth(120);
    connect(m_zoomSlider, &QSlider::valueChanged, [this](int v){
        m_sheetView->setZoom(v * 0.5f);
    });
    toolbar->addWidget(m_zoomSlider);
    toolbar->addStretch();
    vl->addLayout(toolbar);

    // Sheet view in scroll area
    auto* scroll = new QScrollArea;
    m_sheetView  = new SpritesheetView;
    connect(m_sheetView, &SpritesheetView::frameAdded,   this, &MainWindow::onFrameAdded);
    connect(m_sheetView, &SpritesheetView::frameClicked, this, &MainWindow::onFrameClicked);
    scroll->setWidget(m_sheetView);
    scroll->setWidgetResizable(true);
    vl->addWidget(scroll, 1);

    return w;
}

// -----------------------------------------------------------------------
// Right panel — preview (vertically resizable) + tracks + keyframes
// -----------------------------------------------------------------------
QWidget* MainWindow::buildRightPanel() {
    auto* w  = new QWidget;
    auto* vl = new QVBoxLayout(w);
    vl->setContentsMargins(4,4,4,4);
    vl->setSpacing(4);

    auto* vSplit = new QSplitter(Qt::Vertical);

    // ---- Top: preview ----
    {
        auto* top = new QWidget;
        auto* tl  = new QVBoxLayout(top);
        tl->setContentsMargins(0,0,0,0);
        tl->setSpacing(4);

        // Header row: label + zoom buttons + fullscreen button
        auto* header = new QHBoxLayout;
        header->addWidget(new QLabel("<b>Preview</b>"));
        header->addStretch();

        auto* zoomOut   = new QPushButton("−");   zoomOut->setFixedSize(22,22);
        auto* zoomIn    = new QPushButton("+");    zoomIn->setFixedSize(22,22);
        auto* zoomReset = new QPushButton("1:1");  zoomReset->setFixedHeight(22);
        auto* fsBtn     = new QPushButton("⛶");   fsBtn->setFixedSize(22,22);
        zoomOut->setToolTip("Zoom out  (−)");
        zoomIn->setToolTip("Zoom in  (+)");
        zoomReset->setToolTip("Reset zoom & pan  (double-click or 0)");
        fsBtn->setToolTip("Toggle fullscreen  (F)");

        header->addWidget(zoomOut);
        header->addWidget(zoomIn);
        header->addWidget(zoomReset);
        header->addWidget(fsBtn);
        tl->addLayout(header);

        m_preview = new PreviewWidget;
        m_preview->setMinimumHeight(120);
        connect(m_preview,  &PreviewWidget::frameChanged,   this, &MainWindow::onFrameChanged);
        connect(m_preview,  &PreviewWidget::elapsedChanged, this, &MainWindow::onElapsedChanged);
        connect(zoomIn,     &QPushButton::clicked, m_preview, &PreviewWidget::zoomIn);
        connect(zoomOut,    &QPushButton::clicked, m_preview, &PreviewWidget::zoomOut);
        connect(zoomReset,  &QPushButton::clicked, m_preview, &PreviewWidget::resetView);
        connect(fsBtn,      &QPushButton::clicked, m_preview, &PreviewWidget::toggleFullscreen);
        tl->addWidget(m_preview, 1);

        // Playback controls
        auto* row = new QHBoxLayout;
        m_playBtn = new QPushButton("▶ Play"); m_playBtn->setFixedWidth(80);
        connect(m_playBtn, &QPushButton::clicked, this, &MainWindow::onPlayPause);
        auto* stopBtn = new QPushButton("■"); stopBtn->setFixedWidth(30);
        connect(stopBtn, &QPushButton::clicked, this, &MainWindow::onStop);
        m_timeLabel = new QLabel("0.000s");
        row->addWidget(m_playBtn); row->addWidget(stopBtn);
        row->addWidget(m_timeLabel); row->addStretch();
        tl->addLayout(row);

        vSplit->addWidget(top);
    }

    // ---- Bottom: tracks + keyframe props ----
    {
        auto* bot = new QWidget;
        auto* bl  = new QVBoxLayout(bot);
        bl->setContentsMargins(0,0,0,0);
        bl->setSpacing(6);

        // Tracks
        {
            auto* grp = new QGroupBox("Keyframe Tracks");
            auto* gl  = new QVBoxLayout(grp);
            m_trackList = new QListWidget;
            connect(m_trackList, &QListWidget::currentRowChanged, this, &MainWindow::onTrackSelected);
            gl->addWidget(m_trackList, 1);
            auto* row = new QHBoxLayout;
            auto* add = new QPushButton("+ Track");
            auto* rem = new QPushButton("−"); rem->setFixedWidth(30);
            connect(add, &QPushButton::clicked, this, &MainWindow::onAddTrack);
            connect(rem, &QPushButton::clicked, this, &MainWindow::onRemoveTrack);
            row->addWidget(add); row->addWidget(rem); row->addStretch();
            gl->addLayout(row);
            bl->addWidget(grp, 1);
        }

        // Keyframe props
        {
            auto* grp = new QGroupBox("Keyframe");
            auto* gl  = new QGridLayout(grp);
            int r = 0;

            gl->addWidget(new QLabel("Time:"), r, 0);
            m_kfTime = new QDoubleSpinBox;
            m_kfTime->setRange(0,9999); m_kfTime->setSingleStep(0.05);
            connect(m_kfTime, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                    this, &MainWindow::onKFPropChanged);
            gl->addWidget(m_kfTime, r++, 1);

            gl->addWidget(new QLabel("Value:"), r, 0);
            m_kfValue = new QDoubleSpinBox;
            m_kfValue->setRange(-99999,99999); m_kfValue->setSingleStep(0.1);
            connect(m_kfValue, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                    this, &MainWindow::onKFPropChanged);
            gl->addWidget(m_kfValue, r++, 1);

            gl->addWidget(new QLabel("Curve:"), r, 0);
            m_kfCurve = new QComboBox;
            for (int i = 0; i < kEaseCount; i++) m_kfCurve->addItem(kEaseNames[i]);
            connect(m_kfCurve, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this, &MainWindow::onKFPropChanged);
            gl->addWidget(m_kfCurve, r++, 1);

            auto* addKF = new QPushButton("Add Keyframe at Playhead");
            connect(addKF, &QPushButton::clicked, this, &MainWindow::onAddKeyframe);
            gl->addWidget(addKF, r++, 0, 1, 2);

            auto* remKF = new QPushButton("Remove Keyframe");
            connect(remKF, &QPushButton::clicked, this, &MainWindow::onRemoveKeyframe);
            gl->addWidget(remKF, r++, 0, 1, 2);

            bl->addWidget(grp);
        }

        vSplit->addWidget(bot);
    }

    vSplit->setStretchFactor(0, 3);
    vSplit->setStretchFactor(1, 2);

    vl->addWidget(vSplit, 1);
    w->setMinimumWidth(220);
    return w;
}

// -----------------------------------------------------------------------
// Bottom panel — timeline
// -----------------------------------------------------------------------
QWidget* MainWindow::buildBottomPanel() {
    auto* w  = new QWidget;
    auto* vl = new QVBoxLayout(w);
    vl->setContentsMargins(4,2,4,2);
    vl->setSpacing(2);

    auto* scroll = new QScrollArea;
    m_timeline   = new TimelineWidget;
    connect(m_timeline, &TimelineWidget::playheadChanged,  [this](float t){
        m_preview->setPlayhead(t); // not a method yet — handled via stop+seek
        m_timeLabel->setText(QString::number(t,'f',3)+"s");
    });
    connect(m_timeline, &TimelineWidget::keyframeSelected, this, &MainWindow::onKeyframeSelected);
    connect(m_timeline, &TimelineWidget::keyframeMoved,    this, &MainWindow::onKeyframeMoved);
    connect(m_timeline, &TimelineWidget::clipEdited, [this]{
        m_proj.dirty = true; updateTitle();
    });
    scroll->setWidget(m_timeline);
    scroll->setWidgetResizable(true);
    scroll->setMaximumHeight(200);
    vl->addWidget(scroll);

    w->setMaximumHeight(210);
    return w;
}

// -----------------------------------------------------------------------
// File operations
// -----------------------------------------------------------------------
void MainWindow::onNew() {
    if (!confirmDiscard()) return;
    m_proj = AnimProject{};
    m_clipIdx = m_frameIdx = m_trackIdx = m_keyIdx = -1;
    refreshClipList();
    m_sheetView->setPixmap({});
    m_sheetView->setFrames(nullptr);
    m_timeline->setClip(nullptr);
    m_preview->setClip(nullptr);
    updateTitle();
}

void MainWindow::onOpen() {
    if (!confirmDiscard()) return;
    QString path = QFileDialog::getOpenFileName(this, "Open Animation", {},
        "Animation Files (*.anim *.konani);;"
        "Anim Source (*.anim);;"
        "Compiled Anim (*.konani);;"
        "All Files (*)");
    if (path.isEmpty()) return;
    openFile(path);
}

void MainWindow::onSave() {
    if (m_proj.filePath.empty()) { onSaveAs(); return; }
    std::string err;
    if (!AnimIO::save(m_proj, err))
        QMessageBox::critical(this, "Save Error", QString::fromStdString(err));
    else {
        m_proj.dirty = false;
        updateTitle();
        statusBar()->showMessage("Saved: " + QString::fromStdString(m_proj.filePath));
    }
}

void MainWindow::onSaveAs() {
    QString path = QFileDialog::getSaveFileName(this, "Save .anim", {},
        "Anim files (*.anim)");
    if (path.isEmpty()) return;
    if (!path.endsWith(".anim")) path += ".anim";
    m_proj.filePath = path.toStdString();
    onSave();
}

void MainWindow::onLoadSpritesheet() {
    QString path = QFileDialog::getOpenFileName(this, "Load Spritesheet", {},
        "Images (*.png *.jpg *.bmp);;All files (*)");
    if (path.isEmpty()) return;
    QPixmap px(path);
    if (px.isNull()) { QMessageBox::warning(this,"Error","Failed to load image."); return; }
    m_sheetView->setPixmap(px);
    m_preview->setSpritesheetPath(path);
    m_proj.spritesheetPath = path.toStdString();
    m_proj.dirty = true;
    updateTitle();
    statusBar()->showMessage("Spritesheet: " + path);
}

void MainWindow::onCompile() {
    if (m_proj.filePath.empty()) {
        QMessageBox::warning(this,"Save First","Save the project before compiling.");
        return;
    }
    std::string out = AnimIO::konaniPath(m_proj.filePath);
    std::string err;
    if (!AnimIO::compile(m_proj, out, err))
        QMessageBox::critical(this,"Compile Error",QString::fromStdString(err));
    else
        statusBar()->showMessage("Compiled → " + QString::fromStdString(out));
}

// -----------------------------------------------------------------------
// Clip operations
// -----------------------------------------------------------------------
void MainWindow::onAddClip() {
    AnimClip c; c.name = "anim_" + std::to_string(m_proj.clips.size());
    m_proj.clips.push_back(c);
    m_proj.dirty = true;
    refreshClipList();
    m_clipList->setCurrentRow((int)m_proj.clips.size()-1);
    updateTitle();
}

void MainWindow::onRemoveClip() {
    if (!clip()) return;
    m_proj.clips.erase(m_proj.clips.begin()+m_clipIdx);
    m_proj.dirty = true;
    m_clipIdx = -1;
    refreshClipList();
    updateTitle();
}

void MainWindow::onClipSelected(int row) {
    m_clipIdx  = row;
    m_frameIdx = m_trackIdx = m_keyIdx = -1;
    refreshClipSettings();
    refreshFrameTable();
    refreshTrackList();
    if (auto* c = clip()) {
        m_sheetView->setFrames(&c->frames);
        m_timeline->setClip(c);
        m_preview->setClip(c);
    } else {
        m_sheetView->setFrames(nullptr);
        m_timeline->setClip(nullptr);
        m_preview->setClip(nullptr);
    }
}

void MainWindow::onClipNameChanged() {
    auto* c = clip(); if (!c) return;
    c->name = m_clipName->text().toStdString();
    m_suppress = true; refreshClipList(); m_suppress = false;
    m_proj.dirty = true; updateTitle();
}

void MainWindow::onClipSettingsChanged() {
    if (m_suppress) return;
    auto* c = clip(); if (!c) return;
    c->loop         = m_clipLoop->isChecked();
    c->displayW     = (float)m_dispW->value();
    c->displayH     = (float)m_dispH->value();
    c->displayScale = (float)m_dispScale->value();
    m_proj.dirty = true; updateTitle();
}

// -----------------------------------------------------------------------
// Frame operations
// -----------------------------------------------------------------------
void MainWindow::onFrameTableSelected() {
    int row = m_frameTable->currentRow();
    m_frameIdx = row;
    m_sheetView->setSelectedFrame(row);
    refreshFrameProps();
}

void MainWindow::onAddFrameManual() {
    auto* c = clip(); if (!c) return;
    c->frames.push_back({0,0,32,32,0.1f});
    m_proj.dirty = true;
    refreshFrameTable();
    m_frameTable->selectRow((int)c->frames.size()-1);
    updateTitle();
}

void MainWindow::onRemoveFrame() {
    auto* c = clip(); if (!c || m_frameIdx<0 || m_frameIdx>=(int)c->frames.size()) return;
    c->frames.erase(c->frames.begin()+m_frameIdx);
    m_proj.dirty = true;
    refreshFrameTable(); updateTitle();
}

void MainWindow::onFrameAdded(AnimFrame fr) {
    auto* c = clip(); if (!c) return;
    c->frames.push_back(fr);
    m_proj.dirty = true;
    refreshFrameTable();
    m_frameTable->selectRow((int)c->frames.size()-1);
    updateTitle();
}

void MainWindow::onFrameClicked(int idx) {
    m_frameIdx = idx;
    m_frameTable->selectRow(idx);
    refreshFrameProps();
}

void MainWindow::onFramePropChanged() {
    if (m_suppress) return;
    auto* c = clip(); if (!c || m_frameIdx<0 || m_frameIdx>=(int)c->frames.size()) return;
    auto& fr = c->frames[m_frameIdx];
    fr.srcX     = (float)m_fSrcX->value();
    fr.srcY     = (float)m_fSrcY->value();
    fr.srcW     = (float)m_fSrcW->value();
    fr.srcH     = (float)m_fSrcH->value();
    fr.duration = (float)m_fDur->value();
    m_proj.dirty = true;
    m_suppress = true; refreshFrameTable(); m_suppress = false;
    m_sheetView->update();
    updateTitle();
}

// -----------------------------------------------------------------------
// Track / keyframe operations
// -----------------------------------------------------------------------
void MainWindow::onAddTrack() {
    auto* c = clip(); if (!c) return;
    QStringList props = {"x","y","scaleX","scaleY","rotation","alpha"};
    bool ok;
    QString name = QInputDialog::getItem(this,"Add Track","Property:",props,0,false,&ok);
    if (!ok || name.isEmpty()) return;
    c->getOrAddTrack(name.toStdString());
    m_proj.dirty = true;
    refreshTrackList();
    m_timeline->setClip(c);
    updateTitle();
}

void MainWindow::onRemoveTrack() {
    auto* c = clip(); if (!c || m_trackIdx<0 || m_trackIdx>=(int)c->tracks.size()) return;
    c->tracks.erase(c->tracks.begin()+m_trackIdx);
    m_proj.dirty = true; m_trackIdx = -1;
    refreshTrackList();
    m_timeline->setClip(c);
    updateTitle();
}

void MainWindow::onTrackSelected(int row) {
    m_trackIdx = row;
    m_keyIdx   = -1;
    refreshKFProps();
}

void MainWindow::onAddKeyframe() {
    auto* c = clip(); if (!c || m_trackIdx<0 || m_trackIdx>=(int)c->tracks.size()) return;
    float t = m_timeline->playhead();
    c->tracks[m_trackIdx].keys.push_back({t, 0.0f, Ease::EaseInOut});
    c->tracks[m_trackIdx].sortKeys();
    m_proj.dirty = true;
    m_timeline->setClip(c);
    updateTitle();
}

void MainWindow::onRemoveKeyframe() {
    auto* c = clip(); if (!c || m_trackIdx<0 || m_keyIdx<0) return;
    auto& keys = c->tracks[m_trackIdx].keys;
    if (m_keyIdx >= (int)keys.size()) return;
    keys.erase(keys.begin()+m_keyIdx);
    m_proj.dirty = true; m_keyIdx = -1;
    m_timeline->setClip(c);
    updateTitle();
}

void MainWindow::onKeyframeSelected(int ti, int ki) {
    m_trackIdx = ti; m_keyIdx = ki;
    m_trackList->setCurrentRow(ti);
    refreshKFProps();
}

void MainWindow::onKeyframeMoved(int ti, int ki, float t) {
    auto* c = clip(); if (!c) return;
    if (ti<0 || ti>=(int)c->tracks.size()) return;
    if (ki<0 || ki>=(int)c->tracks[ti].keys.size()) return;
    c->tracks[ti].keys[ki].time = t;
    m_proj.dirty = true; updateTitle();
}

void MainWindow::onKFPropChanged() {
    if (m_suppress) return;
    auto* c = clip(); if (!c || m_trackIdx<0 || m_keyIdx<0) return;
    if (m_trackIdx>=(int)c->tracks.size()) return;
    if (m_keyIdx>=(int)c->tracks[m_trackIdx].keys.size()) return;
    auto& kf = c->tracks[m_trackIdx].keys[m_keyIdx];
    kf.time  = (float)m_kfTime->value();
    kf.value = (float)m_kfValue->value();
    kf.curve = static_cast<Ease>(m_kfCurve->currentIndex());
    m_proj.dirty = true;
    m_timeline->update(); updateTitle();
}

// -----------------------------------------------------------------------
// Playback
// -----------------------------------------------------------------------
void MainWindow::onPlayPause() {
    if (m_preview->isPlaying()) {
        m_preview->pause();
        m_playBtn->setText("▶ Play");
    } else {
        m_preview->play();
        m_playBtn->setText("⏸ Pause");
    }
}

void MainWindow::onStop() {
    m_preview->stop();
    m_playBtn->setText("▶ Play");
    m_timeline->setPlayhead(0.0f);
    m_timeLabel->setText("0.000s");
    m_sheetView->setHighlightFrame(-1);
}

void MainWindow::onFrameChanged(int idx) {
    m_sheetView->setHighlightFrame(idx);
}

void MainWindow::onElapsedChanged(float t) {
    m_timeline->setPlayhead(t);
    m_timeLabel->setText(QString::number(t,'f',3)+"s");
}

// -----------------------------------------------------------------------
// Refresh helpers
// -----------------------------------------------------------------------
void MainWindow::refreshClipList() {
    m_suppress = true;
    m_clipList->clear();
    for (auto& c : m_proj.clips)
        m_clipList->addItem(QString::fromStdString(c.name));
    if (m_clipIdx>=0 && m_clipIdx<m_clipList->count())
        m_clipList->setCurrentRow(m_clipIdx);
    m_suppress = false;
}

void MainWindow::refreshFrameTable() {
    m_suppress = true;
    m_frameTable->setRowCount(0);
    auto* c = clip();
    if (c) {
        for (int i=0; i<(int)c->frames.size(); i++) {
            m_frameTable->insertRow(i);
            auto& fr = c->frames[i];
            m_frameTable->setItem(i,0,new QTableWidgetItem(QString::number(fr.srcX)));
            m_frameTable->setItem(i,1,new QTableWidgetItem(QString::number(fr.srcY)));
            m_frameTable->setItem(i,2,new QTableWidgetItem(QString::number(fr.srcW)));
            m_frameTable->setItem(i,3,new QTableWidgetItem(QString::number(fr.srcH)));
            m_frameTable->setItem(i,4,new QTableWidgetItem(QString::number(fr.duration)));
        }
    }
    if (m_frameIdx>=0 && m_frameIdx<m_frameTable->rowCount())
        m_frameTable->selectRow(m_frameIdx);
    m_suppress = false;
    if (c) m_sheetView->setFrames(&c->frames);
}

void MainWindow::refreshTrackList() {
    m_suppress = true;
    m_trackList->clear();
    auto* c = clip();
    if (c)
        for (auto& t : c->tracks)
            m_trackList->addItem(QString::fromStdString(t.name) +
                " (" + QString::number(t.keys.size()) + ")");
    m_suppress = false;
}

void MainWindow::refreshClipSettings() {
    m_suppress = true;
    auto* c = clip();
    if (c) {
        m_clipName->setText(QString::fromStdString(c->name));
        m_clipLoop->setChecked(c->loop);
        m_dispW->setValue(c->displayW);
        m_dispH->setValue(c->displayH);
        m_dispScale->setValue(c->displayScale);
    }
    m_suppress = false;
}

void MainWindow::refreshFrameProps() {
    m_suppress = true;
    auto* c = clip();
    if (c && m_frameIdx>=0 && m_frameIdx<(int)c->frames.size()) {
        auto& fr = c->frames[m_frameIdx];
        m_fSrcX->setValue(fr.srcX); m_fSrcY->setValue(fr.srcY);
        m_fSrcW->setValue(fr.srcW); m_fSrcH->setValue(fr.srcH);
        m_fDur->setValue(fr.duration);
    }
    m_suppress = false;
}

void MainWindow::refreshKFProps() {
    m_suppress = true;
    auto* c = clip();
    if (c && m_trackIdx>=0 && m_trackIdx<(int)c->tracks.size()
          && m_keyIdx>=0   && m_keyIdx<(int)c->tracks[m_trackIdx].keys.size()) {
        auto& kf = c->tracks[m_trackIdx].keys[m_keyIdx];
        m_kfTime->setValue(kf.time);
        m_kfValue->setValue(kf.value);
        m_kfCurve->setCurrentIndex((int)kf.curve);
    }
    m_suppress = false;
}

// -----------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------
void MainWindow::updateTitle() {
    setWindowTitle((m_proj.dirty ? "* " : "") + projectName() + " — KonAnimator");
}

QString MainWindow::projectName() const {
    if (m_proj.filePath.empty()) return "Untitled";
    return QFileInfo(QString::fromStdString(m_proj.filePath)).fileName();
}

bool MainWindow::confirmDiscard() {
    if (!m_proj.dirty) return true;
    return QMessageBox::question(this,"Unsaved Changes",
        "Discard unsaved changes?",
        QMessageBox::Discard|QMessageBox::Cancel) == QMessageBox::Discard;
}
