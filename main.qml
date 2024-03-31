import QtQuick 2.0
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0
import QtQml.Models 2.1

ApplicationWindow {
    visible: true
    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            ToolButton {
                text: "‹"
                onClicked: stack.pop()
            }
            Label {
                text: "PMU"
                elide: Label.ElideRight
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
                Layout.fillWidth: true
            }
            ToolButton {
                text: "⋮"
                onClicked: menu.open()
            }
        }
    }

    ListModel {
        id: options
        ListElement {
            label: "Waveform"
            componentUrl: "Monitor.qml"
            iconUrl: "images/wave-graph.png"
            properties: "{\"dataType\":\"waveform\"}"
        }
        ListElement {
            label: "Phasors"
            componentUrl: "Monitor.qml"
            iconUrl: "images/polar-chart.png"
            properties: "{\"dataType\":\"phasor\"}"
        }
    }
    Monitor {
        anchors.fill: parent
        dataType: "wave"
    }

    //    StackView {
    //        id: stack
    //        anchors.fill: parent
    //        initialItem: GridView {
    //            id: grid
    //            cellWidth: parent.width * 0.2
    //            cellHeight: cellWidth
    //            leftMargin: 10
    //            topMargin: 10
    //            rightMargin: 10
    //            bottomMargin: 10

    //            model: options
    //            delegate: Rectangle {
    //                width: grid.cellWidth * 0.7
    //                height: grid.cellHeight * 0.7
    //                Image {
    //                    anchors.fill: parent
    //                    source: model.iconUrl
    //                }
    //                Text {
    //                    anchors.top: parent.bottom
    //                    anchors.horizontalCenter: parent.horizontalCenter
    //                    text: model.label
    //                }
    //                MouseArea {
    //                    anchors.fill: parent
    //                    onClicked: {

    //                        const comp = Qt.createComponent(model.componentUrl)
    //                        const object = comp.createObject()
    //                        stack.push(object, JSON.parse(model.properties))
    //                    }
    //                }
    //            }
    //        }
    //    }
}
