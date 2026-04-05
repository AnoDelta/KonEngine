#include "TimelineWidget.hpp"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <cmath>
#include <algorithm>

// ── Easing — mirrors PreviewWidget / KonEngine curves.hpp ────────────────
static float applyEaseForCurve(Ease e, float t) {
    using E = Ease;
    switch (e) {
    case E::Linear:         return t;
    case E::EaseIn:         return t * t;
    case E::EaseOut:        return t * (2.0f - t);
    case E::EaseInOut:      return t < 0.5f ? 2*t*t : -1+(4-2*t)*t;
    case E::EaseInCubic:    return t*t*t;
    case E::EaseOutCubic:   { float u=1-t; return 1-u*u*u; }
    case E::EaseInOutCubic: return t<0.5f ? 4*t*t*t : 1-std::pow(-2*t+2,3)/2;
    case E::EaseInElastic:
        if (t==0||t==1) return t;
        return -std::pow(2,10*t-10)*std::sin((t*10-10.75f)*(2*3.14159265f)/3);
    case E::EaseOutElastic:
        if (t==0||t==1) return t;
        return std::pow(2,-10*t)*std::sin((t*10-0.75f)*(2*3.14159265f)/3)+1;
    case E::EaseInOutElastic:
        if (t==0||t==1) return t;
        { const float c=(2*3.14159265f)/4.5f;
          return t<0.5f
            ? -(std::pow(2, 20*t-10)*std::sin((20*t-11.125f)*c))/2
            :  (std::pow(2,-20*t+10)*std::sin((20*t-11.125f)*c))/2+1; }
    case E::EaseOutBounce: {
        float n=7.5625f,d=2.75f;
        if (t<1/d)    return n*t*t;
        if (t<2/d)    { t-=1.5f/d;  return n*t*t+0.75f; }
        if (t<2.5f/d) { t-=2.25f/d; return n*t*t+0.9375f; }
                        t-=2.625f/d; return n*t*t+0.984375f; }
    case E::EaseInBounce:
        return 1-applyEaseForCurve(E::EaseOutBounce,1-t);
    case E::EaseInOutBounce:
        return t<0.5f
            ? (1-applyEaseForCurve(E::EaseOutBounce,1-2*t))/2
            : (1+applyEaseForCurve(E::EaseOutBounce,2*t-1))/2;
    case E::EaseInBack:
        { float c=1.70158f; return (c+1)*t*t*t-c*t*t; }
    case E::EaseOutBack:
        { float c=1.70158f,u=t-1; return 1+(c+1)*u*u*u+c*u*u; }
    case E::EaseInOutBack:
        { float c=1.70158f*1.525f;
          return t<0.5f
            ? (std::pow(2*t,2)*((c+1)*2*t-c))/2
            : (std::pow(2*t-2,2)*((c+1)*(2*t-2)+c)+2)/2; }
    default: return t;
    }
}

TimelineWidget::TimelineWidget(QWidget* parent) : QWidget(parent) {
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    setMinimumHeight(kHeaderH + kTrackH);
}

// ── Height / scroll helpers ───────────────────────────────────────────────────
void TimelineWidget::updateHeight() {
    int tracks = m_clip ? (int)m_clip->tracks.size() : 0;
    setMinimumHeight(kHeaderH + std::max(1, tracks) * kTrackH + 4);
}

void TimelineWidget::clampScroll() {
    float maxScroll = std::max(0.0f, visibleDuration() * m_zoom - (width() - kLabelW));
    m_scroll = std::max(0.0f, std::min(m_scroll, maxScroll));
}

float TimelineWidget::visibleDuration() const {
    float clipDur = m_clip ? m_clip->totalDuration() : 0.0f;
    // Always show at least the clip duration + padding, so the ruler is endless
    // from the user's POV (they can always scroll right)
    return std::max(10.0f, clipDur + kPadding);
}

