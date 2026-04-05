import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import qpmu 1.0

/**
 * AppToolBar - Application-wide toolbar component
 *
 * Follows Qt Quick Controls ToolBar pattern with standard semantics:
 * - Left: Back button (when stackView.depth > 1) or Menu button (when context menu available)
 * - Center: Current screen title
 * - Right: Context menu button (when current screen has contextMenuModel)
 */
ToolBar {
    id: root

    property StackView stackView
    property Drawer contextDrawer

    readonly property var currentScreen: stackView ? stackView.currentItem : null
    readonly property bool hasContextMenu: currentScreen ? currentScreen.contextMenu !== null : false

    background: Rectangle {
        color: AppTheme.colors.surface
        border.color: AppTheme.colors.borderEmphasized
        border.width: AppTheme.border.thin
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: AppTheme.spacing.small
        anchors.rightMargin: AppTheme.spacing.small
        spacing: AppTheme.spacing.small

        // Left: Back button (only when stacked)
        ToolButton {
            id: leftButton
            Layout.preferredWidth: AppTheme.sizing.medium
            Layout.preferredHeight: AppTheme.sizing.medium
            visible: root.stackView.depth > 1

            background: Rectangle {
                color: leftButton.pressed ? AppTheme.state.surfacePressed : "transparent"
                radius: AppTheme.radius.medium

                Behavior on color {
                    ColorAnimation {
                        duration: AppTheme.motion.fast
                    }
                }
            }

            contentItem: Text {
                text: "‹"
                font.pixelSize: AppTheme.typography.size.large
                font.weight: Font.Bold
                color: AppTheme.colors.textPrimary
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            onClicked: root.stackView.pop()

            // Note: HoverHandler.cursorShape removed for Qt 5.12 compatibility
            // Cursor shape on hover is a nice-to-have, not critical functionality
        }

        // Center: Title
        Label {
            Layout.fillWidth: true
            text: root.currentScreen ? root.currentScreen.title : ""
            font.pixelSize: AppTheme.typography.size.large
            font.weight: Font.Bold
            font.family: AppTheme.typography.fontFamily
            color: AppTheme.colors.textPrimary
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        // Right: Context menu button
        ToolButton {
            id: rightButton
            Layout.preferredWidth: AppTheme.sizing.medium
            Layout.preferredHeight: AppTheme.sizing.medium
            visible: root.hasContextMenu

            background: Rectangle {
                color: rightButton.pressed ? AppTheme.state.surfacePressed : "transparent"
                radius: AppTheme.radius.medium

                Behavior on color {
                    ColorAnimation {
                        duration: AppTheme.motion.fast
                    }
                }
            }

            contentItem: Text {
                text: "⋮"  // Vertical ellipsis
                font.pixelSize: AppTheme.typography.size.huge
                font.weight: Font.Bold
                color: AppTheme.colors.textPrimary
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            onClicked: root.contextDrawer.open()

            // Note: HoverHandler.cursorShape removed for Qt 5.12 compatibility
            // Cursor shape on hover is a nice-to-have, not critical functionality
        }
    }
}
