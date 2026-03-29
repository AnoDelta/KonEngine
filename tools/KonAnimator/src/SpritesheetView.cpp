#include "SpritesheetView.hpp"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QScrollArea>
#include <algorithm>

SpritesheetView::SpritesheetView(QWidget* parent) : QWidget(parent) {
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setCursor(Qt::CrossCursor);
}

void SpritesheetView::setPixmap(const QPixmap& px) {
    m_pixmap = px;
    updateGeometry();
    update();
}
void SpritesheetView::setFrames(const std::vector<AnimFrame>* f) { m_frames = f; update(); }
void SpritesheetView::setSelectedFrame(int i)  { m_selectedIdx  = i; update(); }
void SpritesheetView::setHighlightFrame(int i) { m_highlightIdx = i; update(); }
void SpritesheetView::setZoom(float z) {
    m_zoom = std::max(0.5f, std::min(z, 16.0f));
    updateGeometry();
    update();
}

QSize SpritesheetView::sizeHint() const {
    if (m_pixmap.isNull()) return QSize(400, 300);
    return QSize((int)(m_pixmap.width()  * m_zoom) + 8 + m_panOffset.x(),
                 (int)(m_pixmap.height() * m_zoom) + 8 + m_panOffset.y());
}

QPointF SpritesheetView::widgetToSheet(QPoint p) const {
    return QPointF((p.x() - 4 - m_panOffset.x()) / m_zoom,
                   (p.y() - 4 - m_panOffset.y()) / m_zoom);
}

QRect SpritesheetView::frameToWidget(const AnimFrame& f) const {
    int x = (int)(f.srcX * m_zoom) + 4 + m_panOffset.x();
    int y = (int)(f.srcY * m_zoom) + 4 + m_panOffset.y();
    int w = (int)(f.srcW * m_zoom);
    int h = (int)(f.srcH * m_zoom);
    return QRect(x, y, w, h);
}

void SpritesheetView::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::SmoothPixmapTransform, false);

    // Checkerboard background
    const int cs = 8;
    for (int y = 0; y < height(); y += cs)
        for (int x = 0; x < width(); x += cs)
            p.fillRect(x, y, cs, cs,
                ((x/cs + y/cs) % 2 == 0) ? QColor(55,55,55) : QColor(38,38,38));

    if (m_pixmap.isNull()) {
        p.setPen(QColor(130,130,130));
        p.drawText(rect(), Qt::AlignCenter,
            "No spritesheet loaded\n\nFile  \342\206\222  Load Spritesheet");
        return;
    }

    // Sheet image
    int ox = 4 + m_panOffset.x();
    int oy = 4 + m_panOffset.y();
    QRect dst(ox, oy,
              (int)(m_pixmap.width()  * m_zoom),
              (int)(m_pixmap.height() * m_zoom));
    p.drawPixmap(dst, m_pixmap);

    // Frame overlays
    if (m_frames) {
        for (int i = 0; i < (int)m_frames->size(); i++) {
            QRect r = frameToWidget((*m_frames)[i]);
            bool sel  = (i == m_selectedIdx);
            bool play = (i == m_highlightIdx);

            QColor fill   = play ? QColor(255,220,0, 70)
                          : sel  ? QColor(0, 180,255, 60)
                                 : QColor(255,255,255, 25);
            QColor border = play ? QColor(255,220,0)
                          : sel  ? QColor(0, 180,255)
                                 : QColor(200,200,200,120);
            int bw = (sel || play) ? 2 : 1;

            p.fillRect(r, fill);
            p.setPen(QPen(border, bw));
            p.drawRect(r);

            // Index label
            p.setPen(border);
            p.setFont(QFont("monospace", std::max(7, (int)(m_zoom * 3))));
            p.drawText(r.adjusted(2,1,0,0), QString::number(i));
        }
    }

    // Drag preview (new frame being drawn)
    if (m_dragging) {
        QRect drag = QRect(m_dragStart, m_dragCurrent).normalized();
        p.fillRect(drag, QColor(255,120,0,50));
        p.setPen(QPen(QColor(255,160,0), 1, Qt::DashLine));
        p.drawRect(drag);

        QPointF tl = widgetToSheet(drag.topLeft());
        QPointF br = widgetToSheet(drag.bottomRight());
        QString info = QString("%1\303\2272")
            .arg((int)(br.x()-tl.x()))
            .arg((int)(br.y()-tl.y()));
        p.setPen(QColor(255,200,100));
        p.setFont(QFont("monospace", 9));
        p.drawText(drag.bottomLeft() + QPoint(2,14), info);
    }

    // Pan cursor hint
    if (m_panning) {
        p.setPen(QColor(255,255,255,80));
        p.setFont(QFont("monospace", 8));
        p.drawText(8, height()-8, "Panning...");
    }
}

void SpritesheetView::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::RightButton) {
        // Start pan
        m_panning       = true;
        m_panStart      = e->pos();
        m_panOffsetStart = m_panOffset;
        setCursor(Qt::ClosedHandCursor);
        update();
        return;
    }

    if (e->button() != Qt::LeftButton) return;

    // Hit-test existing frames -- selection persists until another is clicked
    if (m_frames) {
        for (int i = (int)m_frames->size() - 1; i >= 0; i--) {
            if (frameToWidget((*m_frames)[i]).contains(e->pos())) {
                m_selectedIdx = i;
                emit frameClicked(i);
                update();
                return;  // don't start a drag if we clicked a frame
            }
        }
    }

    // Start drag for new frame (only if we didn't click an existing frame)
    m_dragging    = true;
    m_dragStart   = e->pos();
    m_dragCurrent = e->pos();
}

void SpritesheetView::mouseMoveEvent(QMouseEvent* e) {
    if (m_panning) {
        QPoint delta  = e->pos() - m_panStart;
        m_panOffset   = m_panOffsetStart + delta;
        updateGeometry();
        update();
        return;
    }
    if (m_dragging) {
        m_dragCurrent = e->pos();
        update();
    }
}

void SpritesheetView::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::RightButton && m_panning) {
        m_panning = false;
        setCursor(Qt::CrossCursor);
        update();
        return;
    }

    if (e->button() != Qt::LeftButton || !m_dragging) return;
    m_dragging = false;

    QRect drag = QRect(m_dragStart, m_dragCurrent).normalized();
    if (drag.width() >= 2 && drag.height() >= 2) {
        QPointF tl = widgetToSheet(drag.topLeft());
        QPointF br = widgetToSheet(drag.bottomRight());
        AnimFrame fr;
        fr.srcX     = std::max(0.0f, (float)tl.x());
        fr.srcY     = std::max(0.0f, (float)tl.y());
        fr.srcW     = std::max(1.0f, (float)(br.x() - tl.x()));
        fr.srcH     = std::max(1.0f, (float)(br.y() - tl.y()));
        fr.duration = 0.1f;
        emit frameAdded(fr);
    }
    update();
}

void SpritesheetView::wheelEvent(QWheelEvent* e) {
    setZoom(m_zoom * (e->angleDelta().y() > 0 ? 1.25f : 0.8f));
}
