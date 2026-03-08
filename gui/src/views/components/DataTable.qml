pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import qpmu

Rectangle {
    id: root
    color: AppTheme.colors.surface

    HorizontalHeaderView {
        id: horizontalHeader
        anchors.left: tableView.left
        anchors.top: parent.top
        syncView: tableView
        clip: true
    }

    VerticalHeaderView {
        id: verticalHeader
        anchors.top: tableView.top
        anchors.left: parent.left
        syncView: tableView
        clip: true
    }

    TableView {
        id: tableView
        anchors.left: verticalHeader.right
        anchors.top: horizontalHeader.bottom
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        clip: true

        columnSpacing: 1
        rowSpacing: 1

        model: AppData.signalDataModel

        selectionModel: AppData.signalDataModel.selectionModel

        // Widths can grow but never shrink, so we cache the max width for each column
        property var maxWidthPerColumn: []
        columnWidthProvider: function (column) {
            let cellWidth = implicitColumnWidth(column);
            let headerWidth = horizontalHeader.implicitColumnWidth(column);
            let width = Math.max(cellWidth, headerWidth) + AppTheme.spacing.small * 2;
            return maxWidthPerColumn[column] = Math.max(maxWidthPerColumn[column] || 0, width);
        }

        // Similar to columnMaxWidth, but we need to track the max height across all rows for consistent row heights
        property real maxHeightAllRows: 0
        rowHeightProvider: function (row) {
            let cellHeight = implicitRowHeight(row);
            let headerHeight = verticalHeader.implicitRowHeight(row);
            let height = Math.max(cellHeight, headerHeight) + AppTheme.spacing.small * 2;
            return maxHeightAllRows = Math.max(maxHeightAllRows, height);
        }

        delegate: Item {
            id: delegateItem
            required property string display
            required property color decoration
            required property bool selected
            required property int row
            required property int column

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
                    tableView.selectionModel.select(modelIndex, ItemSelectionModel.ToggleCurrent | ItemSelectionModel.Rows);
                }
            }
        }
    }
}
