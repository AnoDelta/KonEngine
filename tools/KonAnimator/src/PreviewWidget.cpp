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

// -----------------------------------------------------------------------
// Shaders — colorOnly=1 draws tint as solid color, 0 = textured+tint
// -----------------------------------------------------------------------
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
uniform vec4  tint;
uniform int   colorOnly;
void main() {
    if (colorOnly == 1)
        fragColor = tint;
    else
        fragColor = texture(tex, vUV) * tint;
}
)";

// -----------------------------------------------------------------------
// Easing — complete implementation matching KonEngine curves.hpp exactly
// -----------------------------------------------------------------------
static float applyEase(Ease e, float t) {
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
        return 1-applyEase(E::EaseOutBounce,1-t);
    case E::EaseInOutBounce:
        return t<0.5f
            ? (1-applyEase(E::EaseOutBounce,1-2*t))/2
            : (1+applyEase(E::EaseOutBounce,2*t-1))/2;

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

static float sampleKFTrack(const KFTrack& track, float time) {
    if (track.keys.empty())   return 0.0f;
    if (track.keys.size()==1) return track.keys[0].value;
    if (time <= track.keys.front().time) return track.keys.front().value;
    if (time >= track.keys.back().time)  return track.keys.back().value;
    for (size_t i = 0; i+1 < track.keys.size(); i++) {
        const Keyframe& a = track.keys[i];
        const Keyframe& b = track.keys[i+1];
        if (time >= a.time && time <= b.time) {
            float span = b.time - a.time;
            if (span == 0) return b.value;
            float t = (time - a.time) / span;
            return a.value + (b.value - a.value) * applyEase(a.curve, t);
        }
    }
    return track.keys.back().value;
}

// -----------------------------------------------------------------------
// Constructor / destructor
// -----------------------------------------------------------------------
PreviewWidget::PreviewWidget(QWidget* parent) : QOpenGLWidget(parent) {
    setMinimumSize(200, 150);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setFocusPolicy(Qt::StrongFocus);
    setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);

    m_overlay = new QLabel(
        "No animation loaded\n\nDrag → pan     Scroll → zoom\n"
        "Double-click → reset     F → fullscreen", this);
    m_overlay->setAlignment(Qt::AlignCenter);
    m_overlay->setStyleSheet("color: rgb(120,120,120); background: transparent;");
    m_overlay->setAttribute(Qt::WA_TransparentForMouseEvents);

    m_zoomLabel = new QLabel("100%", this);
    m_zoomLabel->setStyleSheet(
        "color: rgba(200,200,200,160); background: transparent; "
        "font-family: monospace; font-size: 8pt;");
    m_zoomLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_zoomLabel->setVisible(false);

    m_debugLabel = new QLabel(this);
    m_debugLabel->setStyleSheet(
        "color: rgba(180,220,255,200); background: rgba(0,0,0,120); "
        "font-family: monospace; font-size: 8pt; padding: 4px;");
    m_debugLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_debugLabel->setVisible(false);

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
    m_clip = clip; m_elapsed = 0; m_curFrame = 0; update();
}
void PreviewWidget::setSpritesheetPath(const QString& path) {
    m_sheetPath = path;
    makeCurrent(); loadTexture(path); doneCurrent(); update();
}
void PreviewWidget::play() {
    if (!m_clip) return;
    m_playing = true;
    m_lastMs  = QDateTime::currentMSecsSinceEpoch();
    m_timer->start();
}
void PreviewWidget::pause() { m_playing = false; m_timer->stop(); }
void PreviewWidget::stop() {
    m_playing = false; m_elapsed = 0; m_curFrame = 0;
    m_timer->stop(); update();
    emit elapsedChanged(0); emit frameChanged(0);
}
void PreviewWidget::setPlayhead(float t) {
    m_elapsed = t; m_curFrame = currentFrameForTime(t);
    update(); emit elapsedChanged(t); emit frameChanged(m_curFrame);
}

