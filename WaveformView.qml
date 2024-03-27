import QtQuick 2.0
import QtCharts 2.0

ChartView {
    id: chart
    visible: true
    anchors.fill: parent
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
        max: 0.1
        tickCount: 19
        titleText: "Time (s)"
    }

    ValueAxis {
        id: axisVoltage
        min: -100
        max: +100
        tickCount: 9
        titleText: "Voltage (V)"
    }
    ValueAxis {
        id: axisCurrent
        min: -20
        max: +20
        tickCount: 9
        titleText: "Current (I)"
    }
}
