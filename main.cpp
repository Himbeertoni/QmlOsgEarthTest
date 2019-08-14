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
    QGuiApplication application(argc, argv);

    qmlRegisterType<OsgEarthItem>("Test", 1, 0, "OsgEarthItem");

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    if (engine.rootObjects().isEmpty())
    {
        return -1;
    }
    return application.exec();
}
