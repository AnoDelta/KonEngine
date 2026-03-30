#pragma once
#include <QWidget>
#include <QScrollBar>
#include "AnimData.hpp"

// Endless horizontal keyframe timeline.
//   - Ruler extends well beyond the clip end — you can always scroll further right
//   - Orange end-marker line shows the clip's total duration
//   - Click/drag ruler  → move playhead
//   - Click diamond     → select keyframe
//   - Drag diamond      → move keyframe in time
//   - Ctrl+scroll       → zoom the time axis
//   - Scroll / drag scrollbar → pan

class TimelineWidget : public QWidget {
    Q_OBJECT
public:
    explicit TimelineWidget(QWidget* parent = nullptr);

    void  setClip(AnimClip* clip);   // switch clip, resets playhead & scroll
    void  refreshClip();             // same clip, keeps playhead & scroll
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
    void resizeEvent(QResizeEvent*) override;

private:
    AnimClip* m_clip     = nullptr;
    float     m_playhead = 0.0f;
    float     m_zoom     = 80.0f;   // px per second
    float     m_scroll   = 0.0f;   // px offset (how far left we've scrolled)

    int  m_selTrack = -1, m_selKey = -1;
    bool  m_dragging    = false;
    bool  m_panningRuler = false;
    int   m_panStart    = 0;
    float m_scrollAtPan = 0;

    // Right-click pan (same idea as middle-click, but more discoverable)
    bool  m_rightPanning   = false;
    int   m_rightPanStart  = 0;
    float m_rightScrollOrg = 0.0f;

    static constexpr int kHeaderH = 24;
    static constexpr int kTrackH  = 26;
    static constexpr int kLabelW  = 80;
    // Extra seconds shown beyond the clip end so you can always add keyframes later
    static constexpr float kPadding = 5.0f;

    // Convert between time and widget-x (accounts for scroll and label column)
    int   timeToX(float t) const { return kLabelW + (int)(t * m_zoom) - (int)m_scroll; }
    float xToTime(int x)   const { return ((x - kLabelW) + m_scroll) / m_zoom; }
    int   trackY(int i)    const { return kHeaderH + i * kTrackH; }

    float visibleDuration() const;  // total width in seconds we render
    void  updateHeight();
    void  clampScroll();
};
