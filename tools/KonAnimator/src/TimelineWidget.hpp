#pragma once
#include <QWidget>
#include "AnimData.hpp"

// Horizontal keyframe timeline.
// - Click ruler        -- move playhead
// - Click diamond      -- select keyframe
// - Drag diamond       -- move keyframe in time
// - Scroll wheel       -- zoom in/out
// - Timeline is endless -- extends automatically beyond clip duration

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
    void clipEdited();

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    QSize minimumSizeHint() const override;

private:
    AnimClip* m_clip     = nullptr;
    float     m_playhead = 0.0f;
    float     m_zoom     = 100.0f; // px per second
    float     m_scrollX  = 0.0f;  // horizontal scroll offset in seconds

    int m_selTrack = -1, m_selKey = -1;
    bool m_dragging = false;

    static const int kHeaderH = 22;
    static const int kTrackH  = 26;
    static const int kLabelW  = 80;

    // Endless: always show at least 30s, or clip duration + 10s, whichever is larger
    float visibleStart()    const { return m_scrollX; }
    float visibleEnd()      const { return m_scrollX + (width() - kLabelW) / m_zoom; }
    float totalDuration()   const;

    int   timeToX(float t) const { return kLabelW + (int)((t - m_scrollX) * m_zoom); }
    float xToTime(int x)   const { return m_scrollX + (x - kLabelW) / m_zoom; }
    int   trackY(int i)    const { return kHeaderH + i * kTrackH; }
};
