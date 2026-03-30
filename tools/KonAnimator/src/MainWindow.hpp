#pragma once
#include <QMainWindow>
#include <QDockWidget>
#include <QMenu>
#include <QActionGroup>
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
#include <vector>
#include <utility>

#include "AnimData.hpp"
#include "SpritesheetView.hpp"
#include "TimelineWidget.hpp"
#include "PreviewWidget.hpp"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    void openFile(const QString& path);

protected:
    void keyPressEvent(QKeyEvent* e) override;

private slots:
    void onNew();
    void onOpen();
    void onSave();
    void onSaveAs();
    void onLoadSpritesheet();
    void onCompile();

    void onAddClip();
    void onRemoveClip();
    void onClipSelected(int row);
    void onClipNameChanged();
    void onClipSettingsChanged();

    void onFrameTableSelected();
    void onAddFrameManual();
    void onRemoveFrame();
    void onFrameAdded(AnimFrame fr);
    void onFrameClicked(int idx);
    void onFramePropChanged();
    void onFrameResized(int idx, AnimFrame fr);
    void onApplyFpsToClip();   // rescale this clip's frame durations to current FPS
    void onApplyFpsToAll();    // rescale ALL clips' frame durations to current FPS

    void onAddTrack();
    void onRemoveTrack();
    void onTrackSelected(int row);
    void onAddKeyframe();
    void onRemoveKeyframe();
    void onKeyframeSelected(int ti, int ki);
    void onKeyframeMoved(int ti, int ki, float t);
    void onKFPropChanged();

    void onPlayPause();
    void onStop();
    void onFrameChanged(int idx);
    void onElapsedChanged(float t);

    void applyLayoutPreset(int preset);
    void resetDockLayout();

private:
    void buildMenuBar();
    void buildUI();
    QWidget* buildSheetPanel();
    QWidget* buildClipPanel();
    QWidget* buildPreviewPanel();
    QWidget* buildTimelinePanel();

    void refreshClipList();
    void refreshFrameTable();
    void refreshTrackList();
    void refreshClipSettings();
    void refreshFrameProps();
    void refreshKFProps();
    void updateTitle();
    bool confirmDiscard();
    QString projectName() const;

    // Simple snapshot undo
    void pushUndo();
    void undo();

    // ── Data ────────────────────────────────────────────────────────────
    AnimProject m_proj;
    int m_clipIdx  = -1;
    int m_frameIdx = -1;
    int m_trackIdx = -1;
    int m_keyIdx   = -1;
    AnimClip* clip() {
        return (m_clipIdx>=0 && m_clipIdx<(int)m_proj.clips.size())
               ? &m_proj.clips[m_clipIdx] : nullptr;
    }
    bool m_suppress = false;

    // Undo stack: (clip index, clip snapshot before edit)
    std::vector<std::pair<int, AnimClip>> m_undoStack;

    // ── Docks ───────────────────────────────────────────────────────────
    QDockWidget* m_clipDock     = nullptr;
    QDockWidget* m_previewDock  = nullptr;
    QDockWidget* m_timelineDock = nullptr;
    QByteArray   m_defaultLayout;
    QMenu*       m_viewMenu     = nullptr;

    // ── Left dock ───────────────────────────────────────────────────────
    QListWidget*    m_clipList   = nullptr;
    QLineEdit*      m_clipName   = nullptr;
    QCheckBox*      m_clipLoop   = nullptr;
    QDoubleSpinBox* m_dispW      = nullptr;
    QDoubleSpinBox* m_dispH      = nullptr;
    QDoubleSpinBox* m_dispScale  = nullptr;
    QDoubleSpinBox* m_fps        = nullptr;  // frames per second for new frames
    QTableWidget*   m_frameTable = nullptr;
    QDoubleSpinBox* m_fSrcX      = nullptr;
    QDoubleSpinBox* m_fSrcY      = nullptr;
    QDoubleSpinBox* m_fSrcW      = nullptr;
    QDoubleSpinBox* m_fSrcH      = nullptr;
    QDoubleSpinBox* m_fDur       = nullptr;

    // ── Center ──────────────────────────────────────────────────────────
    SpritesheetView* m_sheetView  = nullptr;
    QSlider*         m_zoomSlider = nullptr;

    // ── Right dock ──────────────────────────────────────────────────────
    PreviewWidget*  m_preview   = nullptr;
    QListWidget*    m_trackList = nullptr;
    QDoubleSpinBox* m_kfTime    = nullptr;
    QDoubleSpinBox* m_kfValue   = nullptr;
    QComboBox*      m_kfCurve   = nullptr;
    QDoubleSpinBox* m_vpW       = nullptr;
    QDoubleSpinBox* m_vpH       = nullptr;

    // ── Bottom dock ─────────────────────────────────────────────────────
    TimelineWidget* m_timeline  = nullptr;
    QPushButton*    m_playBtn   = nullptr;
    QLabel*         m_timeLabel = nullptr;
};
