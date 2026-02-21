import QtQuick

pragma ComponentBehavior: Bound

Rectangle {
    id: root
    color: "#0a0e14"  // Deeper dark blue-gray background to match phasor plot

    required property var dataModel

    readonly property int pointsPerCycle: 80  // More points for even smoother curves
    readonly property int cycleCount: 2  // Show 2 cycles
    readonly property real chartWidth: width - 140
    readonly property real chartHeight: height - 155
    readonly property real chartX: 80
    readonly property real chartY: 75

    // Helper functions
    function dataToScreenX(t) {
        return chartX + (t / cycleCount) * chartWidth;
    }

    function dataToScreenY(y) {
        return chartY + chartHeight/2 - (y / 1.2) * (chartHeight / 2);
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
            height: (index === 4) ? 2.5 : 1  // Thicker zero line
            color: (index === 4) ? "#3d5a80" : "#1b2838"
        }
    }

    // Grid - vertical lines (cycle markers)
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

    // Waveforms for all 6 signals using Canvas
    Repeater {
        model: 6
        delegate: Canvas {
            id: waveformCanvas
            anchors.fill: parent
            layer.enabled: true
            layer.samples: 4

            required property int index
            property real magnitude: root.dataModel.phasorMagnitudes[index]
            property real phase: root.dataModel.phasorPhases[index]
            property color signalColor: root.dataModel.signalColors[index]
            property real normalizedMag: index < 3 ? magnitude / 300.0 : magnitude / 20.0

            Connections {
                target: root.dataModel
                function onDataUpdated() { waveformCanvas.requestPaint(); }
            }

            onPaint: {
                let ctx = getContext("2d");
                ctx.clearRect(0, 0, width, height);

                let phaseRad = phase * Math.PI / 180.0;

                ctx.strokeStyle = signalColor;
                ctx.lineWidth = 2.8;
                ctx.lineCap = "round";
                ctx.lineJoin = "round";
                ctx.shadowBlur = 10;
                ctx.shadowColor = signalColor;

                ctx.beginPath();
                ctx.moveTo(root.dataToScreenX(0), root.dataToScreenY(normalizedMag * Math.sin(phaseRad)));

                for (let i = 1; i <= root.pointsPerCycle * root.cycleCount; i++) {
                    let t = i / root.pointsPerCycle;
                    let y = normalizedMag * Math.sin(2.0 * Math.PI * t + phaseRad);
                    ctx.lineTo(root.dataToScreenX(t), root.dataToScreenY(y));
                }

                ctx.stroke();
                ctx.shadowBlur = 0;
            }
        }
    }

    // Y-axis labels - positioned at actual grid line locations
    Repeater {
        model: [1.0, 0.75, 0.5, 0.25, 0, -0.25, -0.5, -0.75, -1.0]
        delegate: Text {
            required property var modelData
            x: 15
            y: root.dataToScreenY(modelData) - height / 2
            text: modelData.toFixed(2)
            font.pixelSize: 11
            font.weight: Font.Medium
            font.family: "SF Mono, Consolas, monospace"
            color: "#6b8cae"
            horizontalAlignment: Text.AlignRight
            width: 50
        }
    }

    // X-axis labels (cycles)
    Row {
        anchors.top: parent.bottom
        anchors.topMargin: -50
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.horizontalCenterOffset: 10
        spacing: root.chartWidth / (cycleCount * 4)

        Repeater {
            model: root.cycleCount * 4 + 1
            delegate: Text {
                required property int index
                text: (index * 0.25).toFixed(2)
                font.pixelSize: 11
                font.weight: Font.Medium
                font.family: "SF Mono, Consolas, monospace"
                color: "#6b8cae"
            }
        }
    }

    // Y-axis label (rotated)
    Text {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 15
        text: "Normalized Amplitude"
        font.pixelSize: 13
        font.weight: Font.DemiBold
        color: "#8ba3be"
        font.family: "SF Pro Text, Segoe UI, sans-serif"
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
        font.family: "SF Pro Text, Segoe UI, sans-serif"
    }

    // Legend
    Row {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 20
        spacing: 18

        Repeater {
            model: 6
            delegate: Row {
                id: legendItem
                required property int index
                spacing: 6
                Rectangle {
                    width: 24
                    height: 4
                    radius: 2
                    color: root.dataModel.signalColors[legendItem.index]
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: root.dataModel.signalNames[legendItem.index]
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                    font.family: "SF Mono, Consolas, monospace"
                    color: root.dataModel.signalColors[legendItem.index]
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
    }

    // Title with better styling
    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 20
        width: titleText.width + 24
        height: titleText.height + 12
        color: "#0a0e1499"
        radius: 8
        border.color: "#3d5a80"
        border.width: 1.5

        Text {
            id: titleText
            anchors.centerIn: parent
            text: "Waveform View"
            font.pixelSize: 17
            font.weight: Font.DemiBold
            color: "#d0dae8"
            font.family: "SF Pro Display, Segoe UI, sans-serif"
        }
    }
}
