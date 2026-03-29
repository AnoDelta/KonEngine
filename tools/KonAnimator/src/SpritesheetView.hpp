#pragma once
#include <QWidget>
#include <QPixmap>
#include <QPoint>
#include <QRect>
#include <QCursor>
#include "AnimData.hpp"

// Displays a spritesheet and lets the user:
//   LMB on empty area -- drag to define a new frame rect
//   LMB on frame      -- select it (stays selected)
//   LMB on selected frame body     -- drag to move it
//   LMB on selected frame edge/corner -- drag to resize it
//   RMB drag          -- pan the view
//   Scroll wheel      -- zoom

class SpritesheetView : public QWidget {
    Q_OBJECT
public:
    explicit SpritesheetView(QWidget* parent = nullptr);

    void setPixmap(const QPixmap& px);
    void setFrames(std::vector<AnimFrame>* frames); // non-const so we can edit
    void setSelectedFrame(int idx);
    void setHighlightFrame(int idx);
    void setZoom(float z);
    float zoom() const { return m_zoom; }

signals:
    void frameAdded(AnimFrame fr);
    void frameClicked(int idx);
    void frameModified(int idx); // emitted when a frame is moved or resized

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    QSize sizeHint() const override;

private:
    enum class HitZone {
        None,
        Body,
        Left, Right, Top, Bottom,
        TopLeft, TopRight, BottomLeft, BottomRight
    };

    QPixmap  m_pixmap;
    std::vector<AnimFrame>* m_frames = nullptr;
    int      m_selectedIdx  = -1;
    int      m_highlightIdx = -1;
    float    m_zoom = 2.0f;

    // New frame drag (LMB on empty)
    bool   m_dragging = false;
    QPoint m_dragStart, m_dragCurrent;

    // Move/resize drag (LMB on existing frame)
    bool     m_editing    = false;
    HitZone  m_editZone   = HitZone::None;
    QPoint   m_editStart;          // mouse pos at drag start (widget coords)
    AnimFrame m_editOriginal;      // frame state at drag start

    // RMB pan
    bool   m_panning        = false;
    QPoint m_panStart;
    QPoint m_panOffset;
    QPoint m_panOffsetStart;

    const int kHandle = 6; // px hit region for edges/corners

    QPointF widgetToSheet(QPoint p) const;
    QRect   frameToWidget(const AnimFrame& f) const;
    HitZone hitTest(const AnimFrame& f, QPoint widgetPos) const;
    QCursor cursorForZone(HitZone z) const;
};
