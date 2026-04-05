#pragma once
#include <QWidget>
#include <QSplitter>
#include <QPushButton>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QScrollArea>

#include "AnimData.hpp"
#include "AnimIO.hpp"
#include "PreviewWidget.hpp"
#include "SpritesheetView.hpp"
#include "TimelineWidget.hpp"

class AnimatorPanel : public QWidget {
    Q_OBJECT
public:
    explicit AnimatorPanel(QWidget* parent = nullptr);
    ~AnimatorPanel();

    void openFile(const QString& path);
    void newFile();
    void saveFile();
    bool hasUnsavedChanges() const;

signals:
    void titleChanged(const QString& title);

private slots:
    void onNewAnimation();
    void onOpenAnimation();
    void onSaveAnimation();
    void onAddTrack();
    void onAddKeyframe();
    void onPlayStop();
    void onClipChanged(int index);
    void onFrameClicked(int idx);
    void onKeyframeSelected(int trackIdx, int keyIdx);
    void onKeyframeMoved(int trackIdx, int keyIdx, float newTime);
    void onPlayheadChanged(float t);

private:
    void setupUI();
    void setupToolbar(QVBoxLayout* mainLayout);
    void rebuildClipSelector();
    void refreshAll();
    void loadSpritesheet();
    void updateKeyframeProperties(int trackIdx, int keyIdx);

    // Data
    AnimProject m_project;
    int         m_currentClip = 0;
    int         m_selTrack    = -1;
    int         m_selKey      = -1;

    // Widgets
    QSplitter*       m_outerSplitter = nullptr;   // vertical: top | timeline
    QSplitter*       m_topSplitter   = nullptr;   // horizontal: spritesheet | preview | props

    SpritesheetView* m_spritesheetView = nullptr;
    PreviewWidget*   m_previewWidget   = nullptr;
    TimelineWidget*  m_timelineWidget  = nullptr;

    // Toolbar
    QPushButton*  m_playBtn   = nullptr;
    QComboBox*    m_clipCombo = nullptr;

    // Keyframe property editor (right sidebar)
    QWidget*        m_propsPanel   = nullptr;
    QDoubleSpinBox* m_kfTimeSpin   = nullptr;
    QDoubleSpinBox* m_kfValueSpin  = nullptr;
    QComboBox*      m_kfCurveCombo = nullptr;
    QLabel*         m_kfInfoLabel  = nullptr;
};
