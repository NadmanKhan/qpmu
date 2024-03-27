// GridMenu.qml

import QtQuick 2.0
import QtQuick.Controls 2.0

Rectangle {
    id: gridMenu

    property var menuItems: []

    Component {
        id: delegate

        Rectangle {
            width: grid.cellWidth
            height: grid.cellHeight

            Rectangle {
                width: parent.width - 30
                height: parent.height - 30
                color: "lightgray"
                radius: 10

                Image {
                    anchors.fill: parent
                    source: menuItems[index].icon
                }

                Text {
                    anchors.top: parent.bottom
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: menuItems[index].name
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        console.log("Clicked:", menuItems[index].name)
                        const component = Qt.createComponent('qrc:/main.qml')
                        component.createObject(gridMenu)
                    }
                }
            }
        }
    }

    GridView {
        id: grid
        anchors.fill: parent
        cellWidth: 150
        cellHeight: cellWidth

        model: menuItems
        delegate: delegate
    }
}
