#pragma once
#include <QMainWindow>
#include <QTimer>
#include <QLabel>
#include <QListWidget>
#include <QTableWidget>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QSlider>
#include <QScrollArea>
#include <QFileInfo>

#include "AnimData.hpp"
#include "SpritesheetView.hpp"
#include "TimelineWidget.hpp"
#include "PreviewWidget.hpp"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    void openFile(const QString& path);

private slots:
    // File
    void onNew();
    void onOpen();
    void onSave();
    void onSaveAs();
    void onLoadSpritesheet();
    void onCompile();

    // Clips
    void onAddClip();
    void onRemoveClip();
    void onClipSelected(int row);
    void onClipNameChanged();
    void onClipSettingsChanged();

    // Frames
    void onFrameTableSelected();
    void onAddFrameManual();
    void onRemoveFrame();
    void onFrameAdded(AnimFrame fr);
    void onFrameClicked(int idx);
    void onFramePropChanged();

    // Tracks / keyframes
    void onAddTrack();
    void onRemoveTrack();
    void onTrackSelected(int row);
    void onAddKeyframe();
    void onRemoveKeyframe();
    void onKeyframeSelected(int ti, int ki);
    void onKeyframeMoved(int ti, int ki, float t);
    void onKFPropChanged();

    // Playback
    void onPlayPause();
    void onStop();
    void onFrameChanged(int idx);
    void onElapsedChanged(float t);

private:
    void buildMenuBar();
    void buildUI();
    QWidget* buildLeftPanel();
    QWidget* buildCenterPanel();
    QWidget* buildRightPanel();
    QWidget* buildBottomPanel();

    void refreshClipList();
    void refreshFrameTable();
    void refreshTrackList();
    void refreshClipSettings();
    void refreshFrameProps();
    void refreshKFProps();
    void updateTitle();
    bool confirmDiscard();
    QString projectName() const;

    // ---- Data ----
    AnimProject m_proj;
    int m_clipIdx  = -1;
    int m_frameIdx = -1;
    int m_trackIdx = -1;
    int m_keyIdx   = -1;

    AnimClip* clip()  { return (m_clipIdx>=0 && m_clipIdx<(int)m_proj.clips.size()) ? &m_proj.clips[m_clipIdx] : nullptr; }

    bool m_suppress = false; // block signal-triggered saves during refresh

    // ---- Left panel ----
    QListWidget*    m_clipList    = nullptr;
    QLineEdit*      m_clipName    = nullptr;
    QCheckBox*      m_clipLoop    = nullptr;
    QDoubleSpinBox* m_dispW       = nullptr;
    QDoubleSpinBox* m_dispH       = nullptr;
    QDoubleSpinBox* m_dispScale   = nullptr;
    QTableWidget*   m_frameTable  = nullptr;
    QDoubleSpinBox* m_fSrcX       = nullptr;
    QDoubleSpinBox* m_fSrcY       = nullptr;
    QDoubleSpinBox* m_fSrcW       = nullptr;
    QDoubleSpinBox* m_fSrcH       = nullptr;
    QDoubleSpinBox* m_fDur        = nullptr;

    // ---- Center panel ----
    SpritesheetView* m_sheetView  = nullptr;
    QSlider*         m_zoomSlider = nullptr;

    // ---- Right panel ----
    PreviewWidget*  m_preview     = nullptr;
    QListWidget*    m_trackList   = nullptr;
    QDoubleSpinBox* m_kfTime      = nullptr;
    QDoubleSpinBox* m_kfValue     = nullptr;
    QComboBox*      m_kfCurve     = nullptr;

    // ---- Bottom panel ----
    TimelineWidget* m_timeline    = nullptr;
    QPushButton*    m_playBtn     = nullptr;
    QLabel*         m_timeLabel   = nullptr;
};