// ── Public API ────────────────────────────────────────────────────────────────
void TimelineWidget::setClip(AnimClip* clip) {
    m_clip     = clip;
    m_playhead = 0.0f;
    m_scroll   = 0.0f;
    m_selTrack = m_selKey = -1;
    updateHeight();
    update();
}

void TimelineWidget::refreshClip() {
    updateHeight();
    update();
}

void TimelineWidget::setPlayhead(float t) {
    m_playhead = t;
    // Auto-scroll to keep the playhead visible
    int px = timeToX(t);
    if (px < kLabelW + 10) {
        m_scroll = std::max(0.0f, t * m_zoom - 20.0f);
        clampScroll();
    } else if (px > width() - 20) {
        m_scroll = t * m_zoom - (width() - kLabelW - 40);
        clampScroll();
    }
    update();
}

// ── Paint ─────────────────────────────────────────────────────────────────────
void TimelineWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    p.fillRect(rect(), QColor(32,32,32));

    // ---- Ruler background ----
    p.fillRect(0, 0, width(), kHeaderH, QColor(22,22,22));

    // --- Adaptive ruler ---
    // kMinPx: minimum pixel gap between labelled ticks — prevents overlap.
    const float kMinPx = 56.0f;

    // Candidate steps in ascending order (seconds)
    static const float kSteps[] = {
        0.05f, 0.1f, 0.2f, 0.25f, 0.5f,
        1.0f, 2.0f, 5.0f, 10.0f, 30.0f, 60.0f
    };

    // Choose the smallest step that still gives kMinPx between labels at this zoom
    float step = kSteps[sizeof(kSteps)/sizeof(float)-1];
    for (float s : kSteps) {
        if (s * m_zoom >= kMinPx) { step = s; break; }
    }

    // Minor tick subdivision — only draw if they'd be at least 4px apart
    float minorStep = step / 5.0f;
    if (minorStep * m_zoom < 4.0f) minorStep = step;

    // Decimal precision for labels
    int decimals = (step >= 1.0f) ? 0 : (step >= 0.1f) ? 1 : 2;

    float tStart = std::max(0.0f, xToTime(kLabelW));
    float tEnd   = xToTime(width()) + step;

    // Minor ticks first (no labels)
    p.setPen(QColor(55,55,55));
    for (float t = std::ceil(tStart / minorStep) * minorStep; t <= tEnd; t += minorStep) {
        int x = timeToX(t);
        if (x <= kLabelW || x > width()) continue;
        p.drawLine(x, 16, x, kHeaderH);
    }

    // Major ticks + labels
    for (float t = std::ceil(tStart / step) * step; t <= tEnd; t += step) {
        int x = timeToX(t);
        if (x <= kLabelW || x > width()) continue;
        p.setPen(QColor(140,140,140));
        p.drawLine(x, 4, x, kHeaderH);
        p.setPen(QColor(170,170,170));
        p.setFont(QFont("monospace", 8));
        p.drawText(x + 3, kHeaderH - 4, QString::number(t, 'f', decimals) + "s");
    }

    // ---- Label column background ----
    p.fillRect(0, 0, kLabelW, height(), QColor(22,22,22));
    p.setPen(QColor(50,50,50));
    p.drawLine(kLabelW, 0, kLabelW, height());

    // ---- Tracks ----
    if (m_clip) {
        for (int ti = 0; ti < (int)m_clip->tracks.size(); ti++) {
            auto& tr = m_clip->tracks[ti];
            int y = trackY(ti);

            p.fillRect(0, y, width(), kTrackH,
                ti%2==0 ? QColor(40,40,40) : QColor(34,34,34));
            p.fillRect(0, y, kLabelW, kTrackH, QColor(24,24,24));

            p.setPen(QColor(185,185,185));
            p.setFont(QFont("monospace", 9));
            p.drawText(QRect(4, y, kLabelW-8, kTrackH),
                Qt::AlignVCenter|Qt::AlignLeft,
                QString::fromStdString(tr.name));

            p.setPen(QColor(45,45,45));
            p.drawLine(0, y+kTrackH-1, width(), y+kTrackH-1);

            // Interpolation curve between adjacent keyframes (selected track only)
            if (ti == m_selTrack && tr.keys.size() >= 2) {
                constexpr int kSamples = 20;
                int cy = y + kTrackH / 2;
                int halfH = kTrackH / 2 - 3; // vertical extent of curve preview

                for (int ki = 0; ki + 1 < (int)tr.keys.size(); ki++) {
                    const Keyframe& kA = tr.keys[ki];
                    const Keyframe& kB = tr.keys[ki + 1];
                    int x0 = timeToX(kA.time);
                    int x1 = timeToX(kB.time);
                    // Skip if entirely off-screen
                    if (x1 < kLabelW - 8 || x0 > width() + 8) continue;

                    QPainterPath curvePath;
                    for (int s = 0; s <= kSamples; s++) {
                        float frac = (float)s / (float)kSamples;
                        float eased = applyEaseForCurve(kA.curve, frac);
                        // Clamp for elastic/bounce overshoot
                        float clamped = std::max(-0.2f, std::min(1.2f, eased));
                        int px = x0 + (int)((x1 - x0) * frac);
                        // y: top of track = eased 1.0, bottom = eased 0.0
                        int py = cy + (int)(halfH * (1.0f - 2.0f * clamped));
                        if (s == 0)
                            curvePath.moveTo(px, py);
                        else
                            curvePath.lineTo(px, py);
                    }
                    p.setBrush(Qt::NoBrush);
                    p.setPen(QPen(QColor(120, 220, 120, 160), 1.5));
                    p.drawPath(curvePath);
                }
            }

            // Keyframe diamonds
            for (int ki = 0; ki < (int)tr.keys.size(); ki++) {
                int   x   = timeToX(tr.keys[ki].time);
                if (x < kLabelW - 8 || x > width() + 8) continue;
                int   cy  = y + kTrackH/2;
                bool  sel = (ti==m_selTrack && ki==m_selKey);

                QColor col    = sel ? QColor(255,210,0)  : QColor(0,190,255);
                QColor border = sel ? QColor(255,255,120) : QColor(0,140,200);

                QPolygon d;
                int r = sel ? 6 : 5;
                d << QPoint(x,   cy-r) << QPoint(x+r, cy)
                  << QPoint(x,   cy+r) << QPoint(x-r, cy);
                p.setBrush(col);
                p.setPen(QPen(border, sel ? 2 : 1));
                p.drawPolygon(d);
            }
        }
    }

    // ---- Clip end marker ----
    if (m_clip) {
        float endT = m_clip->totalDuration();
        if (endT > 0) {
            int ex = timeToX(endT);
            if (ex >= kLabelW && ex <= width()) {
                // Shaded region after end
                p.fillRect(ex, kHeaderH, width()-ex, height()-kHeaderH,
                    QColor(0,0,0,60));
                // Orange vertical line
                p.setPen(QPen(QColor(255,140,0), 2));
                p.drawLine(ex, 0, ex, height());
                // "END" label
                p.setPen(QColor(255,140,0));
                p.setFont(QFont("monospace", 7, QFont::Bold));
                p.drawText(ex+3, kHeaderH-4, "END");
            }
        }
    }

    // ---- Playhead ----
    int px = timeToX(m_playhead);
    if (px >= kLabelW && px <= width()) {
        p.setPen(QPen(QColor(255,60,60), 2));
        p.drawLine(px, 0, px, height());
        QPolygon tri;
        tri << QPoint(px-5,0) << QPoint(px+5,0) << QPoint(px,9);
        p.setBrush(QColor(255,60,60));
        p.setPen(Qt::NoPen);
        p.drawPolygon(tri);
    }
}

