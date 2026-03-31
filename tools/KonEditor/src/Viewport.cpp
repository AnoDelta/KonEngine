#include "Viewport.hpp"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPen>
#include <QFont>
#include <QFontMetrics>
#include <cmath>

// Node type → display color
static QColor nodeColor(const QString& type) {
    if (type == "RigidBody2D")    return {230, 126,  34};
    if (type == "StaticBody2D")   return {149, 165, 166};
    if (type == "KinematicBody2D")return { 52, 152, 219};
    if (type == "Area2D")         return {155,  89, 182};
    if (type == "CollisionShape2D")return{231,  76,  60};
    if (type == "Sprite2D")       return { 46, 204, 113};
    if (type == "AnimatedSprite2D")return{26, 188, 156};
    if (type == "Camera2D" || type == "CameraNode2D") return { 52, 152, 219};
    if (type == "Label")          return {241, 196,  15};
    if (type == "Node2D")         return {127, 140, 141};
    if (type == "Timer")          return {189, 195, 199};
    if (type == "AudioPlayer")    return {230, 126,  34};
    return {100, 100, 100};
}

static QString nodeIcon(const QString& type) {
    if (type == "RigidBody2D")     return "⚡";
    if (type == "StaticBody2D")    return "🧱";
    if (type == "KinematicBody2D") return "🏃";
    if (type == "Area2D")          return "○";
    if (type == "CollisionShape2D")return "⬜";
    if (type == "Sprite2D")        return "🖼";
    if (type == "Camera2D" || type == "CameraNode2D") return "📷";
    if (type == "Label")           return "T";
    if (type == "Timer")           return "⏱";
    return "◆";
}

Viewport::Viewport(QWidget* parent) : QWidget(parent) {
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(400, 300);
    // Center pan on game resolution
    m_panX = 0;
    m_panY = 0;
}

void Viewport::setNodes(const QList<ViewportNode>& nodes) {
    m_nodes = nodes;
    update();
}

void Viewport::clearNodes() {
    m_nodes.clear();
    update();
}

void Viewport::setGameResolution(int w, int h) {
    m_gameW = w;
    m_gameH = h;
    update();
}

void Viewport::selectNode(const QString& name) {
    for (auto& n : m_nodes)
        n.selected = (n.name == name);
    update();
}

// ── Coordinate transforms ─────────────────────────────────────────────────
QPointF Viewport::worldToScreen(float x, float y) const {
    float cx = width()  * 0.5f + m_panX;
    float cy = height() * 0.5f + m_panY;
    return { cx + x * m_zoom, cy + y * m_zoom };
}

QPointF Viewport::screenToWorld(float x, float y) const {
    float cx = width()  * 0.5f + m_panX;
    float cy = height() * 0.5f + m_panY;
    return { (x - cx) / m_zoom, (y - cy) / m_zoom };
}

// ── Hit testing ───────────────────────────────────────────────────────────
ViewportNode* Viewport::nodeAt(QPointF sp) {
    // Iterate in reverse so top-drawn nodes are hit first
    for (int i = m_nodes.size()-1; i >= 0; i--) {
        auto& n = m_nodes[i];
        if (n.type == "Camera2D" || n.type == "CameraNode2D") continue;
        QPointF s = worldToScreen(n.x, n.y);
        float hw = (n.w * 0.5f) * m_zoom;
        float hh = (n.h * 0.5f) * m_zoom;
        if (sp.x() >= s.x()-hw && sp.x() <= s.x()+hw &&
            sp.y() >= s.y()-hh && sp.y() <= s.y()+hh)
            return &n;
    }
    return nullptr;
}

