pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

// Signal data table using TableView with QAbstractItemModel
Rectangle {
    id: root
    color: "#0a0e14"

    required property ApplicationDataModel appDataModel

    Item {
        id: tableContainer
        anchors.fill: parent
        anchors.margins: 15

        // Top-left corner spacer (aligns with vertical header)
        Rectangle {
            id: cornerItem
            anchors.left: parent.left
            anchors.top: parent.top
            width: 60  // Match the vertical header width from model
            height: 40
            color: "#1b2838"
            radius: 8
            z: 2
        }

        // Horizontal header view - automatically syncs with TableView
        HorizontalHeaderView {
            id: headerView
            anchors.left: cornerItem.right
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.leftMargin: 8
            syncView: tableView
            clip: true

            delegate: Rectangle {
                id: horizontalHeaderDelegate
                required property string display
                required property int index

                readonly property size headerSize: root.appDataModel.signalDataModel.headerData(horizontalHeaderDelegate.index, Qt.Horizontal, Qt.SizeHintRole)
                implicitWidth: headerSize.width
                implicitHeight: 40
                color: "#1b2838"
                radius: 8

                Text {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    text: horizontalHeaderDelegate.display
                    font.pixelSize: 12
                    font.weight: Font.Bold
                    font.family: "SF Pro Text, Segoe UI, sans-serif"
                    color: "#8ba3be"
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignLeft
                }
            }
        }

        // Vertical header view
        VerticalHeaderView {
            id: verticalHeaderView
            anchors.left: parent.left
            anchors.top: headerView.bottom
            anchors.bottom: parent.bottom
            anchors.topMargin: 8
            syncView: tableView
            clip: true

            delegate: Rectangle {
                id: verticalHeaderDelegate
                required property string display
                required property int index

                readonly property size headerSize: root.appDataModel.signalDataModel.headerData(verticalHeaderDelegate.index, Qt.Vertical, Qt.SizeHintRole)
                implicitHeight: headerSize.height
                implicitWidth: headerSize.width
                color: "#1b2838"
                radius: 8

                Text {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    text: verticalHeaderDelegate.display
                    font.pixelSize: 13
                    font.weight: Font.Bold
                    font.family: "SF Pro Text, Segoe UI, sans-serif"
                    color: "#8ba3be"
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignLeft
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
            anchors.leftMargin: 8
            anchors.topMargin: 8
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
                radius: 6
                border.color: Qt.rgba(backgroundColor.r, backgroundColor.g, backgroundColor.b, 3.33)
                border.width: 1.5

                Text {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    text: cellDelegate.model.display
                    font.pixelSize: 15
                    font.weight: Font.Medium
                    font.family: "SF Mono, Consolas, monospace"
                    color: cellDelegate.model.foreground || "#ffffff"
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignRight
                }
            }
        }
    }
}
