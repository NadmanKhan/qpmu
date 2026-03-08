pragma ComponentBehavior: Bound
import QtQuick
import qpmu

// Toggle button for two-state controls (RMS/Peak, Dynamic/Manual)
Rectangle {
    id: root

    property string leftLabel: ""
    property string rightLabel: ""
    property bool isRightActive: false  // false = left active, true = right active

    signal toggled

    implicitWidth: 200
    implicitHeight: AppTheme.sizing.medium

    color: AppTheme.colors.surfaceElevated
    radius: AppTheme.radius.large

    Row {
        anchors.fill: parent
        anchors.margins: 3
        spacing: 2

        // Left option
        Rectangle {
            width: (parent.width - parent.spacing) / 2
            height: parent.height
            color: !root.isRightActive ? AppTheme.colors.primary : "transparent"
            radius: AppTheme.radius.medium

            Behavior on color {
                ColorAnimation {
                    duration: AppTheme.motion.normal
                }
            }

            Text {
                anchors.centerIn: parent
                text: root.leftLabel
                font.pixelSize: AppTheme.typography.size.normal
                font.weight: !root.isRightActive ? Font.Bold : Font.Medium
                font.family: AppTheme.typography.fontFamily
                color: !root.isRightActive ? AppTheme.colors.textInverse : AppTheme.colors.textSecondary

                Behavior on color {
                    ColorAnimation {
                        duration: AppTheme.motion.normal
                    }
                }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (root.isRightActive) {
                        root.isRightActive = false;
                        root.toggled();
                    }
                }
            }
        }

        // Right option
        Rectangle {
            width: (parent.width - parent.spacing) / 2
            height: parent.height
            color: root.isRightActive ? AppTheme.colors.primary : "transparent"
            radius: AppTheme.radius.medium

            Behavior on color {
                ColorAnimation {
                    duration: AppTheme.motion.normal
                }
            }

            Text {
                anchors.centerIn: parent
                text: root.rightLabel
                font.pixelSize: AppTheme.typography.size.normal
                font.weight: root.isRightActive ? Font.Bold : Font.Medium
                font.family: AppTheme.typography.fontFamily
                color: root.isRightActive ? AppTheme.colors.textInverse : AppTheme.colors.textSecondary

                Behavior on color {
                    ColorAnimation {
                        duration: AppTheme.motion.normal
                    }
                }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (!root.isRightActive) {
                        root.isRightActive = true;
                        root.toggled();
                    }
                }
            }
        }
    }
}
