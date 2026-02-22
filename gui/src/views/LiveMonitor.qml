import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "components"

Rectangle {
    id: root
    color: "#050810"

    // Application model - manages all signal data
    ApplicationDataModel {
        id: appDataModel
        Component.onCompleted: {
            startSimulation()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Main content area - graph view on left, data table on right
        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            handle: Rectangle {
                implicitWidth: 6
                color: SplitHandle.hovered ? "#3d5a80" : "#1b2838"

                Behavior on color {
                    ColorAnimation { duration: 200 }
                }

                Rectangle {
                    anchors.centerIn: parent
                    width: 2
                    height: parent.height * 0.3
                    radius: 1
                    color: SplitHandle.hovered ? "#6b8cae" : "#2a3f5f"

                    Behavior on color {
                        ColorAnimation { duration: 200 }
                    }
                }
            }

            // Left: Switchable graph view (Phasor or Waveform)
            GraphViewSwitcher {
                SplitView.preferredWidth: parent.width * 0.5
                SplitView.minimumWidth: 400
                SplitView.fillHeight: true
                appDataModel: appDataModel
            }

            // Right: Data table
            SignalTable {
                SplitView.fillWidth: true
                SplitView.minimumWidth: 400
                SplitView.fillHeight: true
                appDataModel: appDataModel
            }
        }

        // Summary bar at bottom
        StatusBar {
            Layout.fillWidth: true
            appDataModel: appDataModel
        }
    }
}
