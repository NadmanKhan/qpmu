import QtQuick 2.0
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0
import QtCharts 2.13
import QtQml.Models 2.1
import QtCharts 2.0

ApplicationWindow {
    visible: true
    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            ToolButton {
                text: qsTr("‹")
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
                text: qsTr("⋮")
                onClicked: menu.open()
            }
        }
    }

    StackView {
        anchors.fill: parent
        initialItem: Monitor {
            dataType: "waveform"
        }
    }

    //    StackView {
    //        anchors.fill: parent
    //        initialItem: GridView {
    //            id: grid
    //            anchors.fill: parent
    //            cellWidth: 150
    //            cellHeight: cellWidth
    //            model: ListModel {
    //                ListElement {
    //                    title: "Monitor Waveform"
    //                    url: Qt.createComponent()
    //                }
    //            }
    //            delegate: delegate
    //        }
    //    }

    //    StackView {
    //        id: stack
    //        anchors.fill: parent
    //        initialItem: Grid {
    //            id: grid
    //            anchors.fill: parent
    //            columns: 3

    //            Rectangle {
    //                Image {
    //                    anchors.fill: parent
    //                    source: "icons.qrc:/images/wave-graph.png"
    //                }

    //                Text {
    //                    anchors.top: parent.bottom
    //                    anchors.horizontalCenter: parent.horizontalCenter
    //                    text: "Monitor Waveform"
    //                }

    //                MouseArea {
    //                    anchors.fill: parent
    //                    onClicked: {
    //                        console.log("Clicked waveform")
    //                        const component = Qt.createComponent('qrc:/Monitor.qml')
    //                        const object = component.createObject(stack, {
    //                                                                  "dataType": "waveform"
    //                                                              })
    //                        stack.push(object)
    //                    }
    //                }
    //            }
    //        }
    //    }
}
