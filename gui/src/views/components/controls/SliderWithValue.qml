import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import qpmu 1.0

// Slider with numeric value display for manual cutoff values
RowLayout {
    id: root

    property alias from: slider.from
    property alias to: slider.to
    property alias value: slider.value
    property alias stepSize: slider.stepSize
    property string unit: ""
    property int decimals: 1

    spacing: AppTheme.spacing.medium

    Slider {
        id: slider
        Layout.fillWidth: true
        Layout.preferredHeight: AppTheme.sizing.medium

        from: 0
        to: 500
        stepSize: 1
        value: 100

        background: Rectangle {
            x: slider.leftPadding
            y: slider.topPadding + slider.availableHeight / 2 - height / 2
            implicitWidth: 200
            implicitHeight: 4
            width: slider.availableWidth
            height: implicitHeight
            radius: 2
            color: AppTheme.colors.borderSubtle

            Rectangle {
                width: slider.visualPosition * parent.width
                height: parent.height
                color: AppTheme.colors.primary
                radius: 2
            }
        }

        handle: Rectangle {
            x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
            y: slider.topPadding + slider.availableHeight / 2 - height / 2
            implicitWidth: 20
            implicitHeight: 20
            radius: 10
            color: slider.pressed ? AppTheme.colors.primaryPressed : (slider.hovered ? AppTheme.colors.primaryHover : AppTheme.colors.primary)
            border.color: AppTheme.colors.surfaceElevated
            border.width: 2

            Behavior on color {
                ColorAnimation {
                    duration: AppTheme.motion.fast
                }
            }
        }
    }

    Rectangle {
        Layout.preferredWidth: 70
        Layout.preferredHeight: AppTheme.sizing.small
        color: AppTheme.colors.surfaceElevated
        radius: AppTheme.radius.small
        border.color: AppTheme.colors.borderEmphasized
        border.width: AppTheme.border.thin

        Text {
            anchors.centerIn: parent
            text: slider.value.toFixed(root.decimals) + (root.unit.length > 0 ? " " + root.unit : "")
            font.pixelSize: AppTheme.typography.size.small
            font.weight: Font.Medium
            font.family: AppTheme.typography.fontFamilyMonospace
            color: AppTheme.colors.textPrimary
        }
    }
}
