#include "osgearth_item.h"

#include <QOpenGLFramebufferObjectFormat>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/TrackballManipulator>
#include <osg/MatrixTransform>
#include <osgEarth/GLUtils>
#include <QtGui/QOpenGLFramebufferObject>

#include <QtQuick/QQuickWindow>
#include <qsgsimpletexturenode.h>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLExtraFunctions>
#include <assert.h>

#include <QTimer>
#include <osgViewer/GraphicsWindow>
#include <QDebug>

#include <osgEarth/Map>
#include <osgEarth/MapNode>
#include <osgEarthDrivers/engine_rex/RexTerrainEngineOptions>
#include <osgEarthDrivers/tms/TMSOptions>
#include <osgEarth/ImageLayer>
#include <osgEarthAnnotation/PlaceNode>

QQuickFramebufferObject::Renderer* OsgEarthItem::createRenderer() const
{
    return new OsgEarthItemRenderer(this);
}

//NOTE(alex): QQuickFBO and QQuickFBO::Renderer classes are separated since they (most likely) run in different threads!
//Only in Renderer::synchronize() the renderer may access shared members with the FBO.

class QtFboOsgWin : public osgViewer::GraphicsWindowEmbedded
{
public:
    explicit QtFboOsgWin(OsgEarthItemRenderer const* renderer)
        : osgViewer::GraphicsWindowEmbedded(0, 0, 0, 0)
        , m_renderer(renderer)
    {
    }
    virtual bool makeCurrentImplementation() override
    {
        return m_renderer->framebufferObject()->bind();
    }

    virtual bool releaseContextImplementation() override
    {
        return m_renderer->framebufferObject()->release();
    }
private:
    OsgEarthItemRenderer const* m_renderer;
};

static const double FOV_FACTOR =  1.0 / 20.0; //Magic value!

void OsgEarthItemRenderer::initialize()
{
    osg::setNotifyLevel(osg::NotifySeverity::WARN);

    m_window = new QtFboOsgWin(this);

    m_viewer = new osgViewer::Viewer();
    m_viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    m_viewer->setKeyEventSetsDone(0); // Disable the default setting of viewer.done() by pressing Escape.

    m_window = m_viewer->setUpViewerAsEmbeddedInWindow(0, 0, m_item->width(), m_item->height());
    m_view = m_viewer;

    m_view->addEventHandler(new osgViewer::StatsHandler);

    osg::Camera* camera = m_view->getCamera();
    camera->setGraphicsContext(m_window.get());

    //camera->setDrawBuffer(GL_COLOR_ATTACHMENT0);
    //camera->setReadBuffer(GL_COLOR_ATTACHMENT0);

    osgEarth::GLUtils::setGlobalDefaults(camera->getOrCreateStateSet()); //NOTE(alex): adding this fixed the line rendering
    m_viewer->setRealizeOperation(new osgEarth::GL3RealizeOperation()); //TODO(alex): required? for GL3/core?
    camera->setSmallFeatureCullingPixelSize(1.0f); // needed for point annotations (e.g. PlaceNode)
    camera->setNearFarRatio(0.0001);

    //camera->setClearColor(osg::Vec4(0.8f, 0.8f, 0.9f, 1.0f));
    camera->setClearColor(osg::Vec4(0, 0, 0, 0));
    camera->setClearMask(0);
    m_itemWidth = m_item->width();
    m_itemHeight = m_item->height();

    m_window->resized(0, 0, m_itemWidth, m_itemHeight);
    m_view->getCamera()->setViewport(new osg::Viewport(0, 0, m_itemWidth, m_itemHeight));
    double aspectRatio = static_cast<double>(m_itemWidth) / m_itemHeight;
    m_view->getCamera()->setProjectionMatrixAsPerspective(m_itemHeight * FOV_FACTOR, aspectRatio, 1.0, 10000.0);
#if 0 //NOTE(alex): rendering becomes super-slow after this
    m_view->getCamera()->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
#endif
    // Create scene graph
    m_root = new osg::Switch();

    m_view->setSceneData(m_root);

    m_maps = new osg::Switch();
    m_root->addChild(m_maps);


    unsigned char* pixeldata = (unsigned char*)malloc(50*25 * 4);

    int cntr=0;
    for (int i=0; i<50; ++i)
    {
        for (int j=0; j<25; ++j)
        {
            pixeldata[cntr++] = 255;
            pixeldata[cntr++] = 0;
            pixeldata[cntr++] = 255;
            pixeldata[cntr++] = 255;
        }
    }
    m_iconImage = new osg::Image();
    m_iconImage->setImage(25, 50, 1, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, pixeldata, osg::Image::NO_DELETE);


}

