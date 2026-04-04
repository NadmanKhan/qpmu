import QtQuick 2.12
import qpmu 1.0

// Base button component with AppTheme styling
Rectangle {
    id: root

    property string label: ""
    property string icon: ""
    property bool active: false
    property alias cursorShape: mouseArea.cursorShape

    signal clicked

    implicitWidth: label.length > 0 ? labelText.width + AppTheme.spacing.large * 2 : AppTheme.sizing.medium
    implicitHeight: AppTheme.sizing.medium

    color: mouseArea.pressed ? AppTheme.state.surfacePressed : (mouseArea.containsMouse ? AppTheme.state.surfaceHover : AppTheme.state.surfaceDefault)
    radius: AppTheme.radius.large
    border.color: active ? AppTheme.colors.primary : AppTheme.colors.borderEmphasized
    border.width: active ? AppTheme.border.thick : AppTheme.border.thin

    Behavior on color {
        ColorAnimation {
            duration: AppTheme.motion.fast
        }
    }

    Behavior on border.color {
        ColorAnimation {
            duration: AppTheme.motion.fast
        }
    }

    Row {
        anchors.centerIn: parent
        spacing: AppTheme.spacing.small

        Text {
            visible: icon.length > 0
            text: root.icon
            font.pixelSize: AppTheme.typography.size.large
            color: root.active ? AppTheme.colors.primary : AppTheme.colors.textSecondary
            anchors.verticalCenter: parent.verticalCenter

            Behavior on color {
                ColorAnimation {
                    duration: AppTheme.motion.fast
                }
            }
        }

        Text {
            id: labelText
            visible: label.length > 0
            text: root.label
            font.pixelSize: AppTheme.typography.size.normal
            font.weight: Font.DemiBold
            font.family: AppTheme.typography.fontFamily
            color: root.active ? AppTheme.colors.primary : AppTheme.colors.textPrimary
            anchors.verticalCenter: parent.verticalCenter

            Behavior on color {
                ColorAnimation {
                    duration: AppTheme.motion.fast
                }
            }
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
