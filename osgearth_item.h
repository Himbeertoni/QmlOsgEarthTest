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

    void initialize();
public:
    virtual void render() override;
    virtual QOpenGLFramebufferObject * createFramebufferObject(const QSize & size) override;
    virtual void synchronize(QQuickFramebufferObject* item) override;



protected:
    friend class QtFboOsgWin;

    OsgEarthItem const* m_item;
    int m_itemWidth;
    int m_itemHeight;
    int m_counter;
    bool m_initialized;
    osg::Switch* m_root;
    osg::ref_ptr<osgViewer::Viewer>  m_viewer;
    osg::ref_ptr<osgViewer::View>             m_view;
    osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> m_window;
    osg::ref_ptr<osg::Switch>                 m_maps;
    QOpenGLFramebufferObject * m_framebufferObject;
    GLuint m_fbo, m_fbo2;
    GLuint texf;
    bool m_rendererInitialised;
    bool m_firstFrameOccurred;
    osgEarth::Map* m_map;
    osgEarth::MapNode* m_mapNode;
    osgEarth::Util::EarthManipulator* m_manipulator;
    osg::Image* m_iconImage;
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

    int getCounter() { return m_counter; }

    Q_INVOKABLE void onQmlKeyPressed(int key, int modifiers, QString const& text);

    void setContinuousUpdate(bool enable);

    struct RandomPlaceEvent
    {
        double lat, lon;
    };
    void createRandomPlaceNode();
    std::vector<RandomPlaceEvent> m_randEventQueue;
protected:
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;
    void geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent* ) override;
    void hoverMoveEvent(QHoverEvent* event) override;
private:
    osg::ref_ptr<osgGA::EventQueue> m_eventQueue;

    QTimer* m_updateTimer;
    int m_counter;
    int m_lastMouseX;
    int m_lastMouseY;
    bool m_continuousUpdate;

};
