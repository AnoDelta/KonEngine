#pragma once
#include <QWidget>
#include <QPixmap>
#include <QPoint>
#include <QRect>
#include <vector>
#include "AnimData.hpp"

// The sheet is rendered with an internal pan offset — no QScrollArea needed.
// Right-drag to pan, scroll wheel to zoom.
//
// Frame interaction:
//   Left-drag center      → move frame
//   Left-drag edge/corner → resize frame
//   Left-drag empty space → create new frame
//   Right-drag            → pan the sheet view
//   Escape                → deselect

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
    void frameDeselected();
    void frameResized(int idx, AnimFrame fr);

protected:
    void paintEvent(QPaintEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;

private:
    QPixmap  m_pixmap;
    const std::vector<AnimFrame>* m_frames = nullptr;
    int   m_selectedIdx  = -1;
    int   m_highlightIdx = -1;
    float m_zoom = 2.0f;

    // Internal view offset (pixels in widget space)
    float m_panX = 8.0f;
    float m_panY = 8.0f;

    enum class DragMode {
        None, Create, Move,
        ResizeL, ResizeR, ResizeT, ResizeB,
        ResizeTL, ResizeTR, ResizeBL, ResizeBR
    };

    DragMode  m_drag  = DragMode::None;
    QPoint    m_dStart;
    QPoint    m_dCur;
    int       m_dIdx  = -1;
    AnimFrame m_orig;
    AnimFrame m_live;

    // Right-drag pan
    bool  m_panning  = false;
    QPoint m_panPress;   // cursor pos at pan start (widget coords)
    float  m_panXAtPress = 0;
    float  m_panYAtPress = 0;

    // Coordinate helpers (account for pan offset)
    QPointF widgetToSheet(QPoint p) const;
    QPoint  sheetToWidget(float sx, float sy) const;
    QRect   frameToWidget(const AnimFrame& f) const;

    DragMode        hitTest(QPoint pos, int& outIdx) const;
    Qt::CursorShape cursorFor(DragMode m) const;

    void  collectEdges(std::vector<float>& xs, std::vector<float>& ys, int skip = -1) const;
    static float snapTo(float v, const std::vector<float>& c, float t);
    AnimFrame doSnap(AnimFrame fr, bool snap, int skip = -1) const;
};
