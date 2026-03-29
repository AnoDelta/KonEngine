#include "TimelineWidget.hpp"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <cmath>
#include <algorithm>

TimelineWidget::TimelineWidget(QWidget* parent) : QWidget(parent) {
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    setMinimumHeight(kHeaderH + kTrackH);
}

void TimelineWidget::setClip(AnimClip* clip) {
    m_clip     = clip;
    m_playhead = 0.0f;
    m_selTrack = m_selKey = -1;
    m_scrollX  = 0.0f;
    int tracks = clip ? (int)clip->tracks.size() : 0;
    setMinimumHeight(kHeaderH + std::max(1, tracks) * kTrackH + 4);
    update();
}

void TimelineWidget::setPlayhead(float t) {
    m_playhead = t;
    // Auto-scroll to keep playhead visible
    float vis = (width() - kLabelW) / m_zoom;
    if (t > m_scrollX + vis - 0.5f)
        m_scrollX = t - vis + 1.0f;
    if (t < m_scrollX + 0.1f && m_scrollX > 0.0f)
        m_scrollX = std::max(0.0f, t - 0.5f);
    update();
}

float TimelineWidget::totalDuration() const {
    float dur = m_clip ? m_clip->totalDuration() : 0.0f;
    return std::max(30.0f, dur + 10.0f); // always at least 30s visible
}

QSize TimelineWidget::minimumSizeHint() const {
    return QSize(200, kHeaderH + kTrackH);
}

void TimelineWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Background
    p.fillRect(rect(), QColor(32,32,32));

    // ---- Ruler ----
    p.fillRect(0, 0, width(), kHeaderH, QColor(24,24,24));

    // Label area background
    p.fillRect(0, 0, kLabelW, height(), QColor(22,22,22));

    // Choose tick step based on zoom
    float step;
    if      (m_zoom >= 200) step = 0.1f;
    else if (m_zoom >= 80)  step = 0.25f;
    else if (m_zoom >= 30)  step = 0.5f;
    else if (m_zoom >= 10)  step = 1.0f;
    else if (m_zoom >= 4)   step = 5.0f;
    else                    step = 10.0f;

    float start = std::floor(visibleStart() / step) * step;
    float end   = visibleEnd() + step;

    for (float t = start; t <= end; t += step) {
        int x = timeToX(t);
        if (x < kLabelW || x > width()) continue;

        bool major = std::fmod(std::abs(t) + 0.0001f, 1.0f) < step * 0.5f;
        p.setPen(major ? QColor(140,140,140) : QColor(60,60,60));
        p.drawLine(x, major ? 4 : 14, x, kHeaderH);

        if (major) {
            p.setPen(QColor(160,160,160));
            p.setFont(QFont("monospace", 8));
            // Format: show minutes if >= 60s
            QString label;
            if (t >= 60.0f) {
                int mins = (int)(t / 60);
                float secs = t - mins * 60;
                label = QString("%1:%2").arg(mins).arg(secs, 4, 'f', 1, '0');
            } else {
                label = QString::number(t, 'f', t < 10 ? 2 : 1) + "s";
            }
            p.drawText(x + 2, kHeaderH - 3, label);
        }
    }

    // Clip duration marker
    if (m_clip) {
        float clipDur = m_clip->totalDuration();
        int cx = timeToX(clipDur);
        if (cx >= kLabelW && cx <= width()) {
            p.setPen(QPen(QColor(100,200,100,120), 1, Qt::DashLine));
            p.drawLine(cx, 0, cx, height());
            p.setPen(QColor(100,200,100,150));
            p.setFont(QFont("monospace", 7));
            p.drawText(cx + 2, kHeaderH - 3, "end");
        }
    }

    // ---- Tracks ----
    if (m_clip) {
        for (int ti = 0; ti < (int)m_clip->tracks.size(); ti++) {
            auto& tr = m_clip->tracks[ti];
            int y = trackY(ti);

            // Row background
            p.fillRect(kLabelW, y, width()-kLabelW, kTrackH,
                       ti%2==0 ? QColor(42,42,42) : QColor(36,36,36));

            // Label
            p.fillRect(0, y, kLabelW, kTrackH, QColor(26,26,26));
            p.setPen(QColor(190,190,190));
            p.setFont(QFont("monospace", 9));
            p.drawText(QRect(4, y, kLabelW-8, kTrackH),
                       Qt::AlignVCenter | Qt::AlignLeft,
                       QString::fromStdString(tr.name));

            // Separator
            p.setPen(QColor(50,50,50));
            p.drawLine(0, y+kTrackH-1, width(), y+kTrackH-1);

            // Keyframe diamonds
            for (int ki = 0; ki < (int)tr.keys.size(); ki++) {
                auto& kf = tr.keys[ki];
                int   kx = timeToX(kf.time);
                if (kx < kLabelW - 8 || kx > width() + 8) continue;
                int   cy = y + kTrackH/2;
                bool  sel = (ti==m_selTrack && ki==m_selKey);

                QColor col    = sel ? QColor(255,210,0)   : QColor(0,190,255);
                QColor border = sel ? QColor(255,255,120)  : QColor(0,140,200);
                int r = sel ? 6 : 5;

                QPolygon diamond;
                diamond << QPoint(kx,   cy-r)
                        << QPoint(kx+r, cy)
                        << QPoint(kx,   cy+r)
                        << QPoint(kx-r, cy);
                p.setBrush(col);
                p.setPen(QPen(border, sel ? 2 : 1));
                p.drawPolygon(diamond);
            }
        }
    }

    // ---- Playhead ----
    int px = timeToX(m_playhead);
    if (px >= kLabelW && px <= width()) {
        p.setPen(QPen(QColor(255,70,70), 2));
        p.drawLine(px, 0, px, height());
        QPolygon tri;
        tri << QPoint(px-5,0) << QPoint(px+5,0) << QPoint(px,8);
        p.setBrush(QColor(255,70,70));
        p.setPen(Qt::NoPen);
        p.drawPolygon(tri);
    }

    // Label area border
    p.setPen(QColor(50,50,50));
    p.drawLine(kLabelW, 0, kLabelW, height());

    // Zoom hint
    p.setPen(QColor(80,80,80));
    p.setFont(QFont("monospace", 7));
    p.drawText(kLabelW + 4, height() - 3,
               QString("Scroll to pan  |  Ctrl+Scroll to zoom  |  %1 px/s").arg((int)m_zoom));
}

