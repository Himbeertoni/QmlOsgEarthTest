#pragma once
#include <QQuickFramebufferObject>

#include <osg/Switch>
#include <osgViewer/Viewer>
#include <osgViewer/View>

#include <osgEarth/Map>
#include <osgEarth/MapNode>
#include <osgEarthUtil/EarthManipulator>
#include <vector>
class QTimer;
class OsgEarthItem;

class OsgEarthItemRenderer : public QQuickFramebufferObject::Renderer
{
public:
    OsgEarthItemRenderer(OsgEarthItem const * item);
    virtual ~OsgEarthItemRenderer() override;

    virtual void render() override;
    virtual QOpenGLFramebufferObject * createFramebufferObject(const QSize & size) override;
    virtual void synchronize(QQuickFramebufferObject* item) override;

private:
    void initOsg();
    void initOsgEarth();

    OsgEarthItem const* m_item;
    osg::Image* m_iconImage;

    struct RenderView
    {
        int x, y, w, h;
        osg::Switch* m_root;
        osg::ref_ptr<osgViewer::Viewer> m_viewer;
        osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> m_window;
        osgEarth::Map* m_map;
        osgEarth::MapNode* m_mapNode;
        osgEarth::Util::EarthManipulator* m_manipulator;
    };
    RenderView m_renderViews[2];
    bool m_initialized;
    bool m_rendererInitialised;
    bool m_firstFrameOccurred;
};


namespace osgGA
{
    class EventQueue;
}

class OsgEarthItem : public QQuickFramebufferObject
{
    Q_OBJECT
public:
    explicit OsgEarthItem(QQuickItem *parent = nullptr);
    virtual ~OsgEarthItem() override;

    virtual Renderer * createRenderer() const override;

    osgGA::EventQueue* getEventQueue() { return m_eventQueue.get(); }

protected:
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;
    void geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override {}
    void wheelEvent(QWheelEvent* ) override;
    void hoverMoveEvent(QHoverEvent* event) override {}
private:
    osg::ref_ptr<osgGA::EventQueue> m_eventQueue;

    QTimer* m_updateTimer;
    bool m_continuousUpdate;

};
