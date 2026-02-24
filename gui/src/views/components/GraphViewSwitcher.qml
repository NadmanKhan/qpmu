pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import qpmu

// Graph view switcher - toggles between phasor and waveform views
Rectangle {
    id: root

    // Reference to singleton for qualified access

    color: AppTheme.colors.surface

    required property ApplicationDataModel appDataModel
    required property ViewStateModel viewStateModel

    property int currentView: 0  // 0 = Phasor, 1 = Waveform

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Toggle control
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 55
            color: AppTheme.colors.surface
            z: 10

            Row {
                anchors.centerIn: parent
                spacing: 0

                // Phasor button
                Rectangle {
                    width: 140
                    height: 42
                    color: root.currentView === 0 ? AppTheme.colors.borderEmphasized : AppTheme.colors.surfaceElevated
                    radius: AppTheme.radius.medium

                    Behavior on color {
                        ColorAnimation {
                            duration: AppTheme.motion.normal
                        }
                    }

                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: AppTheme.border.thick
                        color: AppTheme.colors.primary
                        radius: AppTheme.border.thin
                        visible: root.currentView === 0
                    }

                    Row {
                        anchors.centerIn: parent
                        spacing: AppTheme.spacing.small

                        Text {
                            text: "◉"
                            font.pixelSize: AppTheme.typography.size.large
                            color: root.currentView === 0 ? AppTheme.colors.primary : AppTheme.colors.textTertiary
                            anchors.verticalCenter: parent.verticalCenter

                            Behavior on color {
                                ColorAnimation {
                                    duration: AppTheme.motion.normal
                                }
                            }
                        }

                        Text {
                            text: "Phasor"
                            font.pixelSize: AppTheme.typography.size.normal
                            font.weight: Font.DemiBold
                            font.family: AppTheme.typography.fontFamily
                            color: root.currentView === 0 ? AppTheme.colors.textPrimary : AppTheme.colors.textTertiary
                            anchors.verticalCenter: parent.verticalCenter

                            Behavior on color {
                                ColorAnimation {
                                    duration: AppTheme.motion.normal
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
                    color: root.currentView === 1 ? AppTheme.colors.borderEmphasized : AppTheme.colors.surfaceElevated
                    radius: AppTheme.radius.medium

                    Behavior on color {
                        ColorAnimation {
                            duration: AppTheme.motion.normal
                        }
                    }

                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: AppTheme.border.thick
                        color: AppTheme.colors.primary
                        radius: AppTheme.border.thin
                        visible: root.currentView === 1
                    }

                    Row {
                        anchors.centerIn: parent
                        spacing: AppTheme.spacing.small

                        Text {
                            text: "∿"
                            font.pixelSize: AppTheme.typography.size.large
                            color: root.currentView === 1 ? AppTheme.colors.primary : AppTheme.colors.textTertiary
                            anchors.verticalCenter: parent.verticalCenter

                            Behavior on color {
                                ColorAnimation {
                                    duration: AppTheme.motion.normal
                                }
                            }
                        }

                        Text {
                            text: "Waveform"
                            font.pixelSize: AppTheme.typography.size.normal
                            font.weight: Font.DemiBold
                            font.family: AppTheme.typography.fontFamily
                            color: root.currentView === 1 ? AppTheme.colors.textPrimary : AppTheme.colors.textTertiary
                            anchors.verticalCenter: parent.verticalCenter

                            Behavior on color {
                                ColorAnimation {
                                    duration: AppTheme.motion.normal
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
                viewStateModel: root.viewStateModel
                opacity: root.currentView === 0 ? 1.0 : 0.0
                visible: opacity > 0

                Behavior on opacity {
                    NumberAnimation {
                        duration: AppTheme.motion.slow
                        easing.type: AppTheme.motion.easeInOut
                    }
                }
            }

            WaveformPlotModel {
                id: waveformView
                anchors.fill: parent
                appDataModel: root.appDataModel
                viewStateModel: root.viewStateModel
                opacity: root.currentView === 1 ? 1.0 : 0.0
                visible: opacity > 0

                Behavior on opacity {
                    NumberAnimation {
                        duration: AppTheme.motion.slow
                        easing.type: AppTheme.motion.easeInOut
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
