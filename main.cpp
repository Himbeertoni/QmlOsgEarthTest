#include <iostream>

#include <QGuiApplication>
#include <QDebug>
#include <QScreen>
#include <QQmlApplicationEngine>
#include <QtGlobal>
#include <QObject>
#include <QStringList>
#include <QWindow>
#include <QQmlContext>
#include <QtGui/QSurfaceFormat>

#include "osgearth_item.h"

#include <stdint.h>

int main(int argc, char* argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QSurfaceFormat fmt(QSurfaceFormat::DebugContext);
    fmt.setVersion(4, 5);
    fmt.setSamples(4);
    QSurfaceFormat::setDefaultFormat(fmt);

    QGuiApplication application(argc, argv);

    qmlRegisterType<OsgEarthItem>("Test", 1, 0, "OsgEarthItem");
    //qRegisterMetaType<OsgEarthItem>("OsgEarthItem");

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    if (engine.rootObjects().isEmpty())
    {
        return -1;
    }
    else
    {
        for (QObject* i: engine.rootObjects())
        {
            QObject *foundObj2 = i->findChild<QObject*>("osgearth_widget_01");
            if (foundObj2)
            {
                OsgEarthItem* osgEarthItem01 = dynamic_cast<OsgEarthItem*>(foundObj2);
            }
            else
            {
                qWarning() << "No osgearth_widget_01 widget found!";
            }

            QObject *foundObj3 = i->findChild<QObject*>("osgearth_widget_02");
            if (foundObj3)
            {
                OsgEarthItem* osgEarthItem02 = dynamic_cast<OsgEarthItem*>(foundObj3);
            }
            else
            {
                qWarning() << "No osgearth_widget_02 widget found!";
            }
        }
    }
    return application.exec();
}
