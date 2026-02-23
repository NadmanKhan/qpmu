pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import qpmu

// Signal data table using TableView with QAbstractItemModel
Rectangle {
    id: root

    color: AppTheme.colors.surface

    required property ApplicationDataModel appDataModel

    Item {
        id: tableContainer
        anchors.fill: parent
        anchors.margins: AppTheme.spacing.medium

        // Top-left corner spacer (aligns with vertical header)
        Rectangle {
            id: cornerItem
            anchors.left: parent.left
            anchors.top: parent.top
            width: 60
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

                readonly property size headerSize: root.appDataModel.signalDataModel.headerData(horizontalHeaderDelegate.index, Qt.Horizontal, Qt.SizeHintRole)
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

        // Vertical header view
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

                readonly property size headerSize: root.appDataModel.signalDataModel.headerData(verticalHeaderDelegate.index, Qt.Vertical, Qt.SizeHintRole)
                implicitHeight: headerSize.height
                implicitWidth: headerSize.width
                color: AppTheme.colors.surfaceElevated
                radius: AppTheme.radius.medium

                Text {
                    anchors.fill: parent
                    anchors.leftMargin: AppTheme.spacing.small
                    anchors.rightMargin: AppTheme.spacing.small
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

            model: root.appDataModel.signalDataModel

            columnWidthProvider: function (column) {
                var size = root.appDataModel.signalDataModel.headerData(column, Qt.Horizontal, Qt.SizeHintRole);
                return size.width;
            }

            rowHeightProvider: function (row) {
                var size = root.appDataModel.signalDataModel.headerData(row, Qt.Vertical, Qt.SizeHintRole);
                return size.height;
            }

            delegate: Rectangle {
                id: cellDelegate
                required property int row
                required property int column
                required property var model

                readonly property color backgroundColor: cellDelegate.model.background || "transparent"
                readonly property color decorationColor: cellDelegate.model.decoration || "transparent"

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
