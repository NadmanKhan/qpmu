import QtQuick
import qpmu 1.0

Rectangle {
    id: root

    color: AppTheme.colors.surface

    readonly property int pointsPerCycle: 40
    readonly property int cycleCount: 1

    // Chart layout - percentage of parent dimensions reserved for plotting area
    readonly property real chartAreaWidthRatio: 0.78  // 78% width (22% for axes and labels)
    readonly property real chartAreaHeightRatio: 0.73 // 73% height (27% for axes and labels)

    // Grid configuration
    readonly property int gridRowCount: 9          // Number of horizontal grid lines
    readonly property int gridDivisionsPerCycle: 4 // Vertical grid divisions per cycle

    // Spacing constants for uniform layout
    readonly property real tickLabelGap: AppTheme.spacing.small
    readonly property real axisPadding: AppTheme.spacing.small
    readonly property real tapTargetMinSize: 12
    readonly property real tapTargetMaxSize: 20
    readonly property real tapTargetSize: Math.max(tapTargetMinSize, Math.min(tapTargetMaxSize, chartWidth / 100))
    readonly property real yAxisLabelOffset: AppTheme.spacing.medium + AppTheme.typography.size.small
    readonly property real xAxisLabelOffset: axisPadding + tickLabelGap + AppTheme.typography.size.small * 1.5

    // Styling constants
    readonly property real waveformLineWidth: 2.5
    readonly property real gridLineWidthMajor: AppTheme.border.thick
    readonly property real verticalScaleFactor: 0.5 // Waveforms occupy 50% of vertical space for proper centering

    // Helper constants
    readonly property real degreesToRadians: Math.PI / 180.0

    // Use SignalDataModel cutoff values for scaling
    readonly property real maxVoltage: signalDataModel.voltageCutoff
    readonly property real maxCurrent: signalDataModel.currentCutoff

    // Chart dimensions - referenced by other elements
    readonly property real chartWidth: chartArea.width
    readonly property real chartHeight: chartArea.height
    readonly property real chartX: chartArea.x
    readonly property real chartY: chartArea.y

    // Chart area - anchored in center of root
    Item {
        id: chartArea
        anchors.centerIn: parent
        width: parent.width * root.chartAreaWidthRatio
        height: parent.height * root.chartAreaHeightRatio
    }

    function dataToScreenX(t) {
        return chartX + (t / cycleCount) * chartWidth;
    }

    function dataToScreenY(y, signalType) {
        let maxValue = (signalType === "Voltage") ? maxVoltage : maxCurrent;
        let normalizedY = (y / maxValue) * verticalScaleFactor;
        return chartY + chartHeight / 2 - normalizedY * chartHeight;
    }

    function getGridLineValue(maxValue, index) {
        return -maxValue + index * (maxValue * 2.0 / (gridRowCount - 1));
    }

    // Grid - horizontal lines
    Repeater {
        model: root.gridRowCount
        delegate: Rectangle {
            required property int index
            property real yValue: root.getGridLineValue(root.maxVoltage, index)
            x: root.chartX - root.axisPadding
            y: root.dataToScreenY(yValue, "Voltage")
            width: root.chartWidth + 2 * root.axisPadding
            height: (index === Math.floor(root.gridRowCount / 2)) ? AppTheme.border.thick : AppTheme.border.thin
            color: (index === Math.floor(root.gridRowCount / 2)) ? AppTheme.colors.borderEmphasized : AppTheme.colors.borderSubtle
        }
    }

    // Grid - vertical lines
    Repeater {
        model: root.cycleCount * root.gridDivisionsPerCycle + 1
        delegate: Rectangle {
            required property int index
            property real xValue: index / root.gridDivisionsPerCycle
            x: root.dataToScreenX(xValue)
            y: root.chartY - root.axisPadding
            width: (index % root.gridDivisionsPerCycle === 0) ? root.gridLineWidthMajor : AppTheme.border.thin

            height: root.chartHeight + 2 * root.axisPadding
            color: (index % root.gridDivisionsPerCycle === 0) ? AppTheme.colors.border : AppTheme.colors.borderSubtle
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: signalDataModel.selectionModel.clearSelection()
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
            target: appInstance
            function onDataUpdated() {
                if (!signalDataModel.isPaused) {
                    waveformCanvas.needsRepaint = true;
                }
            }
        }

        Connections {
            target: signalDataModel
            function onMagnitudeModeChanged() {
                waveformCanvas.needsRepaint = true;
            }
            function onPhaseReferenceChanged() {
                waveformCanvas.needsRepaint = true;
            }
            function onDataChanged() {
                waveformCanvas.needsRepaint = true;
            }
            function onVoltageScalingChanged() {
                waveformCanvas.needsRepaint = true;
            }
            function onCurrentScalingChanged() {
                waveformCanvas.needsRepaint = true;
            }
        }

        Connections {
            target: signalDataModel.selectionModel
            function onSelectionChanged() {
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

            // Draw all waveforms from signal model (always visible)
            let signalCount = signalDataModel.rowCount();
            for (let signalIndex = 0; signalIndex < signalCount; signalIndex++) {
                // Get effective magnitude and phase from SignalDataModel (already computed)
                let magnitude = signalDataModel.data(signalDataModel.index(signalIndex, 0), SignalDataModel.MagnitudeRole);
                let phase = signalDataModel.data(signalDataModel.index(signalIndex, 0), SignalDataModel.PhaseAngleRole);
                let color = signalDataModel.data(signalDataModel.index(signalIndex, 0), Qt.DecorationRole);
                let typeSymbol = signalDataModel.data(signalDataModel.index(signalIndex, 0), SignalDataModel.TypeSymbolRole);
                let signalType = typeSymbol === "V" ? "Voltage" : "Current";

                let phaseRad = phase * root.degreesToRadians;
                let scaledMagnitude = magnitude;

                ctx.strokeStyle = color;
                ctx.lineWidth = root.waveformLineWidth;
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
        model: root.gridRowCount
        delegate: Text {
            required property int index
            property real voltageValue: root.getGridLineValue(root.maxVoltage, index)
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
        model: root.gridRowCount
        delegate: Text {
            required property int index
            property real currentValue: root.getGridLineValue(root.maxCurrent, index)
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
        model: root.cycleCount * root.gridDivisionsPerCycle + 1
        delegate: Text {
            required property int index
            property real xValue: index / root.gridDivisionsPerCycle
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
        anchors.right: chartArea.left
        anchors.rightMargin: root.yAxisLabelOffset
        anchors.verticalCenter: chartArea.verticalCenter
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
        anchors.left: chartArea.right
        anchors.leftMargin: root.yAxisLabelOffset
        anchors.verticalCenter: chartArea.verticalCenter
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
        anchors.top: chartArea.bottom
        anchors.topMargin: root.xAxisLabelOffset
        anchors.horizontalCenter: chartArea.horizontalCenter
        text: "Cycles"
        font.pixelSize: AppTheme.typography.size.small
        font.weight: Font.DemiBold
        color: AppTheme.colors.textSecondary
        font.family: AppTheme.typography.fontFamily
    }

    // Invisible tap areas for waveform lines
    Repeater {
        model: signalDataModel

        delegate: Item {
            id: waveformTapArea
            required property int index
            required property real magnitude
            required property real phaseAngle
            required property string typeSymbol

            property string signalType: typeSymbol === "V" ? "Voltage" : "Current"

            x: root.chartX
            y: root.chartY
            width: root.chartWidth
            height: root.chartHeight

            // Create a series of small rectangular tap zones along the waveform
            Repeater {
                model: root.pointsPerCycle * root.cycleCount * 3

                delegate: Item {
                    required property int index

                    property real t: index / (root.pointsPerCycle * root.gridDivisionsPerCycle)
                    property real phaseRad: waveformTapArea.phaseAngle * root.degreesToRadians
                    property real dataY: waveformTapArea.magnitude * Math.sin(2.0 * Math.PI * t + phaseRad)
                    property real screenX: root.dataToScreenX(t)
                    property real screenY: root.dataToScreenY(dataY, waveformTapArea.signalType)
                    property real halfTarget: root.tapTargetSize / 2

                    x: screenX - root.chartX - halfTarget
                    y: screenY - root.chartY - halfTarget
                    width: root.tapTargetSize
                    height: root.tapTargetSize

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            const modelIndex = signalDataModel.index(waveformTapArea.index, 0);
                            signalDataModel.selectionModel.select(modelIndex, ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Rows);
                        }
                    }
                }
            }
        }
    }

    // Waveform tooltips - show when signal is selected (also clickable for selection)
    Repeater {
        model: signalDataModel

        delegate: Rectangle {
            id: labelDelegate

            required property int index
            required property color decoration
            required property real magnitude
            required property real phaseAngle
            required property string typeSymbol
            required property string toolTip

            // Calculate position at the start of the waveform (t=0)
            property string signalType: typeSymbol === "V" ? "Voltage" : "Current"
            property real cutoff: typeSymbol === "V" ? signalDataModel.voltageCutoff : signalDataModel.currentCutoff
            property real normalizedMagnitude: magnitude / cutoff
            property real phaseRad: phaseAngle * root.degreesToRadians
            property real startY: magnitude * Math.sin(phaseRad)
            property real startScreenX: root.dataToScreenX(0)
            property real startScreenY: root.dataToScreenY(startY, signalType)

            // Position tooltip at waveform start
            x: startScreenX + AppTheme.spacing.medium
            y: startScreenY - height / 2

            // Visibility based on selection
            visible: toolTip !== ""

            // Sizing based on content
            width: labelText.width + AppTheme.spacing.small * 2
            height: labelText.height + AppTheme.spacing.small * 2

            // Styling - transparent background with colored border
            color: "transparent"
            radius: AppTheme.radius.small
            border.color: decoration
            border.width: AppTheme.border.thin

            Text {
                id: labelText
                anchors.centerIn: parent
                text: labelDelegate.toolTip
                color: labelDelegate.decoration
                font.pixelSize: AppTheme.typography.size.small
                font.weight: Font.Medium
                font.family: AppTheme.typography.fontFamilyMonospace
            }
        }
    }
}
