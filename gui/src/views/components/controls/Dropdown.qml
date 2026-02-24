pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import qpmu

// Styled ComboBox for phase reference selection
ComboBox {
    id: root

    implicitHeight: AppTheme.sizing.buttonMedium
    font.pixelSize: AppTheme.typography.size.normal
    font.family: AppTheme.typography.fontFamily

    background: Rectangle {
        color: root.pressed ? AppTheme.state.surfacePressed : (root.hovered ? AppTheme.state.surfaceHover : AppTheme.state.surfaceDefault)
        radius: AppTheme.radius.large
        border.color: root.activeFocus ? AppTheme.colors.primary : AppTheme.colors.borderEmphasized
        border.width: AppTheme.border.thin

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
    }

    contentItem: Text {
        leftPadding: AppTheme.spacing.medium
        rightPadding: root.indicator.width + AppTheme.spacing.medium
        text: root.displayText
        font: root.font
        color: AppTheme.colors.textPrimary
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    indicator: Text {
        x: root.width - width - AppTheme.spacing.small
        y: root.topPadding + (root.availableHeight - height) / 2
        text: "▼"
        font.pixelSize: AppTheme.typography.size.small
        color: AppTheme.colors.textTertiary
    }

    popup: Popup {
        y: root.height + 4
        width: root.width
        implicitHeight: contentItem.implicitHeight
        padding: 4

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: root.popup.visible ? root.delegateModel : null
            currentIndex: root.highlightedIndex

            ScrollIndicator.vertical: ScrollIndicator {}
        }

        background: Rectangle {
            color: AppTheme.colors.surfaceElevated
            border.color: AppTheme.colors.borderEmphasized
            border.width: AppTheme.border.thin
            radius: AppTheme.radius.medium
        }
    }

    delegate: ItemDelegate {
        id: delegateItem
        required property var model
        required property int index

        width: root.width - 8
        height: AppTheme.sizing.buttonMedium

        highlighted: root.highlightedIndex === index

        contentItem: Text {
            text: delegateItem.model[root.textRole]
            font: root.font
            color: delegateItem.highlighted ? AppTheme.colors.primary : AppTheme.colors.textPrimary
            verticalAlignment: Text.AlignVCenter

            Behavior on color {
                ColorAnimation {
                    duration: AppTheme.motion.fast
                }
            }
        }

        background: Rectangle {
            color: delegateItem.highlighted ? AppTheme.withAlpha(AppTheme.colors.primary, AppTheme.opacity.overlayLight) : "transparent"
            radius: AppTheme.radius.small

            Behavior on color {
                ColorAnimation {
                    duration: AppTheme.motion.fast
                }
            }
        }
    }
}
