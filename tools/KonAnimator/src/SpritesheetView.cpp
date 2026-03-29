#include "SpritesheetView.hpp"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <algorithm>

SpritesheetView::SpritesheetView(QWidget* parent) : QWidget(parent) {
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setCursor(Qt::CrossCursor);
}

void SpritesheetView::setPixmap(const QPixmap& px) {
    m_pixmap = px; updateGeometry(); update();
}
void SpritesheetView::setFrames(std::vector<AnimFrame>* f) {
    m_frames = f; update();
}
void SpritesheetView::setSelectedFrame(int i)  { m_selectedIdx  = i; update(); }
void SpritesheetView::setHighlightFrame(int i) { m_highlightIdx = i; update(); }
void SpritesheetView::setZoom(float z) {
    m_zoom = std::max(0.5f, std::min(z, 16.0f));
    updateGeometry(); update();
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

SpritesheetView::HitZone SpritesheetView::hitTest(const AnimFrame& f, QPoint p) const {
    QRect r = frameToWidget(f);
    if (!r.adjusted(-kHandle, -kHandle, kHandle, kHandle).contains(p))
        return HitZone::None;

    bool onL = p.x() <= r.left()  + kHandle;
    bool onR = p.x() >= r.right() - kHandle;
    bool onT = p.y() <= r.top()   + kHandle;
    bool onB = p.y() >= r.bottom()- kHandle;

    if (onT && onL) return HitZone::TopLeft;
    if (onT && onR) return HitZone::TopRight;
    if (onB && onL) return HitZone::BottomLeft;
    if (onB && onR) return HitZone::BottomRight;
    if (onL)        return HitZone::Left;
    if (onR)        return HitZone::Right;
    if (onT)        return HitZone::Top;
    if (onB)        return HitZone::Bottom;

    if (r.contains(p)) return HitZone::Body;
    return HitZone::None;
}

QCursor SpritesheetView::cursorForZone(HitZone z) const {
    switch (z) {
        case HitZone::Body:        return Qt::SizeAllCursor;
        case HitZone::Left:
        case HitZone::Right:       return Qt::SizeHorCursor;
        case HitZone::Top:
        case HitZone::Bottom:      return Qt::SizeVerCursor;
        case HitZone::TopLeft:
        case HitZone::BottomRight: return Qt::SizeFDiagCursor;
        case HitZone::TopRight:
        case HitZone::BottomLeft:  return Qt::SizeBDiagCursor;
        default:                   return Qt::CrossCursor;
    }
}

void SpritesheetView::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::SmoothPixmapTransform, false);

    // Checkerboard
    const int cs = 8;
    for (int y = 0; y < height(); y += cs)
        for (int x = 0; x < width(); x += cs)
            p.fillRect(x, y, cs, cs,
                ((x/cs + y/cs) % 2 == 0) ? QColor(55,55,55) : QColor(38,38,38));

    if (m_pixmap.isNull()) {
        p.setPen(QColor(130,130,130));
        p.drawText(rect(), Qt::AlignCenter,
            "No spritesheet loaded\n\nFile \342\206\222 Load Spritesheet");
        return;
    }

    int ox = 4 + m_panOffset.x();
    int oy = 4 + m_panOffset.y();
    QRect dst(ox, oy,
              (int)(m_pixmap.width()  * m_zoom),
              (int)(m_pixmap.height() * m_zoom));
    p.drawPixmap(dst, m_pixmap);

    if (m_frames) {
        for (int i = 0; i < (int)m_frames->size(); i++) {
            QRect r  = frameToWidget((*m_frames)[i]);
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

            p.setPen(border);
            p.setFont(QFont("monospace", std::max(7, (int)(m_zoom * 3))));
            p.drawText(r.adjusted(2,1,0,0), QString::number(i));

            // Draw resize handles on selected frame
            if (sel && !play) {
                p.setBrush(QColor(0,180,255));
                p.setPen(QPen(Qt::white, 1));
                auto drawHandle = [&](int hx, int hy) {
                    p.drawRect(hx - kHandle/2, hy - kHandle/2, kHandle, kHandle);
                };
                drawHandle(r.left(),        r.top());
                drawHandle(r.center().x(),  r.top());
                drawHandle(r.right(),       r.top());
                drawHandle(r.left(),        r.center().y());
                drawHandle(r.right(),       r.center().y());
                drawHandle(r.left(),        r.bottom());
                drawHandle(r.center().x(),  r.bottom());
                drawHandle(r.right(),       r.bottom());
            }
        }
    }

    // New frame drag preview
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
}