// ── Mouse ─────────────────────────────────────────────────────────────────────
void TimelineWidget::mousePressEvent(QMouseEvent* e) {
    // Right-click → pan
    if (e->button() == Qt::RightButton) {
        m_rightPanning   = true;
        m_rightPanStart  = e->pos().x();
        m_rightScrollOrg = m_scroll;
        setCursor(Qt::ClosedHandCursor);
        return;
    }

    if (!m_clip) return;

    // Middle-button or alt+left: ruler pan
    if (e->button() == Qt::MiddleButton ||
        (e->button() == Qt::LeftButton && e->modifiers() & Qt::AltModifier)) {
        m_panningRuler  = true;
        m_panStart      = e->pos().x();
        m_scrollAtPan   = m_scroll;
        setCursor(Qt::ClosedHandCursor);
        return;
    }

    if (e->button() != Qt::LeftButton) return;

    // Hit-test keyframes first
    for (int ti = 0; ti < (int)m_clip->tracks.size(); ti++) {
        auto& tr = m_clip->tracks[ti];
        int y = trackY(ti);
        for (int ki = 0; ki < (int)tr.keys.size(); ki++) {
            int x  = timeToX(tr.keys[ki].time);
            int cy = y + kTrackH/2;
            if (std::abs(e->pos().x()-x) <= 7 && std::abs(e->pos().y()-cy) <= 7) {
                m_selTrack = ti; m_selKey = ki;
                m_dragging = true;
                emit keyframeSelected(ti, ki);
                update();
                return;
            }
        }
    }

    // Click ruler → set playhead
    if (e->pos().y() < kHeaderH || !m_clip) {
        float t = std::max(0.0f, xToTime(e->pos().x()));
        m_playhead = t;
        emit playheadChanged(t);
        update();
        return;
    }

    // Click track area (not on a diamond) → move playhead there too
    float t = std::max(0.0f, xToTime(e->pos().x()));
    m_playhead = t;
    emit playheadChanged(t);
    update();
}

