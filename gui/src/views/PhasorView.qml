import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QPMU 1.0
import "components"

Rectangle {
    id: root
    color: "#050810"

    // Data model instance
    PhasorDataModel {
        id: dataModel
        Component.onCompleted: {
            startSimulation()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Main content area - side by side phasor and waveform
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

            PhasorPlot {
                SplitView.preferredWidth: parent.width * 0.4
                SplitView.fillHeight: true
                dataModel: dataModel
            }

            WaveformPlot {
                SplitView.fillWidth: true
                SplitView.fillHeight: true
                dataModel: dataModel
            }
        }

        // Summary bar at bottom
        StatusBar {
            Layout.fillWidth: true
            dataModel: dataModel
        }
    }
}
