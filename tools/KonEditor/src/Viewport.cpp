#include "Viewport.hpp"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPen>
#include <QFont>
#include <QFontMetrics>
#include <QVBoxLayout>
#include <cmath>

// Node type → display color
static QColor nodeColor(const QString& type) {
    if (type == "RigidBody2D")        return {230, 126,  34};
    if (type == "StaticBody2D")       return {149, 165, 166};
    if (type == "KinematicBody2D")    return { 52, 152, 219};
    if (type == "Area2D")             return {155,  89, 182};
    if (type == "CollisionShape2D")   return {231,  76,  60};
    if (type == "Sprite2D")           return { 46, 204, 113};
    if (type == "AnimatedSprite2D")   return { 26, 188, 156};
    if (type == "Camera2D" || type == "CameraNode2D") return { 52, 152, 219};
    if (type == "Label")              return {241, 196,  15};
    if (type == "Node2D")             return {127, 140, 141};
    if (type == "Timer")              return {189, 195, 199};
    if (type == "AudioPlayer")        return {230, 126,  34};
    return {100, 100, 100};
}

static QString nodeIcon(const QString& type) {
    if (type == "RigidBody2D")        return "⚡";
    if (type == "StaticBody2D")       return "🧱";
    if (type == "KinematicBody2D")    return "🏃";
    if (type == "Area2D")             return "○";
    if (type == "CollisionShape2D")   return "⬜";
    if (type == "Sprite2D")           return "🖼";
    if (type == "Camera2D" || type == "CameraNode2D") return "📷";
    if (type == "Label")              return "T";
    if (type == "Timer")              return "⏱";
    return "◆";
}

Viewport::Viewport(QWidget* parent) : QWidget(parent) {
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(400, 300);

    // Camera preview toggle button — overlaid top-right
    m_previewBtn = new QPushButton("📷  Scene", this);
    m_previewBtn->setFixedHeight(26);
    m_previewBtn->setCheckable(true);
    m_previewBtn->setChecked(false);
    m_previewBtn->setToolTip("Toggle camera preview (shows what the camera sees)");
    m_previewBtn->setStyleSheet(
        "QPushButton {"
        "  background: rgba(30,30,30,200); color: #aaa;"
        "  border: 1px solid #444; border-radius: 4px;"
        "  font-size: 11px; padding: 0 10px;"
        "}"
        "QPushButton:hover { background: rgba(50,50,50,220); color: #ddd; }"
        "QPushButton:checked {"
        "  background: rgba(0,100,180,200); color: #fff;"
        "  border-color: #0078d7;"
        "}"
        "QPushButton:checked:hover { background: rgba(0,120,215,220); }");

    connect(m_previewBtn, &QPushButton::toggled, [this](bool on) {
        setCameraPreview(on);
    });
}

void Viewport::setCameraPreview(bool on) {
    m_cameraPreview = on;
    m_previewBtn->setChecked(on);
    m_previewBtn->setText(on ? "📷  Camera" : "📷  Scene");
    // Reset manual pan when entering preview so camera snaps to center
    if (on) { m_panX = 0; m_panY = 0; }
    update();
}

void Viewport::syncCameraFromNodes() {
    m_hasCamera = false;
    for (auto& n : m_nodes) {
        if (n.type == "Camera2D" || n.type == "CameraNode2D") {
            m_camX    = n.x;
            m_camY    = n.y;
            m_camZoom = (n.zoom > 0.001f) ? n.zoom : 1.0f;
            m_camName = n.name;
            m_hasCamera = true;
            break;
        }
    }
    updatePreviewButton();
}

void Viewport::updatePreviewButton() {
    // Grey out the button when no camera exists in the scene
    if (!m_hasCamera) {
        m_previewBtn->setEnabled(false);
        m_previewBtn->setToolTip("No camera in scene — add a CameraNode2D first");
        if (m_cameraPreview) setCameraPreview(false);
    } else {
        m_previewBtn->setEnabled(true);
        m_previewBtn->setToolTip(
            QString("Camera preview: %1\nShows exactly what the camera sees").arg(m_camName));
    }
}

