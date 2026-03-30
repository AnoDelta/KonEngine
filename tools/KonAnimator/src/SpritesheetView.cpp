#include "SpritesheetView.hpp"
#include <QPainter>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QApplication>
#include <algorithm>
#include <cmath>

static constexpr int   kEdge = 7;
static constexpr float kSnap = 8.0f;

SpritesheetView::SpritesheetView(QWidget* parent) : QWidget(parent) {
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // Dark background
    setAutoFillBackground(false);
}

// ── Setters ───────────────────────────────────────────────────────────────────
void SpritesheetView::setPixmap(const QPixmap& px) { m_pixmap=px; update(); }
void SpritesheetView::setFrames(const std::vector<AnimFrame>* f) { m_frames=f; update(); }
void SpritesheetView::setSelectedFrame(int i)  { m_selectedIdx=i;  update(); }
void SpritesheetView::setHighlightFrame(int i) { m_highlightIdx=i; update(); }
void SpritesheetView::setZoom(float z) {
    // Zoom toward center of widget
    float cx = width()  * 0.5f;
    float cy = height() * 0.5f;
    // Sheet position under centre before zoom
    float sx = (cx - m_panX) / m_zoom;
    float sy = (cy - m_panY) / m_zoom;
    m_zoom = std::max(0.25f, std::min(z, 32.0f));
    // Keep that sheet point under centre
    m_panX = cx - sx * m_zoom;
    m_panY = cy - sy * m_zoom;
    update();
}

// ── Coordinates ───────────────────────────────────────────────────────────────
QPointF SpritesheetView::widgetToSheet(QPoint p) const {
    return { (p.x() - m_panX) / m_zoom,
             (p.y() - m_panY) / m_zoom };
}
QPoint SpritesheetView::sheetToWidget(float sx, float sy) const {
    return { (int)(sx * m_zoom + m_panX),
             (int)(sy * m_zoom + m_panY) };
}
QRect SpritesheetView::frameToWidget(const AnimFrame& f) const {
    QPoint tl = sheetToWidget(f.srcX, f.srcY);
    QPoint br = sheetToWidget(f.srcX + f.srcW, f.srcY + f.srcH);
    return QRect(tl, br);
}

// ── Snapping ──────────────────────────────────────────────────────────────────
void SpritesheetView::collectEdges(std::vector<float>& xs, std::vector<float>& ys, int skip) const {
    xs.push_back(0); ys.push_back(0);
    if (!m_pixmap.isNull()) {
        xs.push_back((float)m_pixmap.width());
        ys.push_back((float)m_pixmap.height());
    }
    if (!m_frames) return;
    for (int i=0;i<(int)m_frames->size();i++) {
        if (i==skip) continue;
        const auto& f=(*m_frames)[i];
        xs.push_back(f.srcX); xs.push_back(f.srcX+f.srcW);
        ys.push_back(f.srcY); ys.push_back(f.srcY+f.srcH);
    }
}
float SpritesheetView::snapTo(float v, const std::vector<float>& c, float t) {
    float best=v, bd=t;
    for (float x:c) { float d=std::fabs(v-x); if(d<bd){bd=d;best=x;} }
    return best;
}
AnimFrame SpritesheetView::doSnap(AnimFrame fr, bool snap, int skip) const {
    if (!snap) return fr;
    std::vector<float> xs,ys;
    collectEdges(xs,ys,skip);
    float x2=fr.srcX+fr.srcW, y2=fr.srcY+fr.srcH;
    fr.srcX=snapTo(fr.srcX,xs,kSnap);
    fr.srcY=snapTo(fr.srcY,ys,kSnap);
    x2=snapTo(x2,xs,kSnap);
    y2=snapTo(y2,ys,kSnap);
    fr.srcW=std::max(1.0f,x2-fr.srcX);
    fr.srcH=std::max(1.0f,y2-fr.srcY);
    return fr;
}