// ── Paint ─────────────────────────────────────────────────────────────────
void Viewport::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Background
    p.fillRect(rect(), QColor(18, 18, 18));

    // Grid
    drawGrid(p);

    // Origin cross
    drawOriginCross(p);

    // Game bounds rectangle
    QPointF tl = worldToScreen(0,        0);
    QPointF br = worldToScreen(m_gameW,  m_gameH);
    p.setPen(QPen(QColor(60, 60, 70), 1, Qt::DashLine));
    p.setBrush(Qt::NoBrush);
    p.drawRect(QRectF(tl, br));

    // Dim outside game bounds
    QColor dim(0, 0, 0, 80);
    p.fillRect(QRectF(0, 0, tl.x(), height()), dim);
    p.fillRect(QRectF(br.x(), 0, width()-br.x(), height()), dim);
    p.fillRect(QRectF(tl.x(), 0, br.x()-tl.x(), tl.y()), dim);
    p.fillRect(QRectF(tl.x(), br.y(), br.x()-tl.x(), height()-br.y()), dim);

    // Draw camera rect first (behind nodes)
    for (auto& n : m_nodes)
        if (n.type == "Camera2D" || n.type == "CameraNode2D") drawCameraRect(p, n);

    // Draw nodes
    for (auto& n : m_nodes)
        if (n.type != "Camera2D" && n.type != "CameraNode2D") drawNode(p, n);

    // Draw cameras on top
    for (auto& n : m_nodes)
        if (n.type == "Camera2D" || n.type == "CameraNode2D") drawNode(p, n);

    // Info overlay
    p.setPen(QColor(80, 80, 80));
    p.setFont(QFont("monospace", 9));
    p.drawText(8, height()-8,
        QString("zoom: %1x  pan: (%2, %3)  nodes: %4")
        .arg(m_zoom, 0, 'f', 2)
        .arg((int)(-m_panX/m_zoom))
        .arg((int)(-m_panY/m_zoom))
        .arg(m_nodes.size()));
}

void Viewport::drawGrid(QPainter& p) {
    float step = GRID_STEP * m_zoom;
    if (step < 8) return; // too small to draw

    float cx = width()  * 0.5f + m_panX;
    float cy = height() * 0.5f + m_panY;

    p.setPen(QPen(QColor(30, 30, 30), 1));

    // Vertical lines
    float startX = std::fmod(cx, step);
    for (float x = startX; x < width(); x += step)
        p.drawLine(QPointF(x, 0), QPointF(x, height()));

    // Horizontal lines
    float startY = std::fmod(cy, step);
    for (float y = startY; y < height(); y += step)
        p.drawLine(QPointF(0, y), QPointF(width(), y));
}

void Viewport::drawOriginCross(QPainter& p) {
    QPointF o = worldToScreen(0, 0);
    p.setPen(QPen(QColor(60, 60, 80), 1));
    p.drawLine(QPointF(o.x(), 0), QPointF(o.x(), height()));
    p.drawLine(QPointF(0, o.y()), QPointF(width(), o.y()));
    // Bright dot at exact origin
    p.setPen(QPen(QColor(100, 100, 140), 2));
    p.drawEllipse(o, 3, 3);
}

void Viewport::drawNode(QPainter& p, ViewportNode& n) {
    QPointF s  = worldToScreen(n.x, n.y);
    float   hw = (n.w * 0.5f) * m_zoom;
    float   hh = (n.h * 0.5f) * m_zoom;
    QColor  col = nodeColor(n.type);

    // Box
    QRectF box(s.x()-hw, s.y()-hh, hw*2, hh*2);
    p.setPen(Qt::NoPen);
    QColor fill = col;
    fill.setAlpha(n.selected ? 180 : 80);
    p.setBrush(fill);
    p.drawRoundedRect(box, 3, 3);

    // Border
    QPen border(n.selected ? QColor(255,255,255) : col, n.selected ? 2 : 1);
    p.setPen(border);
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(box, 3, 3);

    // Icon + name
    if (m_zoom > 0.4f) {
        p.setPen(n.selected ? Qt::white : QColor(220,220,220));
        QFont f("monospace", qMax(8, (int)(9 * m_zoom)));
        p.setFont(f);
        QString label = nodeIcon(n.type) + " " + n.name;
        QFontMetrics fm(f);
        float tx = s.x() - fm.horizontalAdvance(label)*0.5f;
        float ty = s.y() - hh - 4;
        p.drawText(QPointF(tx, ty), label);
    }

    // Selection handles
    if (n.selected) {
        p.setPen(QPen(Qt::white, 1));
        p.setBrush(QColor(0, 120, 215));
        float hs = 5;
        for (int dx : {-1, 1}) for (int dy : {-1, 1})
            p.drawRect(QRectF(s.x()+dx*hw-hs, s.y()+dy*hh-hs, hs*2, hs*2));
    }
}

