#include "AnimatorPanel.hpp"
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QPixmap>
#include <QToolBar>
#include <QInputDialog>

AnimatorPanel::AnimatorPanel(QWidget* parent) : QWidget(parent) {
    setupUI();
}

AnimatorPanel::~AnimatorPanel() {}

void AnimatorPanel::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    setupToolbar(mainLayout);

    // Outer splitter: [top area] / [timeline]
    m_outerSplitter = new QSplitter(Qt::Vertical);

    // Top splitter: [spritesheet] / [preview] / [properties]
    m_topSplitter = new QSplitter(Qt::Horizontal);

    // Left: Spritesheet view
    m_spritesheetView = new SpritesheetView();
    m_spritesheetView->setMinimumWidth(200);
    m_topSplitter->addWidget(m_spritesheetView);

    // Center: Preview widget (OpenGL)
    m_previewWidget = new PreviewWidget();
    m_previewWidget->setMinimumWidth(300);
    m_topSplitter->addWidget(m_previewWidget);

    // Right: Keyframe property editor
    m_propsPanel = new QWidget();
    m_propsPanel->setMinimumWidth(200);
    m_propsPanel->setMaximumWidth(280);
    auto* propsLayout = new QVBoxLayout(m_propsPanel);
    propsLayout->setContentsMargins(8, 8, 8, 8);
    propsLayout->setSpacing(6);

    auto* propsHeader = new QLabel("Keyframe Properties");
    propsHeader->setStyleSheet("QLabel { color: #aaa; font-weight: bold; font-size: 12px; }");
    propsLayout->addWidget(propsHeader);

    m_kfInfoLabel = new QLabel("No keyframe selected");
    m_kfInfoLabel->setStyleSheet("QLabel { color: #666; font-size: 11px; }");
    propsLayout->addWidget(m_kfInfoLabel);

    auto* propsForm = new QFormLayout();
    propsForm->setSpacing(4);

    m_kfTimeSpin = new QDoubleSpinBox();
    m_kfTimeSpin->setRange(0.0, 999.0);
    m_kfTimeSpin->setDecimals(3);
    m_kfTimeSpin->setSingleStep(0.01);
    m_kfTimeSpin->setEnabled(false);
    propsForm->addRow("Time:", m_kfTimeSpin);

    m_kfValueSpin = new QDoubleSpinBox();
    m_kfValueSpin->setRange(-99999.0, 99999.0);
    m_kfValueSpin->setDecimals(3);
    m_kfValueSpin->setSingleStep(0.1);
    m_kfValueSpin->setEnabled(false);
    propsForm->addRow("Value:", m_kfValueSpin);

    m_kfCurveCombo = new QComboBox();
    for (int i = 0; i < kEaseCount; i++)
        m_kfCurveCombo->addItem(kEaseNames[i]);
    m_kfCurveCombo->setEnabled(false);
    propsForm->addRow("Curve:", m_kfCurveCombo);

    propsLayout->addLayout(propsForm);
    propsLayout->addStretch();

    m_topSplitter->addWidget(m_propsPanel);

    m_topSplitter->setStretchFactor(0, 1);  // spritesheet
    m_topSplitter->setStretchFactor(1, 2);  // preview
    m_topSplitter->setStretchFactor(2, 0);  // properties
    m_topSplitter->setSizes({250, 500, 220});

    m_outerSplitter->addWidget(m_topSplitter);

    // Bottom: Timeline
    m_timelineWidget = new TimelineWidget();
    m_timelineWidget->setMinimumHeight(120);
    m_outerSplitter->addWidget(m_timelineWidget);

    m_outerSplitter->setStretchFactor(0, 1);
    m_outerSplitter->setStretchFactor(1, 0);
    m_outerSplitter->setSizes({500, 180});

    mainLayout->addWidget(m_outerSplitter, 1);

    // Wire signals
    connect(m_spritesheetView, &SpritesheetView::frameClicked,
            this, &AnimatorPanel::onFrameClicked);

    connect(m_timelineWidget, &TimelineWidget::keyframeSelected,
            this, &AnimatorPanel::onKeyframeSelected);

    connect(m_timelineWidget, &TimelineWidget::keyframeMoved,
            this, &AnimatorPanel::onKeyframeMoved);

    connect(m_timelineWidget, &TimelineWidget::playheadChanged,
            this, &AnimatorPanel::onPlayheadChanged);

    connect(m_previewWidget, &PreviewWidget::elapsedChanged,
            [this](float t) { m_timelineWidget->setPlayhead(t); });

    // Keyframe property edits
    connect(m_kfTimeSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double val) {
                if (m_selTrack < 0 || m_selKey < 0) return;
                if (m_currentClip >= (int)m_project.clips.size()) return;
                auto& clip = m_project.clips[m_currentClip];
                if (m_selTrack >= (int)clip.tracks.size()) return;
                auto& track = clip.tracks[m_selTrack];
                if (m_selKey >= (int)track.keys.size()) return;
                track.keys[m_selKey].time = (float)val;
                track.sortKeys();
                m_project.dirty = true;
                m_timelineWidget->refreshClip();
            });

    connect(m_kfValueSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double val) {
                if (m_selTrack < 0 || m_selKey < 0) return;
                if (m_currentClip >= (int)m_project.clips.size()) return;
                auto& clip = m_project.clips[m_currentClip];
                if (m_selTrack >= (int)clip.tracks.size()) return;
                auto& track = clip.tracks[m_selTrack];
                if (m_selKey >= (int)track.keys.size()) return;
                track.keys[m_selKey].value = (float)val;
                m_project.dirty = true;
                m_timelineWidget->refreshClip();
            });

    connect(m_kfCurveCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this](int idx) {
                if (m_selTrack < 0 || m_selKey < 0) return;
                if (m_currentClip >= (int)m_project.clips.size()) return;
                auto& clip = m_project.clips[m_currentClip];
                if (m_selTrack >= (int)clip.tracks.size()) return;
                auto& track = clip.tracks[m_selTrack];
                if (m_selKey >= (int)track.keys.size()) return;
                track.keys[m_selKey].curve = static_cast<Ease>(idx);
                m_project.dirty = true;
                m_timelineWidget->refreshClip();
            });
}