void TimelineWidget::mouseMoveEvent(QMouseEvent* e) {
    // Right-click pan
    if (m_rightPanning) {
        int delta = e->pos().x() - m_rightPanStart;
        m_scroll = m_rightScrollOrg - delta;
        clampScroll();
        update();
        return;
    }

    if (m_panningRuler) {
        int delta = e->pos().x() - m_panStart;
        m_scroll = m_scrollAtPan - delta;
        clampScroll();
        update();
        return;
    }

    if (m_dragging && m_selTrack >= 0 && m_selKey >= 0 && m_clip) {
        float t = std::max(0.0f, xToTime(e->pos().x()));
        m_clip->tracks[m_selTrack].keys[m_selKey].time = t;
        emit keyframeMoved(m_selTrack, m_selKey, t);
        update();
        return;
    }

    if (e->buttons() & Qt::LeftButton) {
        float t = std::max(0.0f, xToTime(e->pos().x()));
        m_playhead = t;
        emit playheadChanged(t);
        update();
    }
}

void TimelineWidget::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::RightButton) {
        m_rightPanning = false;
        setCursor(Qt::ArrowCursor);
        return;
    }
    if (m_panningRuler) {
        m_panningRuler = false;
        setCursor(Qt::ArrowCursor);
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
        // Ctrl+scroll → zoom around cursor
        float factor = e->angleDelta().y() > 0 ? 1.2f : 1.0f/1.2f;
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
        float cursorX = e->position().x();
#else
        float cursorX = e->pos().x();
#endif
        float tUnderCursor = xToTime((int)cursorX);
        m_zoom = std::max(8.0f, std::min(m_zoom * factor, 600.0f));
        // Keep the time under the cursor fixed
        m_scroll = tUnderCursor * m_zoom - (cursorX - kLabelW);
        clampScroll();
    } else {
        // Plain scroll → pan left/right
        m_scroll -= e->angleDelta().y() * 0.5f;
        clampScroll();
    }
    update();
}

void TimelineWidget::resizeEvent(QResizeEvent* e) {
    QWidget::resizeEvent(e);
    clampScroll();
}
