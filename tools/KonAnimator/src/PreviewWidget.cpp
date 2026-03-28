#include "PreviewWidget.hpp"
#include <QDateTime>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QWindow>
#include <algorithm>
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static const char* kVertSrc = R"(
#version 330 core
layout(location=0) in vec2 pos;
layout(location=1) in vec2 uv;
uniform mat4 proj;
out vec2 vUV;
void main() {
    gl_Position = proj * vec4(pos, 0.0, 1.0);
    vUV = uv;
}
)";

static const char* kFragSrc = R"(
#version 330 core
in vec2 vUV;
out vec4 fragColor;
uniform sampler2D tex;
void main() {
    fragColor = texture(tex, vUV);
}
)";

// -----------------------------------------------------------------------
// Constructor / destructor
// -----------------------------------------------------------------------
PreviewWidget::PreviewWidget(QWidget* parent)
    : QOpenGLWidget(parent)
{
    setMinimumSize(200, 150);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setFocusPolicy(Qt::StrongFocus);
    setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);

    // Plain QWidget children — no QPainter involvement in paintGL
    m_overlay = new QLabel(
        "No animation loaded\n\nDrag → pan     Scroll → zoom\nDouble-click → reset     F → fullscreen",
        this);
    m_overlay->setAlignment(Qt::AlignCenter);
    m_overlay->setStyleSheet("color: rgb(120,120,120); background: transparent;");
    m_overlay->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_overlay->setVisible(true);

    m_zoomLabel = new QLabel("100%", this);
    m_zoomLabel->setStyleSheet("color: rgba(200,200,200,160); background: transparent; font-family: monospace; font-size: 8pt;");
    m_zoomLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_zoomLabel->setVisible(false);

    m_timer = new QTimer(this);
    m_timer->setInterval(16);
    connect(m_timer, &QTimer::timeout, this, &PreviewWidget::onTick);
}

PreviewWidget::~PreviewWidget() {
    makeCurrent();
    if (m_texID)  glDeleteTextures(1, &m_texID);
    if (m_vao)    glDeleteVertexArrays(1, &m_vao);
    if (m_vbo)    glDeleteBuffers(1, &m_vbo);
    if (m_shader) glDeleteProgram(m_shader);
    doneCurrent();
}

// -----------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------
void PreviewWidget::setClip(AnimClip* clip) {
    m_clip     = clip;
    m_elapsed  = 0.0f;
    m_curFrame = 0;
    update();
}

void PreviewWidget::setSpritesheetPath(const QString& path) {
    m_sheetPath = path;
    makeCurrent();
    loadTexture(path);
    doneCurrent();
    update();
}

void PreviewWidget::play() {
    if (!m_clip) return;
    m_playing = true;
    m_lastMs  = QDateTime::currentMSecsSinceEpoch();
    m_timer->start();
}

void PreviewWidget::pause() {
    m_playing = false;
    m_timer->stop();
}

void PreviewWidget::stop() {
    m_playing  = false;
    m_elapsed  = 0.0f;
    m_curFrame = 0;
    m_timer->stop();
    update();
    emit elapsedChanged(0.0f);
    emit frameChanged(0);
}

void PreviewWidget::setPlayhead(float t) {
    m_elapsed  = t;
    m_curFrame = currentFrameForTime(t);
    update();
    emit elapsedChanged(t);
    emit frameChanged(m_curFrame);
}

// -----------------------------------------------------------------------
// Fullscreen — maximise/restore the top-level window.
// Never reparents QOpenGLWidget (that would destroy the GL context).
// -----------------------------------------------------------------------
void PreviewWidget::toggleFullscreen() {
    QWindow* win = window()->windowHandle();
    if (!win) return;

    if (win->windowState() & Qt::WindowMaximized) {
        m_wasMaximized ? win->showMaximized() : win->showNormal();
    } else {
        m_wasMaximized = (win->windowState() & Qt::WindowMaximized) != 0;
        win->showMaximized();
    }
    setFocus();
}

