#pragma once
#include <QWidget>
#include <QPixmap>
#include <QPoint>
#include <QRect>
#include "AnimData.hpp"

// Displays a spritesheet and lets the user:
//   - Click+drag to define new frame rects
//   - Click an existing frame to select it
//   - Mousewheel to zoom
//   - Highlights the currently playing frame during preview

class SpritesheetView : public QWidget {
    Q_OBJECT
public:
    explicit SpritesheetView(QWidget* parent = nullptr);

    void setPixmap(const QPixmap& px);
    void setFrames(const std::vector<AnimFrame>* frames);
    void setSelectedFrame(int idx);
    void setHighlightFrame(int idx);  // used during playback
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

    bool   m_dragging = false;
    QPoint m_dragStart, m_dragCurrent;

    QPointF widgetToSheet(QPoint p) const;
    QRect   frameToWidget(const AnimFrame& f) const;
};
