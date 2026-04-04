import QtQuick 2.12
import qpmu 1.0

// Styled section header for control panel
Rectangle {
    id: root

    property string title: ""

    implicitWidth: parent ? parent.width : 200
    implicitHeight: AppTheme.spacing.huge

    color: "transparent"

    Text {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: AppTheme.spacing.small
        text: root.title
        font.pixelSize: AppTheme.typography.size.small
        font.weight: Font.Bold
        font.family: AppTheme.typography.fontFamily
        color: AppTheme.colors.textTertiary
        elide: Text.ElideRight
    }

    Rectangle {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: AppTheme.spacing.small
        height: AppTheme.border.thin
        color: AppTheme.colors.borderSubtle
    }
}