void Viewport::drawCameraRect(QPainter& p, const ViewportNode& cam) {
    // Camera shows what it sees — a rectangle centered on its position
    float vw = (cam.camW / cam.zoom) * m_zoom;
    float vh = (cam.camH / cam.zoom) * m_zoom;
    QPointF s = worldToScreen(cam.x, cam.y);
    QRectF  r(s.x()-vw*0.5f, s.y()-vh*0.5f, vw, vh);

    // Soft fill
    p.fillRect(r, QColor(52, 152, 219, 15));

    // Border — dashed blue
    QPen pen(QColor(52, 152, 219), 1.5, Qt::DashLine);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    p.drawRect(r);

    // Label
    p.setPen(QColor(52, 152, 219));
    p.setFont(QFont("monospace", 9));
    p.drawText(r.topLeft() + QPointF(4, -4), "📷 " + cam.name);
}

// ── Input ─────────────────────────────────────────────────────────────────
void Viewport::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::MiddleButton ||
        e->button() == Qt::RightButton ||
        (e->button() == Qt::LeftButton && e->modifiers() & Qt::AltModifier)) {
        m_panning   = true;
        m_dragStart = e->pos();
        setCursor(Qt::ClosedHandCursor);
        return;
    }

    if (e->button() == Qt::LeftButton) {
        ViewportNode* hit = nodeAt(e->pos());
        // Deselect all
        for (auto& n : m_nodes) n.selected = false;

        if (hit) {
            hit->selected      = true;
            m_dragging         = true;
            m_dragNode         = hit;
            m_dragStart        = e->pos();
            m_dragNodeOrigin   = { hit->x, hit->y };
            emit nodeSelected(hit->name);
            setCursor(Qt::SizeAllCursor);
        } else {
            emit nodeSelected("");
        }
        update();
    }
}

void Viewport::mouseMoveEvent(QMouseEvent* e) {
    if (m_panning) {
        QPointF delta = e->pos() - m_dragStart;
        m_panX += delta.x();
        m_panY += delta.y();
        m_dragStart = e->pos();
        update();
        return;
    }

    if (m_dragging && m_dragNode) {
        QPointF delta = e->pos() - m_dragStart;
        m_dragNode->x = m_dragNodeOrigin.x() + delta.x() / m_zoom;
        m_dragNode->y = m_dragNodeOrigin.y() + delta.y() / m_zoom;
        emit nodeMoved(m_dragNode->name, m_dragNode->x, m_dragNode->y);
        update();
    }
}

void Viewport::mouseReleaseEvent(QMouseEvent* e) {
    // Emit final position on release so editor saves to script
    if (m_dragging && m_dragNode) {
        emit nodeMovedFinal(m_dragNode->name, m_dragNode->x, m_dragNode->y);
    }
    m_dragging = false;
    m_panning  = false;
    m_dragNode = nullptr;
    setCursor(Qt::ArrowCursor);
}

void Viewport::wheelEvent(QWheelEvent* e) {
    float factor = e->angleDelta().y() > 0 ? 1.15f : 0.87f;
    float newZoom = qBound(0.05f, m_zoom * factor, 20.0f);

    // Zoom towards cursor
    QPointF c = e->position();
    m_panX = c.x() - (c.x() - m_panX) * (newZoom / m_zoom);
    m_panY = c.y() - (c.y() - m_panY) * (newZoom / m_zoom);
    m_zoom = newZoom;
    update();
}

void Viewport::resizeEvent(QResizeEvent*) {
    update();
}
