pragma ComponentBehavior: Bound
import QtQuick

// "Dumb" waveform plot view - knows nothing about signal count or types
// Just iterates over the provided model and renders each signal
Rectangle {
    id: root
    color: "#0a0e14"

    required property ApplicationDataModel appDataModel

    readonly property int pointsPerCycle: 40
    readonly property int cycleCount: 2
    readonly property real chartWidth: width - 140
    readonly property real chartHeight: height - 155
    readonly property real chartX: 80
    readonly property real chartY: 75

    function dataToScreenX(t) {
        return chartX + (t / cycleCount) * chartWidth;
    }

    function dataToScreenY(y) {
        return chartY + chartHeight / 2 - (y / 1.2) * (chartHeight / 2);
    }

    // Grid - horizontal lines
    Repeater {
        model: 9
        delegate: Rectangle {
            required property int index
            property real yValue: -1.0 + index * 0.25
            x: root.chartX
            y: root.dataToScreenY(yValue)
            width: root.chartWidth
            height: (index === 4) ? 2.5 : 1
            color: (index === 4) ? "#3d5a80" : "#1b2838"
        }
    }

    // Grid - vertical lines
    Repeater {
        model: root.cycleCount * 4 + 1
        delegate: Rectangle {
            required property int index
            property real xValue: index * 0.25
            x: root.dataToScreenX(xValue)
            y: root.chartY
            width: (index % 4 === 0) ? 1.5 : 0.8
            height: root.chartHeight
            color: (index % 4 === 0) ? "#2a3f5f" : "#1b2838"
        }
    }

    // Single canvas for all waveforms
    Canvas {
        id: waveformCanvas
        x: root.chartX
        y: root.chartY
        width: root.chartWidth
        height: root.chartHeight

        property bool needsRepaint: false

        Connections {
            target: root.appDataModel
            function onDataUpdated() {
                waveformCanvas.needsRepaint = true;
            }
        }

        Timer {
            interval: 33  // ~30 fps
            running: true
            repeat: true
            onTriggered: {
                if (waveformCanvas.needsRepaint) {
                    waveformCanvas.requestPaint();
                    waveformCanvas.needsRepaint = false;
                }
            }
        }

        onPaint: {
            let ctx = getContext("2d");
            ctx.clearRect(0, 0, width, height);

            // Draw all waveforms from signal model
            let signalCount = root.appDataModel.signalDataModel.rowCount();
            for (let signalIndex = 0; signalIndex < signalCount; signalIndex++) {
                let signal = root.appDataModel.signalDataModel.data(
                    root.appDataModel.signalDataModel.index(signalIndex, 0),
                    SignalDataModel.SignalDataRole
                );

                if (!signal) continue;

                let magnitude = signal.magnitude;
                let phase = signal.phase;
                let color = signal.color;
                let normalizedMag = signal.normalizedMagnitude;

                let phaseRad = phase * Math.PI / 180.0;

                ctx.strokeStyle = color;
                ctx.lineWidth = 2.5;
                ctx.lineCap = "round";
                ctx.lineJoin = "round";

                ctx.beginPath();

                let startX = 0;
                let startY = root.dataToScreenY(normalizedMag * Math.sin(phaseRad)) - root.chartY;
                ctx.moveTo(startX, startY);

                for (let i = 1; i <= root.pointsPerCycle * root.cycleCount; i++) {
                    let t = i / root.pointsPerCycle;
                    let y = normalizedMag * Math.sin(2.0 * Math.PI * t + phaseRad);
                    let canvasX = root.dataToScreenX(t) - root.chartX;
                    let canvasY = root.dataToScreenY(y) - root.chartY;
                    ctx.lineTo(canvasX, canvasY);
                }

                ctx.stroke();
            }
        }
    }

    // Y-axis labels
    Repeater {
        model: [1.0, 0.75, 0.5, 0.25, 0, -0.25, -0.5, -0.75, -1.0]
        delegate: Text {
            required property var modelData
            x: 15
            y: root.dataToScreenY(modelData) - height / 2
            text: modelData.toFixed(2)
            font.pixelSize: 11
            font.weight: Font.Medium
            font.family: "monospace"
            color: "#6b8cae"
            horizontalAlignment: Text.AlignRight
            width: 50
        }
    }

    // X-axis labels
    Row {
        anchors.top: parent.bottom
        anchors.topMargin: -50
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.horizontalCenterOffset: 10
        spacing: root.chartWidth / (root.cycleCount * 4)

        Repeater {
            model: root.cycleCount * 4 + 1
            delegate: Text {
                required property int index
                text: (index * 0.25).toFixed(2)
                font.pixelSize: 11
                font.weight: Font.Medium
                font.family: "monospace"
                color: "#6b8cae"
            }
        }
    }

    // Y-axis label
    Text {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 15
        text: "Normalized Amplitude"
        font.pixelSize: 13
        font.weight: Font.DemiBold
        color: "#8ba3be"
        font.family: "sans-serif"
        rotation: -90
        transformOrigin: Item.Center
    }

    // X-axis label
    Text {
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: 15
        text: "Cycles"
        font.pixelSize: 13
        font.weight: Font.DemiBold
        color: "#8ba3be"
        font.family: "sans-serif"
    }
}
