import QtQuick 2.12
import QtQuick.Controls 2.12
import qpmu 1.0

Rectangle {
    id: root
    color: AppTheme.colors.surface

    // Custom horizontal header (Qt 5.12 compatible - HorizontalHeaderView is Qt 5.15+)
    Row {
        id: horizontalHeader
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: verticalHeader.width
        height: 30
        z: 2
        clip: true

        Repeater {
            model: signalDataModel ? signalDataModel.columnCount() : 0
            delegate: Rectangle {
                property int index
                width: 150  // Fixed width for Qt 5.12 compatibility
                height: 30
                color: AppTheme.colors.surfaceElevated
                border.width: AppTheme.border.thin
                border.color: AppTheme.colors.border

                Text {
                    anchors.centerIn: parent
                    text: signalDataModel ? signalDataModel.headerData(index, Qt.Horizontal, Qt.DisplayRole) : ""
                    color: AppTheme.colors.textPrimary
                    font.pixelSize: AppTheme.typography.size.small
                    font.weight: Font.DemiBold
                    font.family: AppTheme.typography.fontFamily
                }
            }
        }
    }

    // Custom vertical header (Qt 5.12 compatible - VerticalHeaderView is Qt 5.15+)
    Column {
        id: verticalHeader
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: horizontalHeader.height
        width: 60
        z: 2
        clip: true

        Repeater {
            model: signalDataModel ? signalDataModel.rowCount() : 0
            delegate: Rectangle {
                property int index
                width: 60
                height: 35  // Fixed height for Qt 5.12 compatibility
                color: AppTheme.colors.surfaceElevated
                border.width: AppTheme.border.thin
                border.color: AppTheme.colors.border

                Text {
                    anchors.centerIn: parent
                    text: signalDataModel ? signalDataModel.headerData(index, Qt.Vertical, Qt.DisplayRole) : ""
                    color: AppTheme.colors.textPrimary
                    font.pixelSize: AppTheme.typography.size.small
                    font.weight: Font.Medium
                    font.family: AppTheme.typography.fontFamily
                }
            }
        }
    }

    TableView {
        id: tableView
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: verticalHeader.width
        anchors.topMargin: horizontalHeader.height
        clip: true

        columnSpacing: 1
        rowSpacing: 1

        model: signalDataModel

        // Note: selectionModel is Qt 6 only - Qt 5.12/5.15 TableView doesn't support it
        // In Qt 5, selection still works via the model's selection model,
        // but TableView doesn't visually indicate selection
        Component.onCompleted: {
            if (tableView.selectionModel !== undefined) {
                tableView.selectionModel = signalDataModel.selectionModel;
            }
        }

        // Qt 5.12 compatible: use fixed sizes
        columnWidthProvider: function (column) {
            return 150;  // Fixed width
        }

        rowHeightProvider: function (row) {
            return 35;  // Fixed height
        }

        delegate: Item {
            id: delegateItem
            property string display
            property color decoration
            property bool selected: false  // Qt 5: not provided by TableView, defaults to false
            property int row
            property int column

            implicitWidth: cellRect.implicitWidth
            implicitHeight: cellRect.implicitHeight

            Rectangle {
                id: cellRect
                anchors.fill: parent

                implicitWidth: text.implicitWidth
                implicitHeight: text.implicitHeight
                color: delegateItem.selected ? AppTheme.withAlpha(delegateItem.decoration, 0.3) : AppTheme.withAlpha(delegateItem.decoration, 0.1)

                // Apply border to entire row by checking if we're at the edges
                border.width: delegateItem.selected ? 2 : 0
                border.color: delegateItem.decoration

                Text {
                    id: text
                    text: delegateItem.display

                    anchors.centerIn: parent
                    anchors.fill: parent
                    anchors.leftMargin: AppTheme.spacing.small
                    anchors.rightMargin: AppTheme.spacing.small

                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter

                    color: delegateItem.selected ? AppTheme.colors.textPrimary : delegateItem.decoration

                    font.family: AppTheme.typography.fontFamilyMonospace
                    font.pointSize: AppTheme.typography.size.normal
                    font.weight: delegateItem.selected ? Font.DemiBold : Font.Normal
                }
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    const modelIndex = tableView.model.index(delegateItem.row, delegateItem.column);
                    tableView.model.selectionModel.select(modelIndex, ItemSelectionModel.ToggleCurrent | ItemSelectionModel.Rows);
                }
            }
        }
    }
}
