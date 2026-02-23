pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import qpmu
import "components"

Rectangle {
    id: root

    color: AppTheme.colors.background

    // Detect narrow screens (portrait or narrow landscape)
    property bool isNarrow: width < height * 1.5

    // Application model - manages all signal data
    ApplicationDataModel {
        id: appDataModel
        Component.onCompleted: {
            startSimulation();
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Main content area - graph view on left/top, data table on right/bottom
        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: root.isNarrow ? Qt.Vertical : Qt.Horizontal

            handle: Rectangle {
                implicitWidth: root.isNarrow ? parent.width : 6
                implicitHeight: root.isNarrow ? 6 : parent.height
                color: SplitHandle.hovered ? AppTheme.colors.borderEmphasized : AppTheme.colors.surfaceElevated

                Behavior on color {
                    ColorAnimation {
                        duration: AppTheme.motion.normal
                    }
                }

                Rectangle {
                    anchors.centerIn: parent
                    width: root.isNarrow ? parent.width * 0.3 : 2
                    height: root.isNarrow ? 2 : parent.height * 0.3
                    radius: 1
                    color: SplitHandle.hovered ? AppTheme.colors.textTertiary : AppTheme.colors.border

                    Behavior on color {
                        ColorAnimation {
                            duration: AppTheme.motion.normal
                        }
                    }
                }
            }

            // Graph view (Left in horizontal, Top in vertical)
            GraphViewSwitcher {
                SplitView.preferredWidth: root.isNarrow ? parent.width : parent.width * 0.5
                SplitView.preferredHeight: root.isNarrow ? parent.height * 0.5 : parent.height
                SplitView.minimumWidth: root.isNarrow ? 100 : parent.width * 0.3
                SplitView.minimumHeight: root.isNarrow ? parent.height * 0.2 : 100
                SplitView.fillHeight: !root.isNarrow
                SplitView.fillWidth: root.isNarrow
                appDataModel: appDataModel
            }

            // Data table (Right in horizontal, Bottom in vertical)
            SignalTable {
                SplitView.fillWidth: true
                SplitView.fillHeight: true
                SplitView.minimumWidth: root.isNarrow ? 100 : parent.width * 0.3
                SplitView.minimumHeight: root.isNarrow ? parent.height * 0.2 : 100
                appDataModel: appDataModel
            }
        }

        // Summary bar at bottom
        StatusBar {
            Layout.fillWidth: true
            appDataModel: appDataModel
        }
    }
}
