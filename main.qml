import QtQuick 2.9
import QtQuick.Window 2.3
import QtQuick.Controls 2.2
import Test 1.0
QtObject
{
    property var topWindow: ApplicationWindow
    {
        id: top_window
        objectName: "top_window"
        title:"osgEarth Bug Repo (1)"
        x: 0
        y: 0
        width: 960
        height: 1080
        visible: true
        Rectangle
        {
            anchors.fill: parent
            OsgEarthItem
            {
                objectName: "osgearth_widget_01"
                anchors.fill: parent
            }
        }
    }
    property var dummy2: ApplicationWindow
    {
        id: bottom_window
        objectName: "bottom_window"
        title:"osgEarth Bug Repo (2)"
        x: 960
        y: 0
        width: 960
        height: 1080
        visible: true
        Rectangle
        {
            anchors.fill: parent
            OsgEarthItem
            {
                objectName: "osgearth_widget_02"
                anchors.fill: parent
            }
        }
    }
}