void AnimatorPanel::setupToolbar(QVBoxLayout* mainLayout) {
    auto* toolbar = new QWidget();
    toolbar->setStyleSheet("QWidget { background: #242424; border-bottom: 1px solid #333; }");
    auto* tbLayout = new QHBoxLayout(toolbar);
    tbLayout->setContentsMargins(4, 2, 4, 2);
    tbLayout->setSpacing(4);

    auto makeBtn = [](const QString& text) {
        auto* btn = new QPushButton(text);
        btn->setFixedHeight(24);
        return btn;
    };

    auto* newBtn  = makeBtn("New");
    auto* openBtn = makeBtn("Open");
    auto* saveBtn = makeBtn("Save");
    tbLayout->addWidget(newBtn);
    tbLayout->addWidget(openBtn);
    tbLayout->addWidget(saveBtn);

    tbLayout->addWidget(new QLabel("  |  "));

    tbLayout->addWidget(new QLabel("Clip:"));
    m_clipCombo = new QComboBox();
    m_clipCombo->setFixedWidth(120);
    tbLayout->addWidget(m_clipCombo);

    tbLayout->addWidget(new QLabel("  |  "));

    auto* addTrackBtn = makeBtn("+ Track");
    auto* addKeyBtn   = makeBtn("+ Key");
    tbLayout->addWidget(addTrackBtn);
    tbLayout->addWidget(addKeyBtn);

    tbLayout->addWidget(new QLabel("  |  "));

    m_playBtn = makeBtn("Play");
    m_playBtn->setFixedWidth(60);
    tbLayout->addWidget(m_playBtn);

    tbLayout->addStretch();

    mainLayout->addWidget(toolbar);

    connect(newBtn,      &QPushButton::clicked, this, &AnimatorPanel::onNewAnimation);
    connect(openBtn,     &QPushButton::clicked, this, &AnimatorPanel::onOpenAnimation);
    connect(saveBtn,     &QPushButton::clicked, this, &AnimatorPanel::onSaveAnimation);
    connect(addTrackBtn, &QPushButton::clicked, this, &AnimatorPanel::onAddTrack);
    connect(addKeyBtn,   &QPushButton::clicked, this, &AnimatorPanel::onAddKeyframe);
    connect(m_playBtn,   &QPushButton::clicked, this, &AnimatorPanel::onPlayStop);
    connect(m_clipCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AnimatorPanel::onClipChanged);
}

// ── File Operations ──────────────────────────────────────────────────────

