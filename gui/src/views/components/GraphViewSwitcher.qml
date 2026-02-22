pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

// Graph view switcher - toggles between phasor and waveform views
Rectangle {
    id: root
    color: "#0a0e14"

    required property ApplicationDataModel appDataModel

    property int currentView: 0  // 0 = Phasor, 1 = Waveform

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Toggle control
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 55
            color: "#0a0e14"
            z: 10

            Row {
                anchors.centerIn: parent
                spacing: 0

                // Phasor button
                Rectangle {
                    width: 140
                    height: 42
                    color: root.currentView === 0 ? "#3d5a80" : "#1b2838"
                    radius: 8

                    Behavior on color {
                        ColorAnimation {
                            duration: 200
                        }
                    }

                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: 3
                        color: "#06d6a0"
                        radius: 1.5
                        visible: root.currentView === 0
                    }

                    Row {
                        anchors.centerIn: parent
                        spacing: 8

                        Text {
                            text: "◉"
                            font.pixelSize: 18
                            color: root.currentView === 0 ? "#06d6a0" : "#6b8cae"
                            anchors.verticalCenter: parent.verticalCenter

                            Behavior on color {
                                ColorAnimation {
                                    duration: 200
                                }
                            }
                        }

                        Text {
                            text: "Phasor"
                            font.pixelSize: 15
                            font.weight: Font.DemiBold
                            font.family: "SF Pro Text, Segoe UI, sans-serif"
                            color: root.currentView === 0 ? "#d0dae8" : "#6b8cae"
                            anchors.verticalCenter: parent.verticalCenter

                            Behavior on color {
                                ColorAnimation {
                                    duration: 200
                                }
                            }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.currentView = 0
                    }
                }

                // Small spacer
                Item {
                    width: 4
                    height: 1
                }

                // Waveform button
                Rectangle {
                    width: 140
                    height: 42
                    color: root.currentView === 1 ? "#3d5a80" : "#1b2838"
                    radius: 8

                    Behavior on color {
                        ColorAnimation {
                            duration: 200
                        }
                    }

                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: 3
                        color: "#06d6a0"
                        radius: 1.5
                        visible: root.currentView === 1
                    }

                    Row {
                        anchors.centerIn: parent
                        spacing: 8

                        Text {
                            text: "∿"
                            font.pixelSize: 18
                            color: root.currentView === 1 ? "#06d6a0" : "#6b8cae"
                            anchors.verticalCenter: parent.verticalCenter

                            Behavior on color {
                                ColorAnimation {
                                    duration: 200
                                }
                            }
                        }

                        Text {
                            text: "Waveform"
                            font.pixelSize: 15
                            font.weight: Font.DemiBold
                            font.family: "SF Pro Text, Segoe UI, sans-serif"
                            color: root.currentView === 1 ? "#d0dae8" : "#6b8cae"
                            anchors.verticalCenter: parent.verticalCenter

                            Behavior on color {
                                ColorAnimation {
                                    duration: 200
                                }
                            }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.currentView = 1
                    }
                }
            }
        }

        // View container with cross-fade transition
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            PhasorPlotModel {
                id: phasorView
                anchors.fill: parent
                appDataModel: root.appDataModel
                opacity: root.currentView === 0 ? 1.0 : 0.0
                visible: opacity > 0

                Behavior on opacity {
                    NumberAnimation {
                        duration: 250
                        easing.type: Easing.InOutQuad
                    }
                }
            }

            WaveformPlotModel {
                id: waveformView
                anchors.fill: parent
                appDataModel: root.appDataModel
                opacity: root.currentView === 1 ? 1.0 : 0.0
                visible: opacity > 0

                Behavior on opacity {
                    NumberAnimation {
                        duration: 250
                        easing.type: Easing.InOutQuad
                    }
                }
            }
        }
    }

    // Keyboard shortcuts
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Tab || event.key === Qt.Key_Space) {
            root.currentView = (root.currentView + 1) % 2;
            event.accepted = true;
        } else if (event.key === Qt.Key_P) {
            root.currentView = 0;
            event.accepted = true;
        } else if (event.key === Qt.Key_W) {
            root.currentView = 1;
            event.accepted = true;
        }
    }

    // Enable keyboard focus
    focus: true
}
