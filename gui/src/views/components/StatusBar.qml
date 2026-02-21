import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    required property var dataModel

    height: 72
    color: "#0a0e14"
    border.color: dataModel.isPaused ? "#e63946" : "#06d6a0"
    border.width: 0

    // Top border only
    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 3
        color: dataModel.isPaused ? "#e63946" : "#06d6a0"

        Behavior on color {
            ColorAnimation { duration: 200 }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 24

        // Play/Pause button
        Rectangle {
            Layout.preferredWidth: 54
            Layout.preferredHeight: 44
            color: playPauseButton.pressed ? "#1b2838" : (playPauseButton.hovered ? "#1a2634" : "#0f1823")
            radius: 10
            border.color: root.dataModel.isPaused ? "#e63946" : "#06d6a0"
            border.width: 2.5

            Behavior on color {
                ColorAnimation { duration: 150 }
            }

            Text {
                anchors.centerIn: parent
                text: root.dataModel.isPaused ? "▶" : "⏸"
                font.pixelSize: 22
                color: root.dataModel.isPaused ? "#e63946" : "#06d6a0"
            }

            MouseArea {
                id: playPauseButton
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.dataModel.togglePause()
            }
        }

        // Status indicator
        Rectangle {
            Layout.preferredWidth: 110
            Layout.preferredHeight: 44
            color: root.dataModel.isPaused ? "#e6394620" : "#06d6a020"
            radius: 10
            border.color: root.dataModel.isPaused ? "#e63946" : "#06d6a0"
            border.width: 2

            Behavior on color {
                ColorAnimation { duration: 200 }
            }
            Behavior on border.color {
                ColorAnimation { duration: 200 }
            }

            RowLayout {
                anchors.centerIn: parent
                spacing: 10

                Rectangle {
                    width: 10
                    height: 10
                    radius: 5
                    color: root.dataModel.isPaused ? "#e63946" : "#06d6a0"

                    SequentialAnimation on opacity {
                        running: !root.dataModel.isPaused
                        loops: Animation.Infinite
                        NumberAnimation { from: 1.0; to: 0.3; duration: 800 }
                        NumberAnimation { from: 0.3; to: 1.0; duration: 800 }
                    }
                }

                Text {
                    text: root.dataModel.isPaused ? "PAUSED" : "LIVE"
                    font.pixelSize: 15
                    font.weight: Font.Bold
                    font.family: "SF Mono, Consolas, monospace"
                    color: root.dataModel.isPaused ? "#e63946" : "#06d6a0"
                }
            }
        }

        // Spacer
        Item { Layout.fillWidth: true }

        // Metrics display
        Row {
            spacing: 30

            // Time
            Column {
                spacing: 3
                Text {
                    text: "TIME"
                    font.pixelSize: 10
                    font.weight: Font.Bold
                    color: "#6b8cae"
                    font.family: "SF Pro Text, Segoe UI, sans-serif"
                }
                Text {
                    text: root.dataModel.lastSampleTime
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                    font.family: "SF Mono, Consolas, monospace"
                    color: "#d0dae8"
                }
            }

            Rectangle {
                width: 1.5
                height: 40
                color: "#2a3f5f"
            }

            // Sampling Rate
            Column {
                spacing: 3
                Text {
                    text: "SAMPLING RATE"
                    font.pixelSize: 10
                    font.weight: Font.Bold
                    color: "#6b8cae"
                    font.family: "SF Pro Text, Segoe UI, sans-serif"
                }
                Text {
                    text: root.dataModel.samplingRate.toFixed(1) + " Hz"
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                    font.family: "SF Mono, Consolas, monospace"
                    color: "#45b7d1"
                }
            }

            Rectangle {
                width: 1.5
                height: 40
                color: "#2a3f5f"
            }

            // Frequency
            Column {
                spacing: 3
                Text {
                    text: "FREQUENCY"
                    font.pixelSize: 10
                    font.weight: Font.Bold
                    color: "#6b8cae"
                    font.family: "SF Pro Text, Segoe UI, sans-serif"
                }
                Text {
                    text: root.dataModel.frequencies[0].toFixed(3) + " Hz"
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                    font.family: "SF Mono, Consolas, monospace"
                    color: "#06d6a0"
                }
            }
        }
    }
}
