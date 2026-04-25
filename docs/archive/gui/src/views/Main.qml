import QtQuick
import QtQuick.Controls
import qpmu 1.0
import "components"

/**
 * Main - Application entry point
 *
 * Qt Quick Controls ApplicationWindow with:
 * - AppToolBar header (back button, title, context menu)
 * - StackView for screen navigation
 * - Drawer for context menus (right edge)
 */
ApplicationWindow {
    id: mainWindow

    // Use AutomaticVisibility for platform-appropriate default
    // Can be overridden with command-line args or window controls
    visibility: Window.AutomaticVisibility
    visible: true

    // Set reasonable default size for windowed mode
    width: 1280
    height: 800

    title: "QPMU - Phasor Measurement Unit"

    // Toolbar header with standard navigation
    header: AppToolBar {
        stackView: mainStackView
        contextDrawer: contextMenuDrawer
    }

    // Main content: StackView for screen navigation
    StackView {
        id: mainStackView
        anchors.fill: parent
        initialItem: LiveMonitor {}
    }

    // Status bar footer
    footer: AppStatusBar {}

    // Context menu drawer (right edge)
    Drawer {
        id: contextMenuDrawer
        width: 320
        height: mainWindow.height
        edge: Qt.RightEdge
        modal: true
        interactive: true

        background: Rectangle {
            color: AppTheme.colors.surface
            border.color: AppTheme.colors.borderEmphasized
            border.width: AppTheme.border.thin
        }

        Loader {
            anchors.fill: parent
            sourceComponent: mainStackView.currentItem?.contextMenu ?? null
        }
    }
}