// ── Hit testing ───────────────────────────────────────────────────────────────
SpritesheetView::DragMode SpritesheetView::hitTest(QPoint pos, int& out) const {
    if (!m_frames) return DragMode::None;
    for (int i=(int)m_frames->size()-1;i>=0;i--) {
        QRect r=frameToWidget((*m_frames)[i]);
        bool nL=std::abs(pos.x()-r.left())  <kEdge;
        bool nR=std::abs(pos.x()-r.right()) <kEdge;
        bool nT=std::abs(pos.y()-r.top())   <kEdge;
        bool nB=std::abs(pos.y()-r.bottom())<kEdge;
        bool inX=pos.x()>=r.left()-kEdge && pos.x()<=r.right()+kEdge;
        bool inY=pos.y()>=r.top()-kEdge  && pos.y()<=r.bottom()+kEdge;
        if (!inX||!inY) continue;
        out=i;
        if(nL&&nT) return DragMode::ResizeTL;
        if(nR&&nT) return DragMode::ResizeTR;
        if(nL&&nB) return DragMode::ResizeBL;
        if(nR&&nB) return DragMode::ResizeBR;
        if(nL)     return DragMode::ResizeL;
        if(nR)     return DragMode::ResizeR;
        if(nT)     return DragMode::ResizeT;
        if(nB)     return DragMode::ResizeB;
        if(r.contains(pos)) return DragMode::Move;
    }
    return DragMode::None;
}
Qt::CursorShape SpritesheetView::cursorFor(DragMode m) const {
    switch(m) {
    case DragMode::Move:                            return Qt::SizeAllCursor;
    case DragMode::ResizeL: case DragMode::ResizeR: return Qt::SizeHorCursor;
    case DragMode::ResizeT: case DragMode::ResizeB: return Qt::SizeVerCursor;
    case DragMode::ResizeTL: case DragMode::ResizeBR: return Qt::SizeFDiagCursor;
    case DragMode::ResizeTR: case DragMode::ResizeBL: return Qt::SizeBDiagCursor;
    default: return Qt::ArrowCursor;
    }
}

// ── Paint ─────────────────────────────────────────────────────────────────────
void SpritesheetView::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::SmoothPixmapTransform, false);

    // Checkerboard background
    const int cs=8;
    for (int y=0;y<height();y+=cs)
        for (int x=0;x<width();x+=cs)
            p.fillRect(x,y,cs,cs, ((x/cs+y/cs)%2==0)?QColor(48,48,48):QColor(34,34,34));

    if (m_pixmap.isNull()) {
        p.setPen(QColor(120,120,120));
        p.drawText(rect(), Qt::AlignCenter,
            "No spritesheet loaded\n\nFile → Load Spritesheet\n\n"
            "Right-drag to pan   •   Scroll to zoom");
        return;
    }

    // Sheet image at current pan+zoom
    QRect dst(
        (int)m_panX, (int)m_panY,
        (int)(m_pixmap.width()  * m_zoom),
        (int)(m_pixmap.height() * m_zoom));
    p.drawPixmap(dst, m_pixmap);

    // Thin border around sheet
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(QColor(80,80,80), 1));
    p.drawRect(dst);

    if (!m_frames) return;

    for (int i=0;i<(int)m_frames->size();i++) {
        AnimFrame fr=(*m_frames)[i];
        if (m_drag!=DragMode::None && m_drag!=DragMode::Create && i==m_dIdx)
            fr=m_live;

        QRect r=frameToWidget(fr);
        bool sel  = (i==m_selectedIdx);
        bool play = (i==m_highlightIdx);

        QColor fill, border;
        int    bw;
        if (play) {
            fill=QColor(255,220,0,70); border=QColor(255,220,0); bw=2;
        } else if (sel) {
            fill=QColor(0,180,255,55); border=QColor(80,180,255); bw=2;
        } else {
            fill=QColor(255,255,255,22); border=QColor(200,200,200,140); bw=1;
        }

        p.fillRect(r, fill);
        p.setPen(QPen(border, bw));
        p.drawRect(r);
        p.setPen(border);
        p.setFont(QFont("monospace", std::max(6, (int)(m_zoom*2.5f))));
        p.drawText(r.adjusted(2,1,0,0), QString::number(i));

        // Resize handles + move hint on selected frame (only when idle)
        if (sel && m_drag==DragMode::None) {
            p.setBrush(QColor(255,255,255,210));
            p.setPen(QPen(QColor(50,50,50), 1));
            const int hs=5;
            auto dot=[&](int x,int y){ p.drawRect(x-hs/2,y-hs/2,hs,hs); };
            dot(r.left(),r.top());    dot(r.center().x(),r.top());    dot(r.right(),r.top());
            dot(r.left(),r.center().y());                              dot(r.right(),r.center().y());
            dot(r.left(),r.bottom()); dot(r.center().x(),r.bottom()); dot(r.right(),r.bottom());
            p.setBrush(Qt::NoBrush);
            p.setPen(QColor(180,180,180,180));
            p.setFont(QFont("monospace",7));
            p.drawText(r, Qt::AlignCenter, "+");
        }
    }

    // Create-drag orange preview
    if (m_drag==DragMode::Create) {
        bool snap=!(QApplication::keyboardModifiers()&Qt::ControlModifier);
        QRect drag=QRect(m_dStart,m_dCur).normalized();
        QPointF tl=widgetToSheet(drag.topLeft());
        QPointF br=widgetToSheet(drag.bottomRight());
        if (snap) {
            std::vector<float> xs,ys; collectEdges(xs,ys);
            tl={snapTo((float)tl.x(),xs,kSnap), snapTo((float)tl.y(),ys,kSnap)};
            br={snapTo((float)br.x(),xs,kSnap), snapTo((float)br.y(),ys,kSnap)};
        }
        QPoint sTL=sheetToWidget((float)tl.x(),(float)tl.y());
        QPoint sBR=sheetToWidget((float)br.x(),(float)br.y());
        QRect  sn=QRect(sTL,sBR).normalized();
        p.fillRect(sn, QColor(255,120,0,50));
        p.setPen(QPen(QColor(255,160,0),1,Qt::DashLine));
        p.drawRect(sn);
        QString info=QString("%1×%2 px")
            .arg(std::max(1,(int)(br.x()-tl.x())))
            .arg(std::max(1,(int)(br.y()-tl.y())));
        if (!snap) info+=" [no snap]";
        p.setPen(QColor(255,200,100));
        p.setFont(QFont("monospace",9));
        p.drawText(sn.bottomLeft()+QPoint(2,14), info);
    }

    // Pan-mode indicator
    if (m_panning) {
        p.setPen(QColor(255,255,255,80));
        p.setFont(QFont("monospace",9));
        p.drawText(rect().adjusted(6,6,0,0), "Panning...");
    }
}