void TimelineWidget::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::RightButton) {
        m_panning    = true;
        m_panStartX  = e->pos().x();
        m_panScrollX = m_scrollX;
        return;
    }

    if (!m_clip) return;

    // Hit-test keyframes first
    for (int ti = 0; ti < (int)m_clip->tracks.size(); ti++) {
        auto& tr = m_clip->tracks[ti];
        int y = trackY(ti);
        for (int ki = 0; ki < (int)tr.keys.size(); ki++) {
            int kx = timeToX(tr.keys[ki].time);
            int cy = y + kTrackH/2;
            if (std::abs(e->pos().x()-kx) <= 8 && std::abs(e->pos().y()-cy) <= 8) {
                m_selTrack = ti; m_selKey = ki;
                m_dragging = true;
                emit keyframeSelected(ti, ki);
                update();
                return;
            }
        }
    }

    // Click ruler → move playhead
    float t = std::max(0.0f, xToTime(e->pos().x()));
    m_playhead = t;
    emit playheadChanged(t);
    update();
}

void TimelineWidget::mouseMoveEvent(QMouseEvent* e) {
    if (m_panning) {
        float dx  = (e->pos().x() - m_panStartX) / m_zoom;
        m_scrollX = std::max(0.0f, m_panScrollX - dx);
        update();
        return;
    }
    if (m_dragging && m_selTrack >= 0 && m_selKey >= 0 && m_clip) {
        float t = std::max(0.0f, xToTime(e->pos().x()));
        m_clip->tracks[m_selTrack].keys[m_selKey].time = t;
        emit keyframeMoved(m_selTrack, m_selKey, t);
        update();
    } else if (e->buttons() & Qt::LeftButton) {
        float t = std::max(0.0f, xToTime(e->pos().x()));
        m_playhead = t;
        emit playheadChanged(t);
        update();
    }
}

void TimelineWidget::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::RightButton && m_panning) {
        m_panning = false;
        return;
    }
    if (m_dragging) {
        m_dragging = false;
        if (m_clip && m_selTrack >= 0)
            m_clip->tracks[m_selTrack].sortKeys();
        emit clipEdited();
    }
}

void TimelineWidget::wheelEvent(QWheelEvent* e) {
    if (e->modifiers() & Qt::ControlModifier) {
        // Zoom in/out around mouse position
        float tAtMouse = xToTime(e->position().x());
        float factor   = e->angleDelta().y() > 0 ? 1.25f : 0.8f;
        m_zoom = std::max(4.0f, std::min(m_zoom * factor, 800.0f));
        // Adjust scroll so the time under cursor stays fixed
        m_scrollX = tAtMouse - (e->position().x() - kLabelW) / m_zoom;
        m_scrollX = std::max(0.0f, m_scrollX);
    } else {
        // Horizontal scroll
        float delta = (e->angleDelta().y() > 0 ? -1.0f : 1.0f) * (3.0f / m_zoom);
        m_scrollX   = std::max(0.0f, m_scrollX + delta);
    }
    update();
}
