
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import qpmu 1.0
import "components"

/**
 * LiveMonitor - Main screen for live PMU data monitoring
 *
 * Displays:
 * - Phasor and waveform plots
 * - Signal data table
 * - Status bar
 *
 * Provides context menu for data/plot controls
 */
Screen {
    id: root

    title: "Live Monitor"

    color: AppTheme.colors.background

    // Detect narrow screens (portrait or narrow landscape)
    property bool isNarrow: width < height * 1.5

    // Start simulation when LiveMonitor is created
    Component.onCompleted: {
        appInstance.startSimulation();
    }

    // Context menu for this screen
    contextMenu: Component {
        LiveMonitorContextMenu {}
    }

    // Main content area - graph view and data table
    // Qt 5.12 compatible: Use Row/Column instead of SplitView (which requires Qt 5.13+)
    Item {
        anchors.fill: parent

        // Horizontal layout (wide screens)
        Row {
            anchors.fill: parent
            visible: !root.isNarrow

            GraphViewSwitcher {
                width: parent.width * 0.6
                height: parent.height
            }

            // Divider
            Rectangle {
                width: 2
                height: parent.height
                color: AppTheme.colors.border
            }

            DataTable {
                width: parent.width * 0.4 - 2
                height: parent.height
            }
        }

        // Vertical layout (narrow screens)
        Column {
            anchors.fill: parent
            visible: root.isNarrow

            GraphViewSwitcher {
                width: parent.width
                height: parent.height * 0.5
            }

            // Divider
            Rectangle {
                width: parent.width
                height: 2
                color: AppTheme.colors.border
            }

            DataTable {
                width: parent.width
                height: parent.height * 0.5 - 2
            }
        }
    }
}