void Viewport::setNodes(const QList<ViewportNode>& nodes) {
    // Invalidate drag pointer — m_dragNode pointed into the old list
    if (m_dragging) {
        m_dragging = false;
        m_dragNode = nullptr;
        setCursor(Qt::ArrowCursor);
    }
    m_nodes = nodes;
    syncCameraFromNodes();
    update();
}

// Update only x/y of existing nodes — does not replace the list or reset drag state.
// Used when a parent moves and we need children to follow without interrupting drag.
void Viewport::updateNodePositions(const QList<ViewportNode>& updated) {
    for (auto& u : updated) {
        for (auto& n : m_nodes) {
            if (n.name == u.name) { n.x = u.x; n.y = u.y; break; }
        }
    }
    update();
}

void Viewport::clearNodes() {
    m_nodes.clear();
    m_hasCamera = false;
    updatePreviewButton();
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

    if (m_cameraPreview && m_hasCamera) {
        // Camera preview: world is rendered relative to camera position.
        // The camera sits at the screen center; everything is offset by -camera_pos
        // and scaled by the camera's zoom, then by the editor's manual zoom.
        float ez = m_zoom * m_camZoom;
        return { cx + (x - m_camX) * ez, cy + (y - m_camY) * ez };
    }

    return { cx + x * m_zoom, cy + y * m_zoom };
}

QPointF Viewport::screenToWorld(float x, float y) const {
    float cx = width()  * 0.5f + m_panX;
    float cy = height() * 0.5f + m_panY;

    if (m_cameraPreview && m_hasCamera) {
        float ez = m_zoom * m_camZoom;
        if (std::fabs(ez) < 0.0001f) ez = 0.0001f;
        return { (x - cx) / ez + m_camX, (y - cy) / ez + m_camY };
    }

    float z = (std::fabs(m_zoom) < 0.0001f) ? 0.0001f : m_zoom;
    return { (x - cx) / z, (y - cy) / z };
}

// ── Hit testing ───────────────────────────────────────────────────────────
ViewportNode* Viewport::nodeAt(QPointF sp) {
    for (int i = m_nodes.size()-1; i >= 0; i--) {
        auto& n = m_nodes[i];
        QPointF s = worldToScreen(n.x, n.y);
        // Give cameras a larger hit area so they're easier to select
        float hw, hh;
        if (n.type == "Camera2D" || n.type == "CameraNode2D") {
            hw = 24.0f; hh = 24.0f; // fixed screen-space size, not zoom-scaled
        } else {
            hw = (n.w * 0.5f) * m_zoom;
            hh = (n.h * 0.5f) * m_zoom;
        }
        if (sp.x() >= s.x()-hw && sp.x() <= s.x()+hw &&
            sp.y() >= s.y()-hh && sp.y() <= s.y()+hh)
            return &n;
    }
    return nullptr;
}

// ── Game bounds rectangle ─────────────────────────────────────────────────
void Viewport::drawGameBounds(QPainter& p) {
    if (m_cameraPreview && m_hasCamera) {
        // In camera preview the game frame is always centered on screen,
        // sized to exactly the camera's view (gameRes / camZoom) * editorZoom.
        float cz = (std::fabs(m_camZoom) < 0.0001f) ? 0.0001f : m_camZoom;
        float vw = (m_gameW / cz) * m_zoom;
        float vh = (m_gameH / cz) * m_zoom;
        float cx = width()  * 0.5f + m_panX;
        float cy = height() * 0.5f + m_panY;
        QRectF frame(cx - vw * 0.5f, cy - vh * 0.5f, vw, vh);

        // Dim everything outside the camera frame
        QColor dim(0, 0, 0, 120);
        p.fillRect(QRectF(0, 0, frame.left(), height()), dim);
        p.fillRect(QRectF(frame.right(), 0, width() - frame.right(), height()), dim);
        p.fillRect(QRectF(frame.left(), 0, frame.width(), frame.top()), dim);
        p.fillRect(QRectF(frame.left(), frame.bottom(), frame.width(), height() - frame.bottom()), dim);

        // Solid camera frame border
        p.setPen(QPen(QColor(0, 120, 215), 2));
        p.setBrush(Qt::NoBrush);
        p.drawRect(frame);

        // "CAMERA" label top-left of frame
        p.setFont(QFont("monospace", 9));
        p.setPen(QColor(0, 150, 255));
        p.drawText(frame.topLeft() + QPointF(4, -5),
                   QString("📷 %1").arg(m_camName));
    } else {
        // Scene view: show world-space game bounds (0,0) → (gameW, gameH)
        QPointF tl = worldToScreen(0,       0);
        QPointF br = worldToScreen(m_gameW, m_gameH);
        QRectF  bounds(tl, br);

        // Dim outside
        QColor dim(0, 0, 0, 70);
        p.fillRect(QRectF(0, 0, tl.x(), height()), dim);
        p.fillRect(QRectF(br.x(), 0, width()-br.x(), height()), dim);
        p.fillRect(QRectF(tl.x(), 0, br.x()-tl.x(), tl.y()), dim);
        p.fillRect(QRectF(tl.x(), br.y(), br.x()-tl.x(), height()-br.y()), dim);

        p.setPen(QPen(QColor(60, 60, 70), 1, Qt::DashLine));
        p.setBrush(Qt::NoBrush);
        p.drawRect(bounds);

        // Resolution label
        p.setFont(QFont("monospace", 9));
        p.setPen(QColor(60, 60, 80));
        p.drawText(tl + QPointF(4, -5),
                   QString("%1 × %2").arg(m_gameW).arg(m_gameH));
    }
}

