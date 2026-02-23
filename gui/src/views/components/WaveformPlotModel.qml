pragma ComponentBehavior: Bound
import QtQuick
import qpmu

// "Dumb" waveform plot view - knows nothing about signal count or types
// Just iterates over the provided model and renders each signal
Rectangle {
    id: root

    color: AppTheme.colors.surface

    required property ApplicationDataModel appDataModel

    property bool showPeakValues: false  // Default to RMS (false)

    readonly property int pointsPerCycle: 40
    readonly property int cycleCount: 1

    // Proportional layout - use percentages of width/height
    readonly property real chartWidth: width * 0.78   // was: width - 200
    readonly property real chartHeight: height * 0.73 // was: height - 155
    readonly property real chartX: width * 0.08       // was: 80
    readonly property real chartY: height * 0.11      // was: 75

    // Spacing constants for uniform layout
    readonly property real tickLabelGap: AppTheme.spacing.small
    readonly property real axisLabelOffset: 60
    readonly property real axisPadding: AppTheme.spacing.small

    readonly property real peakScaleFactor: showPeakValues ? Math.SQRT2 : 1.0
    readonly property real maxVoltage: getMaxMagnitude("Voltage") * peakScaleFactor
    readonly property real maxCurrent: getMaxMagnitude("Current") * peakScaleFactor

    function getMaxMagnitude(signalType) {
        let max = 0.0;
        let signalCount = appDataModel.signalDataModel.rowCount();
        for (let i = 0; i < signalCount; i++) {
            let signal = appDataModel.signalDataModel.data(appDataModel.signalDataModel.index(i, 0), SignalDataModel.SignalDataRole);
            if (signal && signal.signalType === signalType) {
                max = Math.max(max, signal.magnitude);
            }
        }
        return max || 1.0; // Fallback to 1.0 to avoid division by zero
    }

    function dataToScreenX(t) {
        return chartX + (t / cycleCount) * chartWidth;
    }

    function dataToScreenY(y, signalType) {
        let maxValue = (signalType === "Voltage") ? maxVoltage : maxCurrent;
        // Scale by 0.5 to occupy 50% of vertical space
        let normalizedY = (y / maxValue) * 0.5;
        return chartY + chartHeight / 2 - normalizedY * chartHeight;
    }

    // Grid - horizontal lines
    Repeater {
        model: 9
        delegate: Rectangle {
            required property int index
            // Use voltage range for grid (since it's the reference)
            property real yValue: -root.maxVoltage + index * (root.maxVoltage * 2.0 / 8.0)
            x: root.chartX - root.axisPadding
            y: root.dataToScreenY(yValue, "Voltage")
            width: root.chartWidth + 2 * root.axisPadding
            height: (index === 4) ? AppTheme.border.thick : AppTheme.border.thin
            color: (index === 4) ? AppTheme.colors.borderEmphasized : AppTheme.colors.borderSubtle
        }
    }

    // Grid - vertical lines
    Repeater {
        model: root.cycleCount * 4 + 1
        delegate: Rectangle {
            required property int index
            property real xValue: index * 0.25
            x: root.dataToScreenX(xValue)
            y: root.chartY - root.axisPadding
            width: (index % 4 === 0) ? 1.5 : AppTheme.border.thin
            height: root.chartHeight + 2 * root.axisPadding
            color: (index % 4 === 0) ? AppTheme.colors.border : AppTheme.colors.borderSubtle
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
                let signal = root.appDataModel.signalDataModel.data(root.appDataModel.signalDataModel.index(signalIndex, 0), SignalDataModel.SignalDataRole);

                if (!signal)
                    continue;

                let magnitude = signal.magnitude;
                let phase = signal.phase;
                let color = signal.color;
                let signalType = signal.signalType;

                let phaseRad = phase * Math.PI / 180.0;
                let scaledMagnitude = magnitude * root.peakScaleFactor;

                ctx.strokeStyle = color;
                ctx.lineWidth = 2.5;
                ctx.lineCap = "round";
                ctx.lineJoin = "round";

                ctx.beginPath();

                let startX = 0;
                let startY = root.dataToScreenY(scaledMagnitude * Math.sin(phaseRad), signalType) - root.chartY;
                ctx.moveTo(startX, startY);

                for (let i = 1; i <= root.pointsPerCycle * root.cycleCount; i++) {
                    let t = i / root.pointsPerCycle;
                    let y = scaledMagnitude * Math.sin(2.0 * Math.PI * t + phaseRad);
                    let canvasX = root.dataToScreenX(t) - root.chartX;
                    let canvasY = root.dataToScreenY(y, signalType) - root.chartY;
                    ctx.lineTo(canvasX, canvasY);
                }

                ctx.stroke();
            }
        }
    }

    // Y-axis labels - Voltage (left)
    Repeater {
        model: 9
        delegate: Text {
            required property int index
            property real voltageValue: -root.maxVoltage + index * (root.maxVoltage * 2.0 / 8.0)
            x: root.chartX - root.axisPadding - width - root.tickLabelGap
            y: root.dataToScreenY(voltageValue, "Voltage") - height / 2
            text: voltageValue.toFixed(1)
            font.pixelSize: AppTheme.typography.size.small
            font.weight: Font.Medium
            font.family: AppTheme.typography.fontFamilyMonospace
            color: AppTheme.colors.textTertiary
            horizontalAlignment: Text.AlignRight
        }
    }

    // Y-axis labels - Current (right)
    Repeater {
        model: 9
        delegate: Text {
            required property int index
            property real currentValue: -root.maxCurrent + index * (root.maxCurrent * 2.0 / 8.0)
            x: root.chartX + root.chartWidth + root.axisPadding + root.tickLabelGap
            y: root.dataToScreenY(currentValue, "Current") - height / 2
            text: currentValue.toFixed(2)
            font.pixelSize: AppTheme.typography.size.small
            font.weight: Font.Medium
            font.family: AppTheme.typography.fontFamilyMonospace
            color: AppTheme.colors.textTertiary
            horizontalAlignment: Text.AlignLeft
        }
    }

    // X-axis labels
    Repeater {
        model: root.cycleCount * 4 + 1
        delegate: Text {
            required property int index
            property real xValue: index * 0.25
            x: root.dataToScreenX(xValue) - width / 2
            y: root.chartY + root.chartHeight + root.axisPadding + root.tickLabelGap
            text: xValue.toFixed(2)
            font.pixelSize: AppTheme.typography.size.small
            font.weight: Font.Medium
            font.family: AppTheme.typography.fontFamilyMonospace
            color: AppTheme.colors.textTertiary
        }
    }

    // Y-axis label - Voltage (left)
    Text {
        x: root.chartX - root.axisLabelOffset
        y: root.chartY + root.chartHeight / 2
        text: "Voltage (V)"
        font.pixelSize: AppTheme.typography.size.small
        font.weight: Font.DemiBold
        color: AppTheme.colors.textSecondary
        font.family: AppTheme.typography.fontFamily
        rotation: -90
        transformOrigin: Item.Center
    }

    // Y-axis label - Current (right)
    Text {
        x: root.chartX + root.chartWidth + root.axisLabelOffset
        y: root.chartY + root.chartHeight / 2
        text: "Current (A)"
        font.pixelSize: AppTheme.typography.size.small
        font.weight: Font.DemiBold
        color: AppTheme.colors.textSecondary
        font.family: AppTheme.typography.fontFamily
        rotation: 90
        transformOrigin: Item.Center
    }

    // X-axis label
    Text {
        x: root.chartX + root.chartWidth / 2 - width / 2
        y: root.chartY + root.chartHeight + root.axisPadding + 35
        text: "Cycles"
        font.pixelSize: AppTheme.typography.size.small
        font.weight: Font.DemiBold
        color: AppTheme.colors.textSecondary
        font.family: AppTheme.typography.fontFamily
    }

    // RMS/Peak toggle button
    Rectangle {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: AppTheme.spacing.small
        width: 120
        height: 30
        color: AppTheme.colors.surfaceElevated
        border.color: AppTheme.colors.borderEmphasized
        border.width: AppTheme.border.thin
        radius: AppTheme.radius.small

        Text {
            anchors.centerIn: parent
            text: root.showPeakValues ? "Peak" : "RMS"
            font.pixelSize: AppTheme.typography.size.small
            font.weight: Font.Medium
            color: AppTheme.colors.textSecondary
        }

        MouseArea {
            anchors.fill: parent
            onClicked: root.showPeakValues = !root.showPeakValues
            cursorShape: Qt.PointingHandCursor
        }
    }
}