void AnimatorPanel::openFile(const QString& path) {
    std::string err;
    bool ok = false;

    if (path.endsWith(".konani"))
        ok = AnimIO::loadKonani(path.toStdString(), m_project, err);
    else
        ok = AnimIO::load(path.toStdString(), m_project, err);

    if (!ok) {
        QMessageBox::warning(this, "Error", QString::fromStdString(err));
        return;
    }

    m_project.filePath = path.toStdString();
    m_currentClip = 0;

    loadSpritesheet();
    rebuildClipSelector();
    refreshAll();

    emit titleChanged(QFileInfo(path).fileName());
}

void AnimatorPanel::newFile() {
    m_project = AnimProject{};
    m_project.clips.emplace_back();
    m_project.clips.back().name = "idle";
    m_project.dirty = true;
    m_currentClip = 0;

    m_spritesheetView->setPixmap(QPixmap());
    m_spritesheetView->setFrames(nullptr);
    rebuildClipSelector();
    refreshAll();

    emit titleChanged("New Animation");
}

void AnimatorPanel::saveFile() {
    if (m_project.filePath.empty()) {
        QString path = QFileDialog::getSaveFileName(this, "Save Animation", "",
            "Animation Files (*.anim)");
        if (path.isEmpty()) return;
        m_project.filePath = path.toStdString();
    }

    std::string err;
    if (!AnimIO::save(m_project, err)) {
        QMessageBox::warning(this, "Save Error", QString::fromStdString(err));
        return;
    }

    // Also compile to .konani
    std::string konaniOut = AnimIO::konaniPath(m_project.filePath);
    AnimIO::compile(m_project, konaniOut, err);

    m_project.dirty = false;
    emit titleChanged(QFileInfo(QString::fromStdString(m_project.filePath)).fileName());
}

bool AnimatorPanel::hasUnsavedChanges() const {
    return m_project.dirty;
}

// ── Slots ────────────────────────────────────────────────────────────────

void AnimatorPanel::onNewAnimation() {
    if (hasUnsavedChanges()) {
        auto r = QMessageBox::question(this, "Unsaved Changes",
            "You have unsaved changes. Create new animation anyway?",
            QMessageBox::Yes | QMessageBox::Cancel);
        if (r != QMessageBox::Yes) return;
    }
    newFile();
}

void AnimatorPanel::onOpenAnimation() {
    QString path = QFileDialog::getOpenFileName(this, "Open Animation", "",
        "Animation Files (*.anim *.konani)");
    if (!path.isEmpty())
        openFile(path);
}

void AnimatorPanel::onSaveAnimation() {
    saveFile();
}

void AnimatorPanel::onAddTrack() {
    if (m_project.clips.empty()) return;
    auto& clip = m_project.clips[m_currentClip];

    QStringList trackNames = {"x", "y", "rotation", "scaleX", "scaleY", "alpha"};
    bool ok;
    QString name = QInputDialog::getItem(this, "Add Track", "Property:", trackNames, 0, true, &ok);
    if (!ok || name.isEmpty()) return;

    clip.getOrAddTrack(name.toStdString());
    m_project.dirty = true;
    m_timelineWidget->refreshClip();
}

void AnimatorPanel::onAddKeyframe() {
    if (m_project.clips.empty()) return;
    auto& clip = m_project.clips[m_currentClip];
    if (clip.tracks.empty()) return;

    // Add keyframe at current playhead on the selected track (or first track)
    int trackIdx = m_selTrack >= 0 ? m_selTrack : 0;
    if (trackIdx >= (int)clip.tracks.size()) trackIdx = 0;

    float t = m_timelineWidget->playhead();
    clip.tracks[trackIdx].keys.push_back({t, 0.0f, Ease::Linear});
    clip.tracks[trackIdx].sortKeys();
    m_project.dirty = true;
    m_timelineWidget->refreshClip();
}

void AnimatorPanel::onPlayStop() {
    if (m_previewWidget->isPlaying()) {
        m_previewWidget->pause();
        m_playBtn->setText("Play");
    } else {
        m_previewWidget->play();
        m_playBtn->setText("Stop");
    }
}

void AnimatorPanel::onClipChanged(int index) {
    if (index < 0 || index >= (int)m_project.clips.size()) return;
    m_currentClip = index;
    refreshAll();
}