// ── Paint ─────────────────────────────────────────────────────────────────
void Viewport::paintEvent(QPaintEvent*) {
    static int paintCount = 0;
    if (++paintCount <= 5 || paintCount % 100 == 0)
        fprintf(stderr, "[Viewport] paintEvent #%d nodes=%d\n", paintCount, m_nodes.size());
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Background
    p.fillRect(rect(), QColor(18, 18, 18));

    // Grid
    drawGrid(p);
    if (paintCount <= 6) fprintf(stderr, "[Viewport] paint: grid done\n");

    // Origin cross (world 0,0)
    drawOriginCross(p);
    if (paintCount <= 6) fprintf(stderr, "[Viewport] paint: origin done\n");

    // Game / camera bounds + dim
    drawGameBounds(p);
    if (paintCount <= 6) fprintf(stderr, "[Viewport] paint: bounds done\n");

    // In scene view: draw camera frustum rects behind nodes
    if (!m_cameraPreview) {
        for (auto& n : m_nodes)
            if (n.type == "Camera2D" || n.type == "CameraNode2D")
                drawCameraFrame(p, n);
    }
    if (paintCount <= 6) fprintf(stderr, "[Viewport] paint: cameraFrames done\n");

    // Draw all non-camera nodes
    for (int ni = 0; ni < m_nodes.size(); ni++) {
        auto& n = m_nodes[ni];
        if (n.type != "Camera2D" && n.type != "CameraNode2D") {
            if (paintCount <= 6) fprintf(stderr, "[Viewport] paint: drawNode[%d] '%s' type='%s' sel=%d\n",
                    ni, n.name.toUtf8().constData(), n.type.toUtf8().constData(), n.selected);
            drawNode(p, n);
        }
    }
    if (paintCount <= 6) fprintf(stderr, "[Viewport] paint: nodes done\n");

    // Camera nodes on top (as icons) in scene view
    if (!m_cameraPreview) {
        for (auto& n : m_nodes)
            if (n.type == "Camera2D" || n.type == "CameraNode2D")
                drawNode(p, n);
    }
    if (paintCount <= 6) fprintf(stderr, "[Viewport] paint: cam icons done\n");

    // Info overlay
    p.setPen(QColor(70, 70, 70));
    p.setFont(QFont("monospace", 9));
    QString info;
    if (m_cameraPreview && m_hasCamera) {
        info = QString("camera: (%1, %2)  zoom: %3x  [preview]")
            .arg((int)m_camX).arg((int)m_camY)
            .arg(m_camZoom, 0, 'f', 2);
    } else {
        float iz = (std::fabs(m_zoom) < 0.0001f) ? 0.0001f : m_zoom;
        info = QString("zoom: %1x  pan: (%2, %3)  nodes: %4")
            .arg(m_zoom, 0, 'f', 2)
            .arg((int)(-m_panX / iz))
            .arg((int)(-m_panY / iz))
            .arg(m_nodes.size());
    }
    p.drawText(8, height() - 8, info);
    if (paintCount <= 6) fprintf(stderr, "[Viewport] paint: info overlay done\n");
    if (paintCount <= 6) fprintf(stderr, "[Viewport] paintEvent #%d COMPLETE\n", paintCount);
}