// -----------------------------------------------------------------------
// Fullscreen / zoom
// -----------------------------------------------------------------------
void PreviewWidget::toggleFullscreen() {
    QWindow* win = window()->windowHandle();
    if (!win) return;
    if (win->windowState() & Qt::WindowMaximized)
        m_wasMaximized ? win->showMaximized() : win->showNormal();
    else {
        m_wasMaximized = (win->windowState() & Qt::WindowMaximized) != 0;
        win->showMaximized();
    }
    setFocus();
}
void PreviewWidget::applyZoom(float nz, QPointF anchor) {
    nz = std::max(0.05f, std::min(nz, 64.0f));
    float ratio = nz / m_zoom;
    QPointF ctr(width()*.5, height()*.5);
    m_pan  = (anchor - ctr) * (1.0 - ratio) + m_pan * ratio;
    m_zoom = nz; update();
}

// -----------------------------------------------------------------------
// Input
// -----------------------------------------------------------------------
void PreviewWidget::wheelEvent(QWheelEvent* e) {
    float f = e->angleDelta().y() > 0 ? 1.25f : 0.8f;
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
    applyZoom(m_zoom * f, e->position());
#else
    applyZoom(m_zoom * f, e->posF());
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
    if (m_dragging) { m_pan = m_panAtDrag + QPointF(e->pos()) - m_dragStart; update(); }
}
void PreviewWidget::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) { m_dragging = false; setCursor(Qt::ArrowCursor); }
}
void PreviewWidget::mouseDoubleClickEvent(QMouseEvent*) { resetView(); }
void PreviewWidget::keyPressEvent(QKeyEvent* e) {
    switch (e->key()) {
    case Qt::Key_F: case Qt::Key_Escape: toggleFullscreen(); break;
    case Qt::Key_Plus: case Qt::Key_Equal: zoomIn(); break;
    case Qt::Key_Minus: zoomOut(); break;
    case Qt::Key_0: resetView(); break;
    default: QOpenGLWidget::keyPressEvent(e);
    }
}
void PreviewWidget::resizeEvent(QResizeEvent* e) {
    QOpenGLWidget::resizeEvent(e);
    m_overlay->setGeometry(rect());
    m_zoomLabel->adjustSize();
    m_zoomLabel->move(width() - m_zoomLabel->width() - 6, 4);
    if (m_debugLabel->isVisible()) {
        m_debugLabel->adjustSize();
        m_debugLabel->move(4, height() - m_debugLabel->height() - 4);
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

GLuint PreviewWidget::compileShader() {
    auto compile = [this](GLenum type, const char* src) {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        return s;
    };
    GLuint vs = compile(GL_VERTEX_SHADER,   kVertSrc);
    GLuint fs = compile(GL_FRAGMENT_SHADER, kFragSrc);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs); glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs); glDeleteShader(fs);

    // Cache uniform locations after linking
    m_locProj      = glGetUniformLocation(prog, "proj");
    m_locTint      = glGetUniformLocation(prog, "tint");
    m_locTex       = glGetUniformLocation(prog, "tex");
    m_locColorOnly = glGetUniformLocation(prog, "colorOnly");
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
// Tick
// -----------------------------------------------------------------------
void PreviewWidget::onTick() {
    if (!m_clip || !m_playing) return;
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    float  dt  = (now - m_lastMs) / 1000.0f;
    m_lastMs   = now;
    m_elapsed += dt;
    float dur = m_clip->totalDuration();
    if (dur > 0) {
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
    float tt  = (m_clip->loop && dur > 0) ? std::fmod(t, dur) : std::min(t, dur);
    float acc = 0;
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

    bool hasFrames  = m_clip && !m_clip->frames.empty() && m_texID;
    bool hasTracks  = m_clip && !m_clip->tracks.empty();
    bool hasContent = m_clip && m_shader && (hasFrames || hasTracks);

    if (hasContent) drawFrame(m_curFrame);

    m_overlay->setVisible(!hasContent);
    if (hasContent) {
        m_zoomLabel->setText(QString("%1%").arg(qRound(m_zoom * 100.0f)));
        m_zoomLabel->setVisible(true);
    } else {
        m_zoomLabel->setVisible(false);
    }

    // Show debug overlay with track values + active curve when playing
    if (m_playing && m_clip && !m_clip->tracks.empty()) {
        static const char* easeNames[] = {
            "Linear","EaseIn","EaseOut","EaseInOut",
            "EaseInCubic","EaseOutCubic","EaseInOutCubic",
            "EaseInElastic","EaseOutElastic","EaseInOutElastic",
            "EaseInBounce","EaseOutBounce","EaseInOutBounce",
            "EaseInBack","EaseOutBack","EaseInOutBack",
        };
        QString dbg;
        for (const auto& track : m_clip->tracks) {
            float v = sampleKFTrack(track, m_elapsed);
            // Find the active curve (from the keyframe just before current time)
            QString curveName = "—";
            for (int i = (int)track.keys.size()-1; i >= 0; --i) {
                if (track.keys[i].time <= m_elapsed) {
                    int ci = static_cast<int>(track.keys[i].curve);
                    if (ci >= 0 && ci < 16) curveName = easeNames[ci];
                    break;
                }
            }
            dbg += QString("%1: %2  [%3]\n")
                .arg(QString::fromStdString(track.name), -8)
                .arg(v, 0, 'f', 3)
                .arg(curveName);
        }
        m_debugLabel->setText(dbg.trimmed());
        m_debugLabel->adjustSize();
        m_debugLabel->move(4, height() - m_debugLabel->height() - 4);
        m_debugLabel->setVisible(true);
    } else {
        m_debugLabel->setVisible(false);
    }
}

void PreviewWidget::drawSolidRect(float x, float y, float w, float h,
                                   float r, float g, float b, float a,
                                   const float proj[16]) {
    glUniform1i(m_locColorOnly, 1);
    glUniform4f(m_locTint, r, g, b, a);

    float verts[] = {
        x,   y,   0,0,
        x+w, y,   1,0,
        x+w, y+h, 1,1,
        x,   y+h, 0,1,
    };
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glUniform1i(m_locColorOnly, 0);
}

void PreviewWidget::drawViewportBorder(const float proj[16]) {
    if (m_vpW <= 0 || m_vpH <= 0) return;
    int W = width(), H = height();
    float hw = m_vpW * 0.5f * m_zoom;
    float hh = m_vpH * 0.5f * m_zoom;
    float cx = W * 0.5f + (float)m_pan.x();
    float cy = H * 0.5f + (float)m_pan.y();

    // Dim the area outside the viewport
    float da = 0.35f;
    drawSolidRect(0,      0,      cx-hw, (float)H,  0,0,0,da,proj); // left
    drawSolidRect(cx+hw,  0,      (float)W-(cx+hw),(float)H, 0,0,0,da,proj); // right
    drawSolidRect(cx-hw,  0,      hw*2,  cy-hh,  0,0,0,da,proj); // top strip
    drawSolidRect(cx-hw,  cy+hh,  hw*2,  (float)H-(cy+hh), 0,0,0,da,proj); // bottom strip

    // Bright border
    float t = 1.5f;
    float br=0.35f, bg=0.75f, bb=1.0f, ba=0.9f;
    drawSolidRect(cx-hw-t, cy-hh-t, hw*2+t*2+t, t,     br,bg,bb,ba,proj);
    drawSolidRect(cx-hw-t, cy+hh,   hw*2+t*2+t, t,     br,bg,bb,ba,proj);
    drawSolidRect(cx-hw-t, cy-hh,   t,           hh*2,  br,bg,bb,ba,proj);
    drawSolidRect(cx+hw,   cy-hh,   t,           hh*2,  br,bg,bb,ba,proj);

    // Viewport size label
    // (drawn via QLabel overlay — skip GL text for simplicity)
}

void PreviewWidget::drawFrame(int frameIdx) {
    if (!m_clip) return;
    int W = width(), H = height();

    // Sample keyframe tracks
    float kx=0, ky=0, ksx=1, ksy=1, krot=0, alpha=1;
    for (const auto& track : m_clip->tracks) {
        float v = sampleKFTrack(track, m_elapsed);
        if      (track.name=="x")        kx    = v;
        else if (track.name=="y")        ky    = v;
        else if (track.name=="scaleX")   ksx   = v;
        else if (track.name=="scaleY")   ksy   = v;
        else if (track.name=="rotation") krot  = v;
        else if (track.name=="alpha")    alpha = v;
    }

    float baseW = (m_clip->displayW  > 0 ? m_clip->displayW  : 64.0f) * m_clip->displayScale * m_zoom;
    float baseH = (m_clip->displayH  > 0 ? m_clip->displayH  : 64.0f) * m_clip->displayScale * m_zoom;
    float dW = baseW * ksx;
    float dH = baseH * ksy;
    float cx = W * 0.5f + (float)m_pan.x() + kx * m_zoom;
    float cy = H * 0.5f + (float)m_pan.y() + ky * m_zoom;

    float u0=0, v0=0, u1=1, v1=1;
    bool hasTexture = m_texID && !m_clip->frames.empty() &&
                      frameIdx >= 0 && frameIdx < (int)m_clip->frames.size();
    if (hasTexture) {
        const auto& fr = m_clip->frames[frameIdx];
        u0 = fr.srcX             / (float)m_texW;
        v0 = fr.srcY             / (float)m_texH;
        u1 = (fr.srcX + fr.srcW) / (float)m_texW;
        v1 = (fr.srcY + fr.srcH) / (float)m_texH;
    }

    float hw = dW*.5f, hh = dH*.5f;
    float rad = krot * 3.14159265f / 180.0f;
    float cr = std::cos(rad), sr = std::sin(rad);
    auto rot = [&](float lx, float ly, float& ox, float& oy){
        ox = cx + lx*cr - ly*sr;
        oy = cy + lx*sr + ly*cr;
    };
    float x0,y0,x1,y1,x2,y2,x3,y3;
    rot(-hw,-hh,x0,y0); rot(+hw,-hh,x1,y1);
    rot(+hw,+hh,x2,y2); rot(-hw,+hh,x3,y3);

    float verts[] = {
        x0,y0, u0,v0,  x1,y1, u1,v0,
        x2,y2, u1,v1,  x3,y3, u0,v1,
    };

    float proj[16] = {
         2.0f/W,  0,      0, 0,
         0,      -2.0f/H, 0, 0,
         0,       0,      1, 0,
        -1.0f,    1.0f,   0, 1
    };

    glUseProgram(m_shader);
    glUniformMatrix4fv(m_locProj, 1, GL_FALSE, proj);
    glUniform1i(m_locTex, 0);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);

    if (hasTexture) {
        // Draw textured sprite
        glUniform1i(m_locColorOnly, 0);
        glUniform4f(m_locTint, 1,1,1, alpha);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_texID);
    } else {
        // No texture — draw a solid placeholder so track curves are still visible.
        // Checkerboard-ish teal box so it's obviously a placeholder not a real sprite.
        glUniform1i(m_locColorOnly, 1);
        glUniform4f(m_locTint, 0.25f, 0.65f, 0.85f, alpha);
    }
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    // Thin white border around placeholder so it's clear it's a box
    if (!hasTexture) {
        // draw outline using colorOnly solid lines (reuse same quad verts, just change draw mode)
        // Easiest: draw as line loop
        glUniform4f(m_locTint, 1.0f, 1.0f, 1.0f, alpha * 0.8f);
        glDrawArrays(GL_LINE_LOOP, 0, 4);
    }

    // Draw viewport border on top
    drawViewportBorder(proj);

    glBindVertexArray(0);
}
