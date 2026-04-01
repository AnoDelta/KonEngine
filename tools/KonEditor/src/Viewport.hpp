#pragma once
#include <QWidget>
#include <QPointF>
#include <QColor>
#include <QList>
#include <QPushButton>

struct ViewportNode {
    QString name;
    QString type;
    float   x = 0, y = 0;
    float   w = 32, h = 32;
    bool    selected = false;
    // camera props (if type == Camera2D / CameraNode2D)
    float   camW = 800, camH = 600;
    float   zoom = 1.0f;
};

class Viewport : public QWidget {
    Q_OBJECT
public:
    explicit Viewport(QWidget* parent = nullptr);

    void setNodes(const QList<ViewportNode>& nodes);
    void setGameResolution(int w, int h);
    void selectNode(const QString& name);
    void clearNodes();
    QList<ViewportNode> nodes() const { return m_nodes; }
    float nodeX(const QString& name) const { for (auto& n : m_nodes) if (n.name==name) return n.x; return 0; }
    float nodeY(const QString& name) const { for (auto& n : m_nodes) if (n.name==name) return n.y; return 0; }

    bool cameraPreview() const { return m_cameraPreview; }
    void setCameraPreview(bool on);

signals:
    void nodeSelected(const QString& name);
    void nodeMoved(const QString& name, float x, float y);
    void nodeMovedFinal(const QString& name, float x, float y);

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void resizeEvent(QResizeEvent*) override;

private:
    QPointF worldToScreen(float x, float y) const;
    QPointF screenToWorld(float x, float y) const;
    ViewportNode* nodeAt(QPointF screenPos);
    void drawGrid(QPainter& p);
    void drawNode(QPainter& p, ViewportNode& n);
    void drawCameraFrame(QPainter& p, const ViewportNode& cam);
    void drawOriginCross(QPainter& p);
    void drawGameBounds(QPainter& p);
    void updatePreviewButton();
    void syncCameraFromNodes();

    QList<ViewportNode> m_nodes;
    int    m_gameW = 800, m_gameH = 600;

    // Viewport pan/zoom (free navigation)
    float  m_panX  = 0, m_panY = 0;
    float  m_zoom  = 1.0f;

    // Camera preview state
    bool   m_cameraPreview = false;
    bool   m_hasCamera     = false;
    float  m_camX = 0, m_camY = 0, m_camZoom = 1.0f;
    QString m_camName;

    // Drag state
    bool   m_dragging    = false;
    bool   m_panning     = false;
    QPointF m_dragStart;
    QPointF m_dragNodeOrigin;
    ViewportNode* m_dragNode = nullptr;

    // Overlay toggle button (top-right corner)
    QPushButton* m_previewBtn = nullptr;

    static constexpr float GRID_STEP = 32.0f;
};
