#include "osgearth_item.h"

#include <osgViewer/Viewer>
#include <osgGA/TrackballManipulator>
#include <osg/MatrixTransform>
#include <osgViewer/GraphicsWindow>

#include <QtQuick/QQuickWindow>
#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLFramebufferObject>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLExtraFunctions>
#include <QDebug>
#include <QTimer>
#include <qsgsimpletexturenode.h>
#include <assert.h>


#include <osgEarth/GLUtils>
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

void OsgEarthItemRenderer::initOsg()
{
    osg::setNotifyLevel(osg::NotifySeverity::WARN);

    for (int i=0; i<2; ++i)
    {
        m_renderViews[i].x = 0;
        m_renderViews[i].y = i*m_item->height() / 2.0;
        m_renderViews[i].w = m_item->width();
        m_renderViews[i].h = m_item->height() / 2.0;

        m_renderViews[i].m_viewer = new osgViewer::Viewer();
        m_renderViews[i].m_window = m_renderViews[i].m_viewer->setUpViewerAsEmbeddedInWindow(m_renderViews[i].x,
                                                                                             m_renderViews[i].y,
                                                                                             m_renderViews[i].w,
                                                                                             m_renderViews[i].h);

        m_renderViews[i].m_viewer->getCamera()->setGraphicsContext(m_renderViews[i].m_window.get());
        m_renderViews[i].m_viewer->getCamera()->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        m_renderViews[i].m_window->resized(m_renderViews[i].x, m_renderViews[i].y, m_renderViews[i].w, m_renderViews[i].h);
        m_renderViews[i].m_viewer->getCamera()->setViewport(new osg::Viewport(m_renderViews[i].x, m_renderViews[i].y, m_renderViews[i].w, m_renderViews[i].h));
        m_renderViews[i].m_viewer->getCamera()->setProjectionMatrixAsPerspective(60, static_cast<double>(m_renderViews[i].w) / m_renderViews[i].h, 1.0, 10000.0);

        m_renderViews[i].m_root = new osg::Switch();
        m_renderViews[i].m_viewer->setSceneData(m_renderViews[i].m_root);
    }

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

void OsgEarthItemRenderer::initOsgEarth()
{
    for (int i=0; i<2; ++i)
    {
        osgEarth::GLUtils::setGlobalDefaults(m_renderViews[i].m_viewer->getCamera()->getOrCreateStateSet());

        m_renderViews[i].m_map = new osgEarth::Map();

        osgEarth::MapNodeOptions mapNodeOptions;
        mapNodeOptions.setTerrainOptions(osgEarth::Drivers::RexTerrainEngine::RexTerrainEngineOptions());

        m_renderViews[i].m_mapNode = new osgEarth::MapNode(m_renderViews[i].m_map, mapNodeOptions);
        m_renderViews[i].m_root->addChild(m_renderViews[i].m_mapNode);

        m_renderViews[i].m_manipulator = new osgEarth::Util::EarthManipulator();
        m_renderViews[i].m_viewer->setCameraManipulator(m_renderViews[i].m_manipulator);

        if (i == 0)
        {
            osgEarth::Viewpoint vp;
            vp.range() = 19134906.407120;
            vp.focalPoint() = osgEarth::GeoPoint(m_renderViews[0].m_map->getSRS(), -129.250176, 0.478506, -693.324114, osgEarth::ALTMODE_ABSOLUTE);
            vp.heading() = 0;
            vp.pitch() = -88.998893;
            m_renderViews[0].m_manipulator->setViewpoint(vp);
        }

#if 1
        char const* url = "http://readymap.org/readymap/tiles/1.0.0/7/";
        osgEarth::Drivers::TMSOptions imgLayerOptions;
        imgLayerOptions.url() = url;
        osgEarth::ImageLayer* imgLayer = new osgEarth::ImageLayer("Img Layer", imgLayerOptions);
        imgLayer->setVisible(true);
        m_renderViews[i].m_map->addLayer(imgLayer);
#endif
    }

    double lat =  -22;
    double lon =  -45;

    for (int i=0; i<2; ++i)
    {
        osgEarth::Style pm;
        pm.getOrCreate<osgEarth::TextSymbol>()->halo() = osgEarth::Color("#5f5f5f");

        std::string posStr = std::to_string(lat) + ", " + std::to_string(lon);
        auto pn = new osgEarth::Annotation::PlaceNode(osgEarth::GeoPoint(m_renderViews[i].m_map->getSRS(), lon, lat), posStr, pm, m_iconImage);
        m_renderViews[i].m_mapNode->addChild(pn);
    }
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
    auto fbo = new QOpenGLFramebufferObject(size, format);
    return fbo;
}

void OsgEarthItemRenderer::render()
{
    for (int i=0; i<2; ++i)
    {
        if (m_renderViews[i].m_viewer)
        {
            GLint drawFboId = 0;
            glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFboId);
            m_renderViews[i].m_viewer->getCamera()->getGraphicsContext()->setDefaultFboId(drawFboId);

            m_renderViews[i].m_viewer->frame();

        }
    }
    m_item->window()->resetOpenGLState();
}

void OsgEarthItemRenderer::synchronize(QQuickFramebufferObject* item)
{
    if (m_initialized && !m_firstFrameOccurred)
    {
        initOsgEarth();
        m_firstFrameOccurred = true;
    }

    if(!m_initialized)
    {
        initOsg();
        m_initialized = true;
    }

    for (int i=0; i<2; ++i)
    {
        osgGA::EventQueue::Events events;
        dynamic_cast<OsgEarthItem*>(item)->getEventQueue()->takeEvents(events);
        m_renderViews[i].m_viewer->getEventQueue()->appendEvents(events);
    }
}


OsgEarthItem::OsgEarthItem(QQuickItem *parent)
    : QQuickFramebufferObject(parent)
    , m_eventQueue(new osgGA::EventQueue(osgGA::GUIEventAdapter::Y_INCREASING_DOWNWARDS))
{
    setAcceptedMouseButtons(Qt::AllButtons);
    setFlag(ItemAcceptsInputMethod, true);

    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &OsgEarthItem::update);
    m_updateTimer->start(0);
}

OsgEarthItem::~OsgEarthItem()
{
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

void OsgEarthItem::mouseReleaseEvent(QMouseEvent *event)
{
    m_eventQueue->mouseButtonRelease(event->x(), event->y(), button(event->button()));
    update();
}

void OsgEarthItem::wheelEvent(QWheelEvent* event)
{
    m_eventQueue->mouseScroll(event->delta() < 0 ? osgGA::GUIEventAdapter::SCROLL_UP : osgGA::GUIEventAdapter::SCROLL_DOWN);
    update();
}