// -----------------------------------------------------------------------
// Zoom — keeps the pixel under `anchor` (widget coords) stationary.
//
//   newPan = (anchor - centre) * (1 - ratio) + oldPan * ratio
// -----------------------------------------------------------------------
void PreviewWidget::applyZoom(float newZoom, QPointF anchor) {
    newZoom = std::max(0.05f, std::min(newZoom, 64.0f));
    float   ratio  = newZoom / m_zoom;
    QPointF centre(width() * 0.5, height() * 0.5);
    m_pan  = (anchor - centre) * (1.0 - ratio) + m_pan * ratio;
    m_zoom = newZoom;
    update();
}

// -----------------------------------------------------------------------
// Input events
// -----------------------------------------------------------------------
void PreviewWidget::wheelEvent(QWheelEvent* e) {
    float factor = e->angleDelta().y() > 0 ? 1.25f : 0.8f;
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    applyZoom(m_zoom * factor, e->position());
#else
    applyZoom(m_zoom * factor, e->posF());
#endif
}

void PreviewWidget::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        m_dragging  = true;
        m_dragStart = e->pos();
        m_panAtDrag = m_pan;
        setCursor(Qt::ClosedHandCursor);
    }
}

void PreviewWidget::mouseMoveEvent(QMouseEvent* e) {
    if (m_dragging) {
        m_pan = m_panAtDrag + QPointF(e->pos()) - m_dragStart;
        update();
    }
}

void PreviewWidget::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        m_dragging = false;
        setCursor(Qt::ArrowCursor);
    }
}

void PreviewWidget::mouseDoubleClickEvent(QMouseEvent*) {
    resetView();
}

void PreviewWidget::keyPressEvent(QKeyEvent* e) {
    switch (e->key()) {
    case Qt::Key_F:                        toggleFullscreen(); break;
    case Qt::Key_Escape:                   toggleFullscreen(); break;
    case Qt::Key_Plus: case Qt::Key_Equal: zoomIn();           break;
    case Qt::Key_Minus:                    zoomOut();          break;
    case Qt::Key_0:                        resetView();        break;
    default: QOpenGLWidget::keyPressEvent(e);
    }
}

// -----------------------------------------------------------------------
// GL setup
// -----------------------------------------------------------------------
void PreviewWidget::initializeGL() {
    initializeOpenGLFunctions();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_shader = compileShader();
    buildQuad();
}

void PreviewWidget::resizeGL(int, int) { update(); }

void PreviewWidget::resizeEvent(QResizeEvent* e) {
    QOpenGLWidget::resizeEvent(e);
    // Keep overlay centred, zoom label top-right
    m_overlay->setGeometry(rect());
    m_zoomLabel->adjustSize();
    m_zoomLabel->move(width() - m_zoomLabel->width() - 6, 4);
}

GLuint PreviewWidget::compileShader() {
    auto compile = [this](GLenum type, const char* src) -> GLuint {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        return s;
    };
    GLuint vs   = compile(GL_VERTEX_SHADER,   kVertSrc);
    GLuint fs   = compile(GL_FRAGMENT_SHADER, kFragSrc);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs); glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs); glDeleteShader(fs);
    return prog;
}