OsgEarthItemRenderer::OsgEarthItemRenderer(OsgEarthItem const* item)
{
    m_item = item;
    m_initialized = false;
    m_firstFrameOccurred = false;
}
OsgEarthItemRenderer::~OsgEarthItemRenderer()
{
}

QOpenGLFramebufferObject * OsgEarthItemRenderer::createFramebufferObject(const QSize & size)
{
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(4);
    m_framebufferObject = new QOpenGLFramebufferObject(size, format);
    return m_framebufferObject;
}

void OsgEarthItemRenderer::render()
{
    if (m_viewer)
    {
        // Without this line the model is not displayed in the second and subsequent frames:
        //NOTE(alex): not true anymore, apparently (since which osg/osgEarth version?)
        //QOpenGLContext::currentContext()->functions()->glUseProgram(0);

        if ( m_window->getTraits()->width != m_itemWidth ||
             m_window->getTraits()->height != m_itemHeight)
        {
            m_window->resized(0, 0, m_itemWidth, m_itemHeight);
            m_view->getCamera()->setViewport(new osg::Viewport(0, 0, m_itemWidth, m_itemHeight));
            double aspectRatio = static_cast<double>(m_itemWidth) / m_itemHeight;
            double fovy = m_itemHeight * FOV_FACTOR; //NOTE(alex): Don't really understand it but seems to work.
            m_view->getCamera()->setProjectionMatrixAsPerspective(fovy, aspectRatio, 1.0, 10000.0);
            //m_view->getCamera()->setInheritanceMask( m_view->getCamera()->getInheritanceMask()& ~osg::Camera::COMPUTE_NEAR_FAR_MODE );
            //m_view->getCamera()->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
        }

        if (m_viewer)
        {
#if 0 //TODO(alex): could this help with visual artifacts on osgEarth renderings?
            osg::State *osgState = m_view->getCamera()->getGraphicsContext()->getState();
            osgState->popAllStateSets();
            osgState->apply();
#endif
            //Blit gradient background
            glClearColor(0.5, 0.5, 0.5, 1);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
            GLint drawFboId = 0;
            glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFboId);

            m_viewer->getCamera()->getGraphicsContext()->setDefaultFboId(drawFboId);

            m_viewer->frame();
        }

        m_item->window()->resetOpenGLState();
    }
}

void OsgEarthItemRenderer::synchronize(QQuickFramebufferObject* item)
{
    if (m_initialized && !m_firstFrameOccurred)
    {
        //m_viewer->addEventHandler(new osgViewer::StatsHandler);
        //m_viewer->realize();

        m_map = new osgEarth::Map();

        osgEarth::Drivers::RexTerrainEngine::RexTerrainEngineOptions rex;

        osgEarth::MapNodeOptions mapNodeOptions;
        mapNodeOptions.setTerrainOptions(rex);

        m_mapNode = new osgEarth::MapNode(m_map, mapNodeOptions);
        m_maps->addChild(m_mapNode);

        m_manipulator = new osgEarth::Util::EarthManipulator();
        m_manipulator->getSettings()->bindMouseDoubleClick(osgEarth::Util::EarthManipulator::ACTION_NULL, osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON);
        m_manipulator->getSettings()->bindMouseDoubleClick(osgEarth::Util::EarthManipulator::ACTION_NULL, osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON);
        m_manipulator->getSettings()->bindMouseClick(osgEarth::Util::EarthManipulator::ACTION_NULL, osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON);
        m_manipulator->getSettings()->bindMouse(osgEarth::Util::EarthManipulator::ACTION_NULL, osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON);
        m_view->setCameraManipulator(m_manipulator);

        char const* url = "http://readymap.org/readymap/tiles/1.0.0/7/";
        osgEarth::Drivers::TMSOptions imgLayerOptions;
        imgLayerOptions.url() = url;
        osgEarth::ImageLayer* imgLayer = new osgEarth::ImageLayer("Img Layer", imgLayerOptions);
        imgLayer->setVisible(true);
        m_map->addLayer(imgLayer);

        m_firstFrameOccurred = true;
    }

    if(!m_initialized)
    {
        initialize();
        m_initialized = true;
    }

    if (m_initialized && m_firstFrameOccurred)
    {
        OsgEarthItem* oeItem = dynamic_cast<OsgEarthItem*>(item);
        for (size_t i=0; i<oeItem->m_randEventQueue.size(); ++i)
        {
            OsgEarthItem::RandomPlaceEvent ev = oeItem->m_randEventQueue[i];
            osgEarth::Style pm;
            pm.getOrCreate<osgEarth::TextSymbol>()->halo() = osgEarth::Color("#5f5f5f");

            std::string pos = std::to_string(ev.lat) + " - " + std::to_string(ev.lon);
            auto bla = new osgEarth::Annotation::PlaceNode(osgEarth::GeoPoint(m_map->getSRS(), ev.lon, ev.lat), pos, pm, m_iconImage);
            bla->setDynamic(true);
            m_mapNode->addChild(bla);
        }
        oeItem->m_randEventQueue.clear();
    }
    OsgEarthItem* actualItem = dynamic_cast<OsgEarthItem*>(item);

    m_itemWidth = static_cast<int>(item->width());
    m_itemHeight = static_cast<int>(item->height());

    m_counter = actualItem->getCounter();

    {
        osgGA::EventQueue::Events events;
        actualItem->getEventQueue()->takeEvents(events);
        m_view->getEventQueue()->appendEvents(events);
    }
}


