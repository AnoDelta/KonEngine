#include "TimelineWidget.hpp"
#include <QPainter>
#include <QMouseEvent>
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
    int tracks = clip ? (int)clip->tracks.size() : 0;
    setMinimumHeight(kHeaderH + std::max(1, tracks) * kTrackH + 4);
    update();
}

void TimelineWidget::setPlayhead(float t) { m_playhead = t; update(); }

float TimelineWidget::visibleDuration() const {
    float dur = m_clip ? m_clip->totalDuration() : 1.0f;
    return std::max(1.0f, dur + 0.5f);
}

void TimelineWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Background
    p.fillRect(rect(), QColor(32,32,32));

    // ---- Ruler ----
    p.fillRect(0, 0, width(), kHeaderH, QColor(24,24,24));

    float dur  = visibleDuration();
    float step = (m_zoom >= 80) ? 0.1f : (m_zoom >= 30) ? 0.5f : 1.0f;

    for (float t = 0.0f; t <= dur; t += step) {
        int x = timeToX(t);
        if (x < kLabelW || x > width()) continue;
        bool major = std::fmod(t + 0.0001f, 1.0f) < 0.01f;
        p.setPen(major ? QColor(160,160,160) : QColor(70,70,70));
        p.drawLine(x, major ? 2 : 12, x, kHeaderH);
        if (major) {
            p.setPen(QColor(170,170,170));
            p.setFont(QFont("monospace", 8));
            p.drawText(x+2, kHeaderH-3, QString::number(t,'f',1)+"s");
        }
    }

    // ---- Tracks ----
    if (m_clip) {
        for (int ti = 0; ti < (int)m_clip->tracks.size(); ti++) {
            auto& tr = m_clip->tracks[ti];
            int y = trackY(ti);

            // Row bg
            p.fillRect(0, y, width(), kTrackH,
                       ti%2==0 ? QColor(42,42,42) : QColor(36,36,36));

            // Label area
            p.fillRect(0, y, kLabelW, kTrackH, QColor(26,26,26));
            p.setPen(QColor(190,190,190));
            p.setFont(QFont("monospace", 9));
            QRect labelRect(4, y, kLabelW-8, kTrackH);
            p.drawText(labelRect, Qt::AlignVCenter | Qt::AlignLeft,
                       QString::fromStdString(tr.name));

            // Separator
            p.setPen(QColor(50,50,50));
            p.drawLine(0, y+kTrackH-1, width(), y+kTrackH-1);

            // Keyframe diamonds
            for (int ki = 0; ki < (int)tr.keys.size(); ki++) {
                auto& kf = tr.keys[ki];
                int   x  = timeToX(kf.time);
                int   cy = y + kTrackH/2;
                bool  sel = (ti==m_selTrack && ki==m_selKey);

                QColor col    = sel ? QColor(255,210,0)  : QColor(0,190,255);
                QColor border = sel ? QColor(255,255,120) : QColor(0,140,200);

                QPolygon diamond;
                int r = sel ? 6 : 5;
                diamond << QPoint(x,   cy-r)
                        << QPoint(x+r, cy)
                        << QPoint(x,   cy+r)
                        << QPoint(x-r, cy);
                p.setBrush(col);
                p.setPen(QPen(border, sel ? 2 : 1));
                p.drawPolygon(diamond);
            }
        }
    }

    // ---- Playhead ----
    int px = timeToX(m_playhead);
    p.setPen(QPen(QColor(255,70,70), 2));
    p.drawLine(px, 0, px, height());
    QPolygon tri;
    tri << QPoint(px-5,0) << QPoint(px+5,0) << QPoint(px,8);
    p.setBrush(QColor(255,70,70));
    p.setPen(Qt::NoPen);
    p.drawPolygon(tri);
}

void TimelineWidget::mousePressEvent(QMouseEvent* e) {
    if (!m_clip) return;

    // Hit-test keyframes
    for (int ti = 0; ti < (int)m_clip->tracks.size(); ti++) {
        auto& tr = m_clip->tracks[ti];
        int y = trackY(ti);
        for (int ki = 0; ki < (int)tr.keys.size(); ki++) {
            int x  = timeToX(tr.keys[ki].time);
            int cy = y + kTrackH/2;
            if (std::abs(e->pos().x()-x) <= 6 && std::abs(e->pos().y()-cy) <= 6) {
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

void TimelineWidget::mouseReleaseEvent(QMouseEvent*) {
    if (m_dragging) {
        m_dragging = false;
        if (m_clip && m_selTrack >= 0)
            m_clip->tracks[m_selTrack].sortKeys();
        emit clipEdited();
    }
}