void PreviewWidget::buildQuad() {
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

void PreviewWidget::loadTexture(const QString& path) {
    if (m_texID) { glDeleteTextures(1, &m_texID); m_texID = 0; }
    stbi_set_flip_vertically_on_load(false);
    int ch;
    unsigned char* data = stbi_load(path.toStdString().c_str(), &m_texW, &m_texH, &ch, 0);
    if (!data) return;
    glGenTextures(1, &m_texID);
    glBindTexture(GL_TEXTURE_2D, m_texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    GLenum fmt = ch == 4 ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, fmt, m_texW, m_texH, 0, fmt, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
}

// -----------------------------------------------------------------------
// Tick / frame logic
// -----------------------------------------------------------------------
void PreviewWidget::onTick() {
    if (!m_clip || !m_playing) return;
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    float  dt  = (now - m_lastMs) / 1000.0f;
    m_lastMs   = now;

    m_elapsed += dt;
    float dur = m_clip->totalDuration();
    if (dur > 0.0f) {
        if (m_clip->loop) m_elapsed = std::fmod(m_elapsed, dur);
        else              m_elapsed = std::min(m_elapsed, dur);
    }
    int f = currentFrameForTime(m_elapsed);
    if (f != m_curFrame) { m_curFrame = f; emit frameChanged(f); }
    emit elapsedChanged(m_elapsed);
    update();
}

int PreviewWidget::currentFrameForTime(float t) const {
    if (!m_clip || m_clip->frames.empty()) return 0;
    float dur = m_clip->totalDuration();
    float tt  = (m_clip->loop && dur > 0.0f) ? std::fmod(t, dur) : std::min(t, dur);
    float acc = 0.0f;
    for (int i = 0; i < (int)m_clip->frames.size(); i++) {
        acc += m_clip->frames[i].duration;
        if (tt < acc) return i;
    }
    return (int)m_clip->frames.size() - 1;
}

// -----------------------------------------------------------------------
// Render
// -----------------------------------------------------------------------
void PreviewWidget::paintGL() {
    int W = width(), H = height();
    glViewport(0, 0, W, H);
    glClearColor(0.18f, 0.18f, 0.18f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    bool hasContent = m_clip && !m_clip->frames.empty() && m_texID && m_shader;
    if (hasContent)
        drawFrame(m_curFrame);

    // No QPainter here at all — it calls beginNativePainting() which
    // recomposites the old framebuffer on Wayland, causing the trail.
    // Text overlay is handled by m_overlay (a plain QLabel child widget).
    m_overlay->setVisible(!hasContent);
    if (hasContent) {
        m_zoomLabel->setText(QString("%1%").arg(qRound(m_zoom * 100.0f)));
        m_zoomLabel->setVisible(true);
    } else {
        m_zoomLabel->setVisible(false);
    }
}

void PreviewWidget::drawFrame(int frameIdx) {
    if (!m_clip || frameIdx < 0 || frameIdx >= (int)m_clip->frames.size()) return;
    const auto& fr = m_clip->frames[frameIdx];

    int W = width(), H = height();

    // Sprite size in screen pixels = display size x clip scale x user zoom
    float dW = m_clip->displayW * m_clip->displayScale * m_zoom;
    float dH = m_clip->displayH * m_clip->displayScale * m_zoom;

    // Top-left corner: widget centre + pan offset - half sprite size
    float x = W * 0.5f + (float)m_pan.x() - dW * 0.5f;
    float y = H * 0.5f + (float)m_pan.y() - dH * 0.5f;

    // Orthographic projection: top-left origin, y pointing down
    float proj[16] = {
         2.0f/W,  0,      0, 0,
         0,      -2.0f/H, 0, 0,
         0,       0,      1, 0,
        -1.0f,    1.0f,   0, 1
    };

    float u0 = fr.srcX             / (float)m_texW;
    float v0 = fr.srcY             / (float)m_texH;
    float u1 = (fr.srcX + fr.srcW) / (float)m_texW;
    float v1 = (fr.srcY + fr.srcH) / (float)m_texH;

    float verts[] = {
        x,    y,    u0, v0,
        x+dW, y,    u1, v0,
        x+dW, y+dH, u1, v1,
        x,    y+dH, u0, v1,
    };

    glUseProgram(m_shader);
    glUniformMatrix4fv(glGetUniformLocation(m_shader, "proj"), 1, GL_FALSE, proj);
    glUniform1i(glGetUniformLocation(m_shader, "tex"), 0);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texID);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
}