// ── Keys ──────────────────────────────────────────────────────────────────────
void SpritesheetView::keyPressEvent(QKeyEvent* e) {
    if (e->key()==Qt::Key_Escape && m_selectedIdx>=0) {
        m_selectedIdx=-1; update(); emit frameDeselected(); return;
    }
    // Double-press Home or 0 to reset view
    if (e->key()==Qt::Key_Home || e->key()==Qt::Key_0) {
        m_panX=8; m_panY=8; update(); return;
    }
    QWidget::keyPressEvent(e);
}

// ── Mouse ─────────────────────────────────────────────────────────────────────
void SpritesheetView::mousePressEvent(QMouseEvent* e) {
    setFocus();

    // ── Right-click → pan ─────────────────────────────────────────────────
    if (e->button()==Qt::RightButton) {
        m_panning     = true;
        m_panPress    = e->pos();
        m_panXAtPress = m_panX;
        m_panYAtPress = m_panY;
        setCursor(Qt::ClosedHandCursor);
        update();
        return;
    }

    if (e->button()!=Qt::LeftButton) return;

    int      idx=-1;
    DragMode dm=hitTest(e->pos(),idx);

    if (dm==DragMode::Move) {
        m_drag=DragMode::Move; m_dIdx=idx;
        m_orig=(*m_frames)[idx]; m_live=m_orig;
        m_dStart=e->pos(); m_selectedIdx=idx;
        setCursor(Qt::SizeAllCursor);
        emit frameClicked(idx); update(); return;
    }
    if (dm!=DragMode::None) {
        m_drag=dm; m_dIdx=idx;
        m_orig=(*m_frames)[idx]; m_live=m_orig;
        m_dStart=e->pos(); m_selectedIdx=idx;
        setCursor(cursorFor(dm));
        emit frameClicked(idx); update(); return;
    }

    // Empty space — start potential create drag, keep selection
    m_drag=DragMode::Create;
    m_dStart=m_dCur=e->pos();
}

