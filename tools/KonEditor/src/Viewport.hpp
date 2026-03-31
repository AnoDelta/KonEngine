#pragma once
#include <QWidget>
#include <QPointF>
#include <QColor>
#include <QList>

struct ViewportNode {
    QString name;
    QString type;
    float   x = 0, y = 0;
    float   w = 32, h = 32;
    bool    selected = false;
    bool    hasCamera = false;
    // camera props (if type == Camera2D)
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
    void drawCameraRect(QPainter& p, const ViewportNode& cam);
    void drawOriginCross(QPainter& p);

    QList<ViewportNode> m_nodes;
    int    m_gameW = 800, m_gameH = 600;

    // Viewport pan/zoom
    float  m_panX  = 0, m_panY = 0;
    float  m_zoom  = 1.0f;

    // Drag state
    bool   m_dragging    = false;
    bool   m_panning     = false;
    QPointF m_dragStart;
    QPointF m_dragNodeOrigin;
    ViewportNode* m_dragNode = nullptr;

    static constexpr float GRID_STEP = 32.0f;
};
