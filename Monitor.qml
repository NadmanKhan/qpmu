import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtCharts 2.1
import QtQuick.Controls 2.0
import QtQml.Models 2.1

Row {
    property string dataType
    property ChartView chart
    property ValueAxis axisTime: null
    property ValueAxis axisVoltage: null
    property ValueAxis axisCurrent: null
    property ValueAxis axisAngular: null
    property ValueAxis axisRadial: null
    property var seriesList: []

    ListModel {
        id: seriesInfoListModel
        dynamicRoles: true
    }

    Rectangle {
        color: "lightgray"
        id: controlPanel
        width: parent.width * 0.2
        height: parent.height
        Column {
            spacing: 1
            CheckBox {
                id: allVoltagesCheck
                text: "All voltages"
                checked: true
                onCheckedChanged: {
                    for (var i = 0; i < seriesInfoListModel.count; ++i) {
                        let model = seriesInfoListModel.get(i)
                        if (model.type === "voltage") {
                            model.visible = checked
                        }
                    }
                }
            }

            CheckBox {
                id: allCurrentsCheck
                text: "All currents"
                checked: true
                onCheckedChanged: {
                    for (var i = 0; i < seriesInfoListModel.count; ++i) {
                        let model = seriesInfoListModel.get(i)
                        if (model.type === "current") {
                            model.visible = checked
                        }
                    }
                }
            }

            Repeater {
                model: seriesInfoListModel
                CheckBox {
                    text: model.name
                    checked: model.visible
                    onCheckedChanged: model.visible = checked
                }
            }
        }
    }

    Loader {
        id: chartLoader
        width: parent.width - controlPanel.width
        height: parent.height
        source: (dataType === "phasor") ? "PhasorView.qml" : "WaveformView.qml"
        onLoaded: {
            chart = chartLoader.item
            if (dataType === "phasor") {
                axisAngular = chart.axisAngular
                axisRadial = chart.axisRadial
            } else {
                axisTime = chart.axisTime
                axisVoltage = chart.axisVoltage
                axisCurrent = chart.axisCurrent
            }
            console.log(chart)
            refreshTimer.start()
        }
    }

    Component.onCompleted: {
        const seriesInfoList = worker.seriesInfoList(dataType)
        console.assert(axisTime != null, "axisTime is null")
        console.assert(axisVoltage != null, "axisVoltage is null")
        console.assert(axisCurrent != null, "axisTime is null")
        for (var i = 0; i < seriesInfoList.length; ++i) {
            const info = seriesInfoList[i]
            seriesInfoListModel.append(info)
            const model = seriesInfoListModel.get(i)
            const series = chart.createSeries(ChartView.SeriesTypeLine,
                                              model.name)
            if (dataType === "phasor") {
                series.axisAngular = axisAngular
                series.axisRadial = axisRadial
            } else {
                series.axisX = axisTime
                if (model.type === "voltage") {
                    series.axisY = axisVoltage
                } else {
                    series.axisYRight = axisCurrent
                }
            }
            series.color = model.color
            series.visible = Qt.binding(function () {
                return model.visible
            })

            seriesList.push(series)
        }
    }

    Timer {
        id: refreshTimer
        interval: 10 // ms
        running: false
        repeat: true
        onTriggered: {
            worker.updatePoints(seriesList, dataType)
            if (dataType !== "phasor") {

                let vMax = 0
                let iMax = 0
                for (var i = 0; i < chart.count; ++i) {
                    const s = chart.series(i)
                    if (seriesInfoListModel.get(i).type === "voltage") {
                        for (var j = 0; j < s.count; ++j) {
                            vMax = Math.max(vMax, Math.abs(s.at(j).y))
                        }
                    } else {
                        for (var j = 0; j < s.count; ++j) {
                            iMax = Math.max(iMax, Math.abs(s.at(j).y))
                        }
                    }
                }
                axisCurrent.min = -iMax
                axisCurrent.max = +iMax
                axisVoltage.min = -vMax
                axisVoltage.max = +vMax
            }
        }
    }
}