void SpritesheetView::mouseMoveEvent(QMouseEvent* e) {
    // ── Pan (right-drag) — pure widget-space delta, always works ────────
    if (m_panning) {
        QPoint delta = e->pos() - m_panPress;
        m_panX = m_panXAtPress + delta.x();
        m_panY = m_panYAtPress + delta.y();
        update();
        return;
    }

    // ── Create preview ───────────────────────────────────────────────────
    if (m_drag==DragMode::Create) { m_dCur=e->pos(); update(); return; }

    // ── Move frame ───────────────────────────────────────────────────────
    if (m_drag==DragMode::Move) {
        bool snap=!(QApplication::keyboardModifiers()&Qt::ControlModifier);
        QPointF ds=widgetToSheet(e->pos())-widgetToSheet(m_dStart);
        AnimFrame fr=m_orig;
        fr.srcX=std::max(0.0f, m_orig.srcX+(float)ds.x());
        fr.srcY=std::max(0.0f, m_orig.srcY+(float)ds.y());
        if (snap) {
            std::vector<float> xs,ys; collectEdges(xs,ys,m_dIdx);
            float snL=snapTo(fr.srcX,xs,kSnap);
            float snR=snapTo(fr.srcX+fr.srcW,xs,kSnap)-fr.srcW;
            fr.srcX=std::max(0.0f, std::fabs(snL-fr.srcX)<=std::fabs(snR-fr.srcX)?snL:snR);
            float snT=snapTo(fr.srcY,ys,kSnap);
            float snB=snapTo(fr.srcY+fr.srcH,ys,kSnap)-fr.srcH;
            fr.srcY=std::max(0.0f, std::fabs(snT-fr.srcY)<=std::fabs(snB-fr.srcY)?snT:snB);
        }
        m_live=fr; emit frameResized(m_dIdx,fr); update(); return;
    }

    // ── Resize frame ─────────────────────────────────────────────────────
    if (m_drag!=DragMode::None) {
        bool snap=!(QApplication::keyboardModifiers()&Qt::ControlModifier);
        QPointF sc=widgetToSheet(e->pos()), ss=widgetToSheet(m_dStart);
        float dx=(float)(sc.x()-ss.x()), dy=(float)(sc.y()-ss.y());
        AnimFrame fr=m_orig;
        float r=fr.srcX+fr.srcW, b=fr.srcY+fr.srcH;
        switch(m_drag) {
        case DragMode::ResizeL:  fr.srcX+=dx;           break;
        case DragMode::ResizeR:  r+=dx;                 break;
        case DragMode::ResizeT:  fr.srcY+=dy;           break;
        case DragMode::ResizeB:  b+=dy;                 break;
        case DragMode::ResizeTL: fr.srcX+=dx; fr.srcY+=dy; break;
        case DragMode::ResizeTR: r+=dx;       fr.srcY+=dy; break;
        case DragMode::ResizeBL: fr.srcX+=dx; b+=dy;    break;
        case DragMode::ResizeBR: r+=dx;       b+=dy;    break;
        default: break;
        }
        if(r<fr.srcX+1)r=fr.srcX+1; if(b<fr.srcY+1)b=fr.srcY+1;
        fr.srcW=r-fr.srcX; fr.srcH=b-fr.srcY;
        fr=doSnap(fr,snap,m_dIdx);
        m_live=fr; emit frameResized(m_dIdx,fr); update(); return;
    }

    // Hover — update cursor shape
    int dummy=-1;
    setCursor(cursorFor(hitTest(e->pos(),dummy)));
}

void SpritesheetView::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::RightButton) {
        m_panning=false; setCursor(Qt::ArrowCursor); update(); return;
    }
    if (e->button()!=Qt::LeftButton) return;

    if (m_drag==DragMode::Create) {
        m_drag=DragMode::None;
        QRect drag=QRect(m_dStart,m_dCur).normalized();
        if (drag.width()>=6 && drag.height()>=6) {
            bool snap=!(QApplication::keyboardModifiers()&Qt::ControlModifier);
            QPointF tl=widgetToSheet(drag.topLeft());
            QPointF br=widgetToSheet(drag.bottomRight());
            if (snap) {
                std::vector<float> xs,ys; collectEdges(xs,ys);
                tl={snapTo((float)tl.x(),xs,kSnap),snapTo((float)tl.y(),ys,kSnap)};
                br={snapTo((float)br.x(),xs,kSnap),snapTo((float)br.y(),ys,kSnap)};
            }
            AnimFrame fr;
            fr.srcX=std::max(0.0f,(float)tl.x()); fr.srcY=std::max(0.0f,(float)tl.y());
            fr.srcW=std::max(1.0f,(float)(br.x()-tl.x())); fr.srcH=std::max(1.0f,(float)(br.y()-tl.y()));
            fr.duration=0.1f;
            emit frameAdded(fr);
        }
        setCursor(Qt::ArrowCursor); update(); return;
    }

    if (m_drag!=DragMode::None) {
        emit frameResized(m_dIdx,m_live);
        m_drag=DragMode::None; m_dIdx=-1;
        setCursor(Qt::ArrowCursor); update();
    }
}

void SpritesheetView::mouseDoubleClickEvent(QMouseEvent*) {
    // Double-click resets pan/zoom to default view
    m_zoom=2.0f; m_panX=8; m_panY=8; update();
}

void SpritesheetView::wheelEvent(QWheelEvent* e) {
    float factor = e->angleDelta().y() > 0 ? 1.25f : 0.8f;
    // Zoom toward cursor position
    float mx, my;
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
    mx = e->position().x(); my = e->position().y();
#else
    mx = e->pos().x(); my = e->pos().y();
#endif
    float sx=(mx-m_panX)/m_zoom, sy=(my-m_panY)/m_zoom;
    m_zoom=std::max(0.25f,std::min(m_zoom*factor,32.0f));
    m_panX=mx-sx*m_zoom;
    m_panY=my-sy*m_zoom;
    update();
}