void Viewport::drawGrid(QPainter& p) {
    float step = GRID_STEP * m_zoom;
    if (step < 6) return;

    float cx = width()  * 0.5f + m_panX;
    float cy = height() * 0.5f + m_panY;

    // In camera preview, offset grid by camera pos so it stays world-aligned
    if (m_cameraPreview && m_hasCamera) {
        float ez = m_zoom * m_camZoom;
        step = GRID_STEP * ez;
        if (step < 6) return;
        cx = width()  * 0.5f + m_panX - m_camX * ez;
        cy = height() * 0.5f + m_panY - m_camY * ez;
    }

    p.setPen(QPen(QColor(28, 28, 28), 1));

    float startX = std::fmod(cx, step);
    if (startX < 0) startX += step;
    for (float x = startX; x < width(); x += step)
        p.drawLine(QPointF(x, 0), QPointF(x, height()));

    float startY = std::fmod(cy, step);
    if (startY < 0) startY += step;
    for (float y = startY; y < height(); y += step)
        p.drawLine(QPointF(0, y), QPointF(width(), y));
}

void Viewport::drawOriginCross(QPainter& p) {
    QPointF o = worldToScreen(0, 0);
    p.setPen(QPen(QColor(50, 50, 70), 1));
    p.drawLine(QPointF(o.x(), 0), QPointF(o.x(), height()));
    p.drawLine(QPointF(0, o.y()), QPointF(width(), o.y()));
    p.setPen(QPen(QColor(90, 90, 130), 2));
    p.drawEllipse(o, 3, 3);
}

void Viewport::drawNode(QPainter& p, ViewportNode& n) {
    QPointF s  = worldToScreen(n.x, n.y);
    float   hw = (n.w * 0.5f) * m_zoom;
    float   hh = (n.h * 0.5f) * m_zoom;
    QColor  col = nodeColor(n.type);

    QRectF box(s.x()-hw, s.y()-hh, hw*2, hh*2);
    p.setPen(Qt::NoPen);
    QColor fill = col;
    fill.setAlpha(n.selected ? 180 : 80);
    p.setBrush(fill);
    p.drawRoundedRect(box, 3, 3);

    QPen border(n.selected ? QColor(255,255,255) : col, n.selected ? 2 : 1);
    p.setPen(border);
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(box, 3, 3);

    if (m_zoom > 0.4f) {
        p.setPen(n.selected ? Qt::white : QColor(220,220,220));
        QFont f("monospace", qMax(8, (int)(9 * m_zoom)));
        p.setFont(f);
        QString label = nodeIcon(n.type) + " " + n.name;
        QFontMetrics fm(f);
        float tx = s.x() - fm.horizontalAdvance(label) * 0.5f;
        float ty = s.y() - hh - 4;
        p.drawText(QPointF(tx, ty), label);
    }

    if (n.selected) {
        p.setPen(QPen(Qt::white, 1));
        p.setBrush(QColor(0, 120, 215));
        float hs = 5;
        for (int dx : {-1, 1}) for (int dy : {-1, 1})
            p.drawRect(QRectF(s.x()+dx*hw-hs, s.y()+dy*hh-hs, hs*2, hs*2));
    }
}

void Viewport::drawCameraFrame(QPainter& p, const ViewportNode& cam) {
    // In scene view: show what this camera would see as a dashed rect
    float camZ = (cam.zoom > 0.001f) ? cam.zoom : 1.0f;
    float vw   = (cam.camW / camZ) * m_zoom;
    float vh   = (cam.camH / camZ) * m_zoom;
    QPointF s  = worldToScreen(cam.x, cam.y);
    QRectF  r(s.x() - vw*0.5f, s.y() - vh*0.5f, vw, vh);

    p.fillRect(r, QColor(52, 152, 219, 12));

    QPen pen(QColor(52, 152, 219), 1.5f, Qt::DashLine);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    p.drawRect(r);

    p.setPen(QColor(52, 152, 219));
    p.setFont(QFont("monospace", 9));
    p.drawText(r.topLeft() + QPointF(4, -4), "📷 " + cam.name);
}