void AnimatorPanel::onFrameClicked(int idx) {
    m_spritesheetView->setSelectedFrame(idx);
    m_previewWidget->setPlayhead(0);
    if (m_currentClip < (int)m_project.clips.size()) {
        auto& clip = m_project.clips[m_currentClip];
        // Compute time offset for this frame
        float t = 0;
        for (int i = 0; i < idx && i < (int)clip.frames.size(); i++)
            t += clip.frames[i].duration;
        m_previewWidget->setPlayhead(t);
        m_timelineWidget->setPlayhead(t);
    }
}

void AnimatorPanel::onKeyframeSelected(int trackIdx, int keyIdx) {
    m_selTrack = trackIdx;
    m_selKey   = keyIdx;
    updateKeyframeProperties(trackIdx, keyIdx);
}

void AnimatorPanel::onKeyframeMoved(int trackIdx, int keyIdx, float newTime) {
    if (m_currentClip >= (int)m_project.clips.size()) return;
    auto& clip = m_project.clips[m_currentClip];
    if (trackIdx >= (int)clip.tracks.size()) return;
    auto& track = clip.tracks[trackIdx];
    if (keyIdx >= (int)track.keys.size()) return;

    track.keys[keyIdx].time = newTime;
    m_project.dirty = true;
    updateKeyframeProperties(trackIdx, keyIdx);
}

void AnimatorPanel::onPlayheadChanged(float t) {
    m_previewWidget->setPlayhead(t);
}

// ── Helpers ──────────────────────────────────────────────────────────────

void AnimatorPanel::rebuildClipSelector() {
    m_clipCombo->blockSignals(true);
    m_clipCombo->clear();
    for (auto& c : m_project.clips)
        m_clipCombo->addItem(QString::fromStdString(c.name));
    if (m_currentClip < (int)m_project.clips.size())
        m_clipCombo->setCurrentIndex(m_currentClip);
    m_clipCombo->blockSignals(false);
}

void AnimatorPanel::refreshAll() {
    if (m_project.clips.empty()) {
        m_previewWidget->setClip(nullptr);
        m_timelineWidget->setClip(nullptr);
        m_spritesheetView->setFrames(nullptr);
        return;
    }

    auto& clip = m_project.clips[m_currentClip];
    m_previewWidget->setClip(&clip);
    m_timelineWidget->setClip(&clip);
    m_spritesheetView->setFrames(&clip.frames);

    m_selTrack = -1;
    m_selKey   = -1;
    updateKeyframeProperties(-1, -1);
}

void AnimatorPanel::loadSpritesheet() {
    if (m_project.spritesheetPath.empty()) return;

    // Resolve spritesheet path relative to the .anim file
    QString sheetPath = QString::fromStdString(m_project.spritesheetPath);
    if (!QFileInfo(sheetPath).isAbsolute()) {
        QString animDir = QFileInfo(QString::fromStdString(m_project.filePath)).absolutePath();
        sheetPath = animDir + "/" + sheetPath;
    }

    QPixmap px(sheetPath);
    if (!px.isNull()) {
        m_spritesheetView->setPixmap(px);
        m_previewWidget->setSpritesheetPath(sheetPath);
    }
}

void AnimatorPanel::updateKeyframeProperties(int trackIdx, int keyIdx) {
    bool valid = false;

    if (trackIdx >= 0 && keyIdx >= 0 &&
        m_currentClip < (int)m_project.clips.size()) {
        auto& clip = m_project.clips[m_currentClip];
        if (trackIdx < (int)clip.tracks.size()) {
            auto& track = clip.tracks[trackIdx];
            if (keyIdx < (int)track.keys.size()) {
                auto& kf = track.keys[keyIdx];
                valid = true;

                m_kfTimeSpin->blockSignals(true);
                m_kfValueSpin->blockSignals(true);
                m_kfCurveCombo->blockSignals(true);

                m_kfTimeSpin->setValue(kf.time);
                m_kfValueSpin->setValue(kf.value);
                m_kfCurveCombo->setCurrentIndex(static_cast<int>(kf.curve));

                m_kfTimeSpin->blockSignals(false);
                m_kfValueSpin->blockSignals(false);
                m_kfCurveCombo->blockSignals(false);

                m_kfInfoLabel->setText(
                    QString("Track: %1  Key: %2")
                        .arg(QString::fromStdString(track.name))
                        .arg(keyIdx));
            }
        }
    }

    m_kfTimeSpin->setEnabled(valid);
    m_kfValueSpin->setEnabled(valid);
    m_kfCurveCombo->setEnabled(valid);

    if (!valid)
        m_kfInfoLabel->setText("No keyframe selected");
}
