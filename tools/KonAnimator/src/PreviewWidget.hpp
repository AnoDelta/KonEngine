#pragma once
#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QTimer>
#include <QPointF>
#include <QLabel>
#include <QResizeEvent>
#include "AnimData.hpp"

// Live animation preview.
//
// Controls:
//   Scroll wheel        zoom centred on cursor
//   Left-click drag     pan
//   Double-click        reset zoom & pan
//   F                   fullscreen (maximises top-level window; F or Esc restores)
//   +  /  -  /  0       zoom in / out / reset

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
    bool isPlaying() const { return m_playing; }
    float elapsed()  const { return m_elapsed; }

    // Toolbar-callable zoom
    void zoomIn()    { applyZoom(m_zoom * 1.25f, {width() * 0.5, height() * 0.5}); }
    void zoomOut()   { applyZoom(m_zoom * 0.8f,  {width() * 0.5, height() * 0.5}); }
    void resetView() { m_zoom = 1.0f; m_pan = {}; update(); }

    // Fullscreen: maximises/restores the top-level QWindow that contains us.
    // Safe — never reparents the QOpenGLWidget.
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
    // playback
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

    // view state
    float   m_zoom      = 1.0f;
    QPointF m_pan       = {};   // sprite-centre offset from widget-centre, widget pixels
    bool    m_dragging  = false;
    QPointF m_dragStart = {};   // cursor pos when drag began
    QPointF m_panAtDrag = {};   // m_pan value when drag began

    // fullscreen
    bool m_wasMaximized = false;

    // helpers
    void   applyZoom(float newZoom, QPointF anchor);
    void   loadTexture(const QString& path);
    void   buildQuad();
    GLuint compileShader();
    void   drawFrame(int frameIdx);
    int    currentFrameForTime(float t) const;

    QString m_sheetPath;

    // Plain child widgets for text overlay (no QPainter in paintGL)
    QLabel* m_overlay   = nullptr;
    QLabel* m_zoomLabel = nullptr;
};
