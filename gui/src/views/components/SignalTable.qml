import QtQuick 2.12
import QtQuick.Controls 2.12
import QtCore
import qpmu 1.0

// Signal data table using TableView with QAbstractItemModel
Rectangle {
    id: root

    color: AppTheme.colors.surface

    Item {
        id: tableContainer
        anchors.fill: parent
        anchors.margins: AppTheme.spacing.medium

        // Top-left corner spacer (aligns with vertical header)
        Rectangle {
            id: cornerItem
            anchors.left: parent.left
            anchors.top: parent.top
            width: verticalHeaderView.width > 0 ? verticalHeaderView.width : 90  // Match vertical header width
            height: 40
            color: AppTheme.colors.surfaceElevated
            radius: AppTheme.radius.medium
            z: 2
        }

        // Horizontal header view - automatically syncs with TableView
        HorizontalHeaderView {
            id: headerView
            anchors.left: cornerItem.right
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.leftMargin: AppTheme.spacing.small
            syncView: tableView
            clip: true

            delegate: Rectangle {
                id: horizontalHeaderDelegate
                required property string display
                required property int index

                readonly property size headerSize: signalDataModel.headerData(horizontalHeaderDelegate.index, Qt.Horizontal, Qt.SizeHintRole)
                implicitWidth: headerSize.width
                implicitHeight: 40
                color: AppTheme.colors.surfaceElevated
                radius: AppTheme.radius.medium

                Text {
                    anchors.fill: parent
                    anchors.leftMargin: AppTheme.spacing.small
                    anchors.rightMargin: AppTheme.spacing.small
                    text: horizontalHeaderDelegate.display
                    font.pixelSize: AppTheme.typography.size.small
                    font.weight: Font.Bold
                    font.family: AppTheme.typography.fontFamily
                    color: AppTheme.colors.textSecondary
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignLeft
                    elide: Text.ElideRight
                }
            }
        }

        // Vertical header view with checkboxes
        VerticalHeaderView {
            id: verticalHeaderView
            anchors.left: parent.left
            anchors.top: headerView.bottom
            anchors.bottom: parent.bottom
            anchors.topMargin: AppTheme.spacing.small
            syncView: tableView
            clip: true

            delegate: Rectangle {
                id: verticalHeaderDelegate
                required property string display
                required property int index

                readonly property size headerSize: signalDataModel.headerData(verticalHeaderDelegate.index, Qt.Vertical, Qt.SizeHintRole)
                readonly property color signalColor: signalDataModel.data(signalDataModel.index(verticalHeaderDelegate.index, 0), Qt.DecorationRole)
                readonly property bool isSelected: signalDataModel.selectionModel.isRowSelected(verticalHeaderDelegate.index)

                implicitHeight: headerSize.height
                implicitWidth: headerSize.width + 30  // Extra space for checkbox
                color: AppTheme.colors.surfaceElevated
                radius: AppTheme.radius.medium

                Row {
                    anchors.fill: parent
                    anchors.leftMargin: AppTheme.spacing.small
                    anchors.rightMargin: AppTheme.spacing.small
                    spacing: AppTheme.spacing.small

                    // Checkbox for selection toggle
                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        width: 20
                        height: 20
                        radius: 4
                        color: verticalHeaderDelegate.isSelected ? verticalHeaderDelegate.signalColor : AppTheme.colors.surfacePressed
                        border.color: verticalHeaderDelegate.signalColor
                        border.width: 2

                        Behavior on color {
                            ColorAnimation {
                                duration: AppTheme.motion.fast
                            }
                        }

                        // Checkmark icon when selected
                        Text {
                            anchors.centerIn: parent
                            text: "✓"
                            font.pixelSize: 14
                            font.weight: Font.Bold
                            color: verticalHeaderDelegate.isSelected ? AppTheme.colors.textInverse : AppTheme.colors.textTertiary
                            visible: true

                            Behavior on color {
                                ColorAnimation {
                                    duration: AppTheme.motion.fast
                                }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                // Toggle selection in selection model
                                let modelIndex = signalDataModel.index(verticalHeaderDelegate.index, 0);
                                if (verticalHeaderDelegate.isSelected) {
                                    signalDataModel.selectionModel.select(modelIndex, ItemSelectionModel.Deselect | ItemSelectionModel.Rows);
                                } else {
                                    signalDataModel.selectionModel.select(modelIndex, ItemSelectionModel.Select | ItemSelectionModel.Rows);
                                }
                            }
                        }
                    }

                    // Signal name
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: verticalHeaderDelegate.display
                        font.pixelSize: AppTheme.typography.size.medium
                        font.weight: Font.Bold
                        font.family: AppTheme.typography.fontFamily
                        color: AppTheme.colors.textSecondary
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignLeft
                        elide: Text.ElideRight
                    }
                }
            }
        }

        // Main table view - automatically uses model's rowCount() and columnCount()
        TableView {
            id: tableView
            anchors.left: verticalHeaderView.right
            anchors.top: headerView.bottom
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.leftMargin: AppTheme.spacing.small
            anchors.topMargin: AppTheme.spacing.small
            clip: true

            rowSpacing: 4
            columnSpacing: 4

            model: signalDataModel

            columnWidthProvider: function (column) {
                var size = signalDataModel.headerData(column, Qt.Horizontal, Qt.SizeHintRole);
                return size.width;
            }

            rowHeightProvider: function (row) {
                var size = signalDataModel.headerData(row, Qt.Vertical, Qt.SizeHintRole);
                return size.height;
            }

            delegate: Rectangle {
                id: cellDelegate
                required property int row
                required property int column
                required property var model

                readonly property var signalData: signalDataModel.data(signalDataModel.index(cellDelegate.row, 0), Qt.UserRole)
                readonly property color backgroundColor: cellDelegate.model.background || "transparent"
                readonly property color decorationColor: cellDelegate.model.decoration || "transparent"

                // Display text comes directly from model (already computed with effective values)
                color: backgroundColor
                radius: AppTheme.radius.small
                border.color: Qt.rgba(backgroundColor.r, backgroundColor.g, backgroundColor.b, 3.33)
                border.width: AppTheme.border.thin

                Text {
                    anchors.fill: parent
                    anchors.leftMargin: AppTheme.spacing.small
                    anchors.rightMargin: AppTheme.spacing.small
                    text: cellDelegate.model.display
                    font.pixelSize: AppTheme.typography.size.normal
                    font.weight: Font.Medium
                    font.family: AppTheme.typography.fontFamilyMonospace
                    color: cellDelegate.model.foreground || AppTheme.colors.textInverse
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignRight
                    elide: Text.ElideRight
                }
            }
        }
    }
}