void SpritesheetView::mousePressEvent(QMouseEvent* e) {
    // RMB: pan
    if (e->button() == Qt::RightButton) {
        m_panning        = true;
        m_panStart       = e->pos();
        m_panOffsetStart = m_panOffset;
        setCursor(Qt::ClosedHandCursor);
        return;
    }

    if (e->button() != Qt::LeftButton) return;

    // Hit-test frames (selected frame first, then others)
    if (m_frames) {
        // Check selected frame first for better UX
        auto tryFrame = [&](int i) -> bool {
            HitZone z = hitTest((*m_frames)[i], e->pos());
            if (z == HitZone::None) return false;
            m_selectedIdx  = i;
            m_editing      = true;
            m_editZone     = z;
            m_editStart    = e->pos();
            m_editOriginal = (*m_frames)[i];
            emit frameClicked(i);
            update();
            return true;
        };

        if (m_selectedIdx >= 0 && m_selectedIdx < (int)m_frames->size())
            if (tryFrame(m_selectedIdx)) return;

        for (int i = (int)m_frames->size()-1; i >= 0; i--) {
            if (i == m_selectedIdx) continue;
            if (tryFrame(i)) return;
        }
    }

    // No frame hit — start drawing a new frame
    m_dragging    = true;
    m_dragStart   = e->pos();
    m_dragCurrent = e->pos();
}

void SpritesheetView::mouseMoveEvent(QMouseEvent* e) {
    if (m_panning) {
        m_panOffset = m_panOffsetStart + (e->pos() - m_panStart);
        updateGeometry(); update();
        return;
    }

    if (m_editing && m_frames && m_selectedIdx >= 0) {
        QPointF delta = QPointF(e->pos() - m_editStart) / m_zoom;
        float dx = (float)delta.x();
        float dy = (float)delta.y();
        AnimFrame& fr = (*m_frames)[m_selectedIdx];
        AnimFrame  o  = m_editOriginal;

        switch (m_editZone) {
            case HitZone::Body:
                fr.srcX = o.srcX + dx;
                fr.srcY = o.srcY + dy;
                break;
            case HitZone::Left:
                fr.srcX = o.srcX + dx;
                fr.srcW = std::max(1.0f, o.srcW - dx);
                break;
            case HitZone::Right:
                fr.srcW = std::max(1.0f, o.srcW + dx);
                break;
            case HitZone::Top:
                fr.srcY = o.srcY + dy;
                fr.srcH = std::max(1.0f, o.srcH - dy);
                break;
            case HitZone::Bottom:
                fr.srcH = std::max(1.0f, o.srcH + dy);
                break;
            case HitZone::TopLeft:
                fr.srcX = o.srcX + dx; fr.srcW = std::max(1.0f, o.srcW - dx);
                fr.srcY = o.srcY + dy; fr.srcH = std::max(1.0f, o.srcH - dy);
                break;
            case HitZone::TopRight:
                fr.srcW = std::max(1.0f, o.srcW + dx);
                fr.srcY = o.srcY + dy; fr.srcH = std::max(1.0f, o.srcH - dy);
                break;
            case HitZone::BottomLeft:
                fr.srcX = o.srcX + dx; fr.srcW = std::max(1.0f, o.srcW - dx);
                fr.srcH = std::max(1.0f, o.srcH + dy);
                break;
            case HitZone::BottomRight:
                fr.srcW = std::max(1.0f, o.srcW + dx);
                fr.srcH = std::max(1.0f, o.srcH + dy);
                break;
            default: break;
        }

        // Clamp position to sheet bounds
        if (!m_pixmap.isNull()) {
            fr.srcX = std::max(0.0f, fr.srcX);
            fr.srcY = std::max(0.0f, fr.srcY);
        }

        emit frameModified(m_selectedIdx);
        update();
        return;
    }

    if (m_dragging) {
        m_dragCurrent = e->pos();
        update();
        return;
    }

    // Update cursor based on what's under the mouse
    if (m_frames && m_selectedIdx >= 0 && m_selectedIdx < (int)m_frames->size()) {
        HitZone z = hitTest((*m_frames)[m_selectedIdx], e->pos());
        if (z != HitZone::None) { setCursor(cursorForZone(z)); return; }
    }
    // Check other frames for hover cursor
    if (m_frames) {
        for (int i = (int)m_frames->size()-1; i >= 0; i--) {
            HitZone z = hitTest((*m_frames)[i], e->pos());
            if (z != HitZone::None) {
                setCursor(z == HitZone::Body ? Qt::SizeAllCursor : cursorForZone(z));
                return;
            }
        }
    }
    setCursor(Qt::CrossCursor);
}

void SpritesheetView::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::RightButton && m_panning) {
        m_panning = false;
        setCursor(Qt::CrossCursor);
        return;
    }

    if (m_editing) {
        m_editing = false;
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