OsgEarthItem::OsgEarthItem(QQuickItem *parent)
    : QQuickFramebufferObject(parent)
    , m_eventQueue(new osgGA::EventQueue(osgGA::GUIEventAdapter::Y_INCREASING_DOWNWARDS))
{
    m_lastMouseX = -1;
    m_lastMouseY = -1;

    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true);
    setFlag(ItemAcceptsInputMethod, true);

    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &OsgEarthItem::update);
    setContinuousUpdate(true);

    QTimer* timer = new QTimer();
    QObject::connect(timer, &QTimer::timeout, this, &OsgEarthItem::createRandomPlaceNode);
    timer->start(1000);
}

OsgEarthItem::~OsgEarthItem()
{
}

void OsgEarthItem::setContinuousUpdate(bool enable)
{
    m_continuousUpdate = enable;
    if (enable)
    {
        m_updateTimer->start(17);
    }
    else
    {
        m_updateTimer->stop();
    }
}

QSGNode* OsgEarthItem::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* updateData)
{
    QSGNode* node = QQuickFramebufferObject::updatePaintNode(oldNode, updateData);
    if (node)
    {
        QSGSimpleTextureNode* textureNode = dynamic_cast<QSGSimpleTextureNode*>(node);
        textureNode->setTextureCoordinatesTransform(QSGSimpleTextureNode::MirrorVertically);
    }
    return node;
}

void OsgEarthItem::geometryChanged(QRectF const& newGeo, QRectF const& oldGeo)
{
    m_eventQueue->windowResize(newGeo.x(), newGeo.y(), newGeo.width(), newGeo.height());
    QQuickFramebufferObject::geometryChanged(newGeo, oldGeo);
}

static unsigned button(Qt::MouseButton button)
{
    unsigned result = 0;
    switch(button)
    {
    case Qt::LeftButton: result = 1; break;
    case Qt::MidButton: result = 2; break;
    case Qt::RightButton: result = 3; break;
    default: assert(!"unknown button");
    }
    return result;
}

void OsgEarthItem::mousePressEvent(QMouseEvent *event)
{
    m_eventQueue->mouseButtonPress(event->x(), event->y(), button(event->button()));
    update();
}

void OsgEarthItem::mouseMoveEvent(QMouseEvent *event)
{
    m_eventQueue->mouseMotion(event->x(), event->y());
    update();
}

void OsgEarthItem::hoverMoveEvent(QHoverEvent* event)
{
    //NOTE(alex): hoverMoveEvents are continuously fired if the mouse is on the item/widget,
    //regardless if mouse was moved or not! Does not make sense?!
    if (event->pos().x() != m_lastMouseX || event->pos().y() != m_lastMouseY)
    {
        m_eventQueue->mouseMotion(event->pos().x(), event->pos().y());
        m_lastMouseX = event->pos().x();
        m_lastMouseY = event->pos().y();
        update();
    }
}

void OsgEarthItem::mouseReleaseEvent(QMouseEvent *event)
{
    m_eventQueue->mouseButtonRelease(event->x(), event->y(), button(event->button()));
    update();
}

void OsgEarthItem::mouseDoubleClickEvent(QMouseEvent *event)
{
    m_eventQueue->mouseDoubleButtonPress(event->x(), event->y(), button(event->button()));
    update();
}
void OsgEarthItem::wheelEvent(QWheelEvent* event)
{
    m_eventQueue->mouseScroll(event->delta() < 0 ? osgGA::GUIEventAdapter::SCROLL_UP : osgGA::GUIEventAdapter::SCROLL_DOWN);
    update();
}

void OsgEarthItem::onQmlKeyPressed(int key, int modifiers, QString const& text)
{
    Q_UNUSED(key)
    Q_UNUSED(modifiers)
    if (text.size() == 1)
    {
        m_eventQueue->keyPress(text.toStdString().at(0));
        update();
    }
}

void OsgEarthItem::createRandomPlaceNode()
{
    double lat = -90 + 180.0 * rand() / (double)RAND_MAX;
    double lon = -180 + 360.0 * rand() / (double)RAND_MAX;
    m_randEventQueue.push_back(RandomPlaceEvent{lat, lon});
}
