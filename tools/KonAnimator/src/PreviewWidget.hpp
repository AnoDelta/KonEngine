#pragma once
#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QTimer>
#include <QPointF>
#include <QLabel>
#include <QResizeEvent>
#include "AnimData.hpp"

class PreviewWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core {
    Q_OBJECT
public:
    explicit PreviewWidget(QWidget* parent = nullptr);
    ~PreviewWidget();

    void setClip(AnimClip* clip);
    void setSpritesheetPath(const QString& path);

    void play();
    void pause();
    void stop();
    void setPlayhead(float t);
    bool  isPlaying() const { return m_playing; }
    float elapsed()   const { return m_elapsed; }

    // Viewport border — set to 0×0 to hide
    void  setViewportSize(float w, float h) { m_vpW = w; m_vpH = h; update(); }
    float viewportW() const { return m_vpW; }
    float viewportH() const { return m_vpH; }

    void zoomIn()    { applyZoom(m_zoom * 1.25f, {width() * 0.5, height() * 0.5}); }
    void zoomOut()   { applyZoom(m_zoom * 0.8f,  {width() * 0.5, height() * 0.5}); }
    void resetView() { m_zoom = 1.0f; m_pan = {}; update(); }
    void toggleFullscreen();

signals:
    void frameChanged(int frameIdx);
    void elapsedChanged(float t);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void wheelEvent(QWheelEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    void resizeEvent(QResizeEvent*) override;

private slots:
    void onTick();

private:
    // Playback
    AnimClip* m_clip     = nullptr;
    bool      m_playing  = false;
    float     m_elapsed  = 0.0f;
    qint64    m_lastMs   = 0;
    int       m_curFrame = 0;
    QTimer*   m_timer    = nullptr;

    // GL
    GLuint m_texID  = 0;
    int    m_texW   = 0, m_texH = 0;
    GLuint m_vao    = 0, m_vbo  = 0;
    GLuint m_shader = 0;

    // Cached uniform locations
    GLint m_locProj      = -1;
    GLint m_locTint      = -1;
    GLint m_locTex       = -1;
    GLint m_locColorOnly = -1;   // 1 = solid color, 0 = textured

    // View state
    float   m_zoom      = 1.0f;
    QPointF m_pan       = {};
    bool    m_dragging  = false;
    QPointF m_dragStart = {};
    QPointF m_panAtDrag = {};

    // Fullscreen
    bool m_wasMaximized = false;

    // Viewport overlay (game screen border)
    float m_vpW = 0.0f;
    float m_vpH = 0.0f;

    // Helpers
    void   applyZoom(float newZoom, QPointF anchor);
    void   loadTexture(const QString& path);
    void   buildQuad();
    GLuint compileShader();
    void   drawFrame(int frameIdx);
    void   drawSolidRect(float x, float y, float w, float h,
                         float r, float g, float b, float a,
                         const float proj[16]);
    void   drawViewportBorder(const float proj[16]);
    int    currentFrameForTime(float t) const;

    QString m_sheetPath;
    QLabel* m_overlay   = nullptr;
    QLabel* m_zoomLabel = nullptr;
};
