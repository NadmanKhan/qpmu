import QtQuick 2.0
import QtCharts 2.0

PolarChartView {
    id: chart
    visible: true
    anchors.fill: parent
    antialiasing: true
    property alias axisAngular: axisAngular
    property alias axisRadial: axisRadial
    CategoryAxis {
        id: axisAngular
        startValue: 0
        min: 0
        max: 360
        labelsPosition: CategoryAxis.AxisLabelsPositionOnValue
        CategoryRange {
            label: "0°"
            endValue: 90
        }
        CategoryRange {
            label: "30°"
            endValue: 60
        }
        CategoryRange {
            label: "60°"
            endValue: 30
        }
        CategoryRange {
            label: "90°"
            endValue: 0
        }
        CategoryRange {
            label: "120°"
            endValue: 330
        }
        CategoryRange {
            label: "150°"
            endValue: 300
        }
        CategoryRange {
            label: "180°"
            endValue: 270
        }
        CategoryRange {
            label: "-150°"
            endValue: 240
        }
        CategoryRange {
            label: "-120°"
            endValue: 210
        }
        CategoryRange {
            label: "-90°"
            endValue: 180
        }
        CategoryRange {
            label: "-60°"
            endValue: 150
        }
        CategoryRange {
            label: "-30°"
            endValue: 120
        }
    }
    ValueAxis {
        id: axisRadial
        min: 0
        max: 1
        labelsVisible: false
    }
}
