pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: mainWindow
    width: 1280
    height: 800
    visible: true
    title: "QPMU - Phasor Measurement Unit"

    LiveMonitor {
        anchors.fill: parent
    }
}
