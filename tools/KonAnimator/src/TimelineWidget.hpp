#pragma once
#include <QWidget>
#include "AnimData.hpp"

// Horizontal keyframe timeline.
// Shows all KFTracks for the current clip.
// - Click ruler to move playhead
// - Click diamond to select keyframe
// - Drag diamond to move it in time

class TimelineWidget : public QWidget {
    Q_OBJECT
public:
    explicit TimelineWidget(QWidget* parent = nullptr);

    void  setClip(AnimClip* clip);
    void  setPlayhead(float t);
    float playhead() const { return m_playhead; }

signals:
    void playheadChanged(float t);
    void keyframeSelected(int trackIdx, int keyIdx);
    void keyframeMoved(int trackIdx, int keyIdx, float newTime);
    void clipEdited();  // emitted after any drag

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;

private:
    AnimClip* m_clip     = nullptr;
    float     m_playhead = 0.0f;
    float     m_zoom     = 100.0f; // px per second

    int m_selTrack = -1, m_selKey = -1;
    bool m_dragging = false;

    static const int kHeaderH = 22;
    static const int kTrackH  = 26;
    static const int kLabelW  = 76;

    int   timeToX(float t) const { return kLabelW + (int)(t * m_zoom); }
    float xToTime(int x)   const { return (x - kLabelW) / m_zoom; }
    int   trackY(int i)    const { return kHeaderH + i * kTrackH; }

    float visibleDuration() const;
};
