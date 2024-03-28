import QtQuick 2.0
import QtCharts 2.0

ChartView {
    id: chart
    visible: true
    anchors.fill: parent
    antialiasing: true
    property alias axisTime: axisTime
    property alias axisVoltage: axisVoltage
    property alias axisCurrent: axisCurrent
    //    DateTimeAxis {
    //        id: axisTime
    //        format: "hh:mm:ss.zzz"
    //        min: new Date()
    //        max: new Date()
    //    }
    ValueAxis {
        id: axisTime
        min: 0
        max: 100
        tickCount: 21
        titleText: "Time (ms)"
    }

    ValueAxis {
        id: axisVoltage
        min: -100
        max: +100
        tickCount: 11
        titleText: "Voltage (V)"
    }
    ValueAxis {
        id: axisCurrent
        min: -20
        max: +20
        tickCount: 11
        titleText: "Current (A)"
    }
}