// ── Input ─────────────────────────────────────────────────────────────────
void Viewport::mousePressEvent(QMouseEvent* e) {
    fprintf(stderr, "[Viewport] mousePress btn=%d nodes=%d\n", (int)e->button(), m_nodes.size());
    if (e->button() == Qt::MiddleButton ||
        e->button() == Qt::RightButton  ||
        (e->button() == Qt::LeftButton && e->modifiers() & Qt::AltModifier)) {
        m_panning   = true;
        m_dragStart = e->pos();
        setCursor(Qt::ClosedHandCursor);
        return;
    }

    if (e->button() == Qt::LeftButton) {
        ViewportNode* hit = nodeAt(e->pos());
        for (auto& n : m_nodes) n.selected = false;

        if (hit) {
            hit->selected    = true;
            m_dragging       = true;
            m_dragNode       = hit;
            m_dragStart      = e->pos();
            m_dragNodeOrigin = { hit->x, hit->y };
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
        fprintf(stderr, "[Viewport] mouseMove drag: %s dragging=%d dragNode=%p\n",
                m_dragNode->name.toUtf8().constData(), m_dragging, (void*)m_dragNode);
        QPointF delta = e->pos() - m_dragStart;
        float scale = (m_cameraPreview && m_hasCamera)
            ? m_zoom * m_camZoom
            : m_zoom;
        if (std::fabs(scale) < 0.0001f) scale = 0.0001f;
        m_dragNode->x = m_dragNodeOrigin.x() + delta.x() / scale;
        m_dragNode->y = m_dragNodeOrigin.y() + delta.y() / scale;
        // Cache values before emit — handler runs synchronously and could
        // invalidate m_dragNode via setNodes()
        QString dragName = m_dragNode->name;
        QString dragType = m_dragNode->type;
        float   dragX    = m_dragNode->x;
        float   dragY    = m_dragNode->y;
        emit nodeMoved(dragName, dragX, dragY);
        fprintf(stderr, "[Viewport] mouseMove: after emit, dragging=%d dragNode=%p\n",
                m_dragging, (void*)m_dragNode);
        // After emit, m_dragNode may be invalid if setNodes() was called
        if (!m_dragging || !m_dragNode) { fprintf(stderr, "[Viewport] mouseMove: drag invalidated, return\n"); return; }
        // Keep camera state in sync when moving camera node
        if (dragType == "Camera2D" || dragType == "CameraNode2D") {
            m_camX = dragX;
            m_camY = dragY;
        }
        fprintf(stderr, "[Viewport] mouseMove: calling update()\n");
        update();
        fprintf(stderr, "[Viewport] mouseMove: done\n");
    }
}

void Viewport::mouseReleaseEvent(QMouseEvent*) {
    fprintf(stderr, "[Viewport] mouseRelease dragging=%d dragNode=%p\n", m_dragging, (void*)m_dragNode);
    if (m_dragging && m_dragNode)
        emit nodeMovedFinal(m_dragNode->name, m_dragNode->x, m_dragNode->y);
    m_dragging = false;
    m_panning  = false;
    m_dragNode = nullptr;
    setCursor(Qt::ArrowCursor);
}

void Viewport::wheelEvent(QWheelEvent* e) {
    float factor  = e->angleDelta().y() > 0 ? 1.15f : 0.87f;
    float newZoom = qBound(0.05f, m_zoom * factor, 20.0f);

    // Zoom towards cursor
    QPointF c = e->position();
    float wz = (std::fabs(m_zoom) < 0.0001f) ? 0.0001f : m_zoom;
    m_panX = c.x() - (c.x() - m_panX) * (newZoom / wz);
    m_panY = c.y() - (c.y() - m_panY) * (newZoom / wz);
    m_zoom = newZoom;
    update();
}

void Viewport::resizeEvent(QResizeEvent*) {
    // Keep preview button anchored to top-right
    if (m_previewBtn) {
        m_previewBtn->adjustSize();
        m_previewBtn->move(width() - m_previewBtn->width() - 8, 8);
    }
    update();
}
