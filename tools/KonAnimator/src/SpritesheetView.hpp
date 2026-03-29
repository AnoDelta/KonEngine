#pragma once
#include <QWidget>
#include <QPixmap>
#include <QPoint>
#include <QRect>
#include "AnimData.hpp"

// Displays a spritesheet and lets the user:
//   - Left click+drag  -- define new frame rects
//   - Left click frame -- select it (stays selected until another is clicked)
//   - Right click+drag -- pan the view
//   - Mousewheel       -- zoom
//   - Highlights the currently playing frame during preview

class SpritesheetView : public QWidget {
    Q_OBJECT
public:
    explicit SpritesheetView(QWidget* parent = nullptr);

    void setPixmap(const QPixmap& px);
    void setFrames(const std::vector<AnimFrame>* frames);
    void setSelectedFrame(int idx);
    void setHighlightFrame(int idx);
    void setZoom(float z);
    float zoom() const { return m_zoom; }

signals:
    void frameAdded(AnimFrame fr);
    void frameClicked(int idx);

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    QSize sizeHint() const override;

private:
    QPixmap  m_pixmap;
    const std::vector<AnimFrame>* m_frames = nullptr;
    int      m_selectedIdx  = -1;
    int      m_highlightIdx = -1;
    float    m_zoom = 2.0f;

    // Left button -- draw new frame
    bool   m_dragging = false;
    QPoint m_dragStart, m_dragCurrent;

    // Right button -- pan view
    bool   m_panning = false;
    QPoint m_panStart;
    QPoint m_panOffset;
    QPoint m_panOffsetStart;

    QPointF widgetToSheet(QPoint p) const;
    QRect   frameToWidget(const AnimFrame& f) const;
};
