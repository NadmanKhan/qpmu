pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import qpmu

ApplicationWindow {
    id: mainWindow

    // Use AutomaticVisibility for platform-appropriate default
    // Can be overridden with command-line args or window controls
    visibility: Window.AutomaticVisibility
    visible: true

    // Set reasonable default size for windowed mode
    width: 1280
    height: 800

    title: "QPMU - Phasor Measurement Unit"

    LiveMonitor {
        anchors.fill: parent
    }
}
