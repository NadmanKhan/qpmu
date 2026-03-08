pragma ComponentBehavior: Bound
import QtQml
import QtQuick
import qpmu

// "Dumb" phasor plot view - knows nothing about signal count or types
// Just iterates over the provided model and renders each signal
Rectangle {
    id: root

    color: AppTheme.colors.surface

    // Plot layout - size as percentage of available space
    readonly property real plotRadiusRatio: 0.38  // 38% of smallest dimension (62% for labels/margins)
    readonly property real plotRadius: Math.min(width, height) * plotRadiusRatio
    readonly property real centerX: width / 2
    readonly property real centerY: height / 2

    // Grid configuration
    readonly property var circleRadii: [1.0, 0.75, 0.5, 0.25]  // Concentric circle positions
    readonly property int radialAngleStep: 30  // Degrees between radial lines
    readonly property int majorAngleStep: 90   // Degrees for emphasized radial lines

    // Styling constants
    readonly property real phasorLineWidth: 3
    readonly property real gridLineThin: 1.2
    readonly property real gridLineThick: 2.5
    readonly property real arrowSize: 12
    readonly property real arrowAngleDeg: 20
    readonly property real centerDotRadius: 5

    // Label positioning
    readonly property real angleLabelDistanceRatio: 1.15  // Distance from center as ratio of plotRadius

    // Tap target configuration
    readonly property real tapTargetMinSize: 12
    readonly property real tapTargetMaxSize: 20
    readonly property real tapTargetSize: 16  // Fixed size for phasor plot
    readonly property real tapTargetSpacing: 5  // Pixels between tap targets along phasor

    // Helper constants
    readonly property real degreesToRadians: Math.PI / 180.0

    property bool needsRepaint: false

    Connections {
        target: AppData
        function onDataUpdated() {
            if (!AppData.signalDataModel.isPaused) {
                root.needsRepaint = true;
            }
        }
    }

    Connections {
        target: AppData.signalDataModel
        function onMagnitudeModeChanged() {
            root.needsRepaint = true;
        }
        function onPhaseReferenceChanged() {
            root.needsRepaint = true;
        }
        function onDataChanged() {
            root.needsRepaint = true;
        }
        function onVoltageScalingChanged() {
            root.needsRepaint = true;
        }
        function onCurrentScalingChanged() {
            root.needsRepaint = true;
        }
    }

    Connections {
        target: AppData.signalDataModel.selectionModel
        function onSelectionChanged() {
            root.needsRepaint = true;
        }
    }

    // Throttle to 30 fps
    Timer {
        interval: 33
        running: true
        repeat: true
        onTriggered: {
            if (root.needsRepaint) {
                phasorCanvas.requestPaint();
                root.needsRepaint = false;
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: AppData.signalDataModel.selectionModel.clearSelection()
    }

    // Single canvas for grid + phasors
    Canvas {
        id: phasorCanvas
        anchors.fill: parent

        onPaint: {
            let ctx = getContext("2d");
            ctx.clearRect(0, 0, width, height);

            // Draw concentric circles
            for (let i = 0; i < root.circleRadii.length; i++) {
                ctx.beginPath();
                ctx.arc(root.centerX, root.centerY, root.circleRadii[i] * root.plotRadius, 0, 2 * Math.PI);
                ctx.strokeStyle = (root.circleRadii[i] === 1.0) ? AppTheme.colors.borderEmphasized : AppTheme.colors.borderSubtle;
                ctx.lineWidth = (root.circleRadii[i] === 1.0) ? root.gridLineThick : root.gridLineThin;

                ctx.stroke();
            }

            // Draw radial lines
            for (let angle = 0; angle < 360; angle += root.radialAngleStep) {
                let angleRad = angle * root.degreesToRadians;
                let endX = root.centerX + root.plotRadius * Math.cos(angleRad);
                let endY = root.centerY - root.plotRadius * Math.sin(angleRad);

                ctx.beginPath();
                ctx.moveTo(root.centerX, root.centerY);
                ctx.lineTo(endX, endY);
                ctx.strokeStyle = (angle % root.majorAngleStep === 0) ? AppTheme.colors.border : AppTheme.colors.borderSubtle;
                ctx.lineWidth = (angle % root.majorAngleStep === 0) ? root.gridLineThin * 1.25 : root.gridLineThin * 0.66;
                ctx.stroke();
            }

            // Draw phasor arrows - iterate over all signals (always visible)
            let signalCount = AppData.signalDataModel.rowCount();
            for (let i = 0; i < signalCount; i++) {
                // Get effective magnitude and phase from SignalDataModel (already computed)
                let magnitude = AppData.signalDataModel.data(AppData.signalDataModel.index(i, 0), SignalDataModel.MagnitudeRole);
                let phase = AppData.signalDataModel.data(AppData.signalDataModel.index(i, 0), SignalDataModel.PhaseAngleRole);
                let color = AppData.signalDataModel.data(AppData.signalDataModel.index(i, 0), Qt.DecorationRole);
                let typeSymbol = AppData.signalDataModel.data(AppData.signalDataModel.index(i, 0), SignalDataModel.TypeSymbolRole);
                let signalType = typeSymbol === "V" ? "Voltage" : "Current";

                // Calculate normalized magnitude based on cutoff
                let cutoff = signalType === "Voltage" ? AppData.signalDataModel.voltageCutoff : AppData.signalDataModel.currentCutoff;
                let normalizedMag = magnitude / cutoff;

                let phaseRad = phase * root.degreesToRadians;
                let tipX = root.centerX + normalizedMag * root.plotRadius * Math.cos(phaseRad);
                let tipY = root.centerY - normalizedMag * root.plotRadius * Math.sin(phaseRad);

                // Draw phasor line
                ctx.beginPath();
                ctx.moveTo(root.centerX, root.centerY);
                ctx.lineTo(tipX, tipY);
                ctx.strokeStyle = color;
                ctx.lineWidth = root.phasorLineWidth;
                ctx.lineCap = "round";
                ctx.stroke();

                // Draw arrowhead
                let arrowAngle = root.arrowAngleDeg * root.degreesToRadians;
                let backAngle = phaseRad + Math.PI;

                ctx.beginPath();
                ctx.moveTo(tipX, tipY);
                ctx.lineTo(tipX + root.arrowSize * Math.cos(backAngle + arrowAngle), tipY - root.arrowSize * Math.sin(backAngle + arrowAngle));
                ctx.lineTo(tipX + root.arrowSize * Math.cos(backAngle - arrowAngle), tipY - root.arrowSize * Math.sin(backAngle - arrowAngle));
                ctx.closePath();
                ctx.fillStyle = color;
                ctx.fill();
            }

            // Draw center dot
            ctx.beginPath();
            ctx.arc(root.centerX, root.centerY, root.centerDotRadius, 0, 2 * Math.PI);
            ctx.fillStyle = AppTheme.colors.borderEmphasized;
            ctx.fill();
            ctx.strokeStyle = AppTheme.colors.textTertiary;
            ctx.lineWidth = root.gridLineThin;
            ctx.stroke();
        }
    }

    // Angle labels
    Repeater {
        model: [0, 90, 180, 270]
        delegate: Text {
            required property var modelData
            property real angleRad: modelData * root.degreesToRadians
            property real labelDist: root.plotRadius * root.angleLabelDistanceRatio
            x: root.centerX + labelDist * Math.cos(angleRad) - width / 2
            y: root.centerY - labelDist * Math.sin(angleRad) - height / 2
            text: modelData + "°"
            color: AppTheme.colors.textTertiary
            font.pixelSize: AppTheme.typography.size.medium
            font.weight: Font.Medium
            font.family: AppTheme.typography.fontFamily
        }
    }

    // Invisible tap areas for phasor arrows
    Repeater {
        model: AppData.signalDataModel

        delegate: Item {
            id: phasorTapArea
            required property int index
            required property real magnitude
            required property real phaseAngle
            required property string typeSymbol

            property string signalType: typeSymbol === "V" ? "Voltage" : "Current"
            property real cutoff: phasorTapArea.signalType === "Voltage" ? AppData.signalDataModel.voltageCutoff : AppData.signalDataModel.currentCutoff
            property real normalizedMagnitude: phasorTapArea.magnitude / phasorTapArea.cutoff
            property real phaseRad: phasorTapArea.phaseAngle * root.degreesToRadians
            property real tipX: root.centerX + phasorTapArea.normalizedMagnitude * root.plotRadius * Math.cos(phasorTapArea.phaseRad)
            property real tipY: root.centerY - phasorTapArea.normalizedMagnitude * root.plotRadius * Math.sin(phasorTapArea.phaseRad)
            property real arrowLength: phasorTapArea.normalizedMagnitude * root.plotRadius

            // Create a series of small rectangular tap zones along the phasor arrow
            Repeater {
                model: Math.max(10, Math.floor(phasorTapArea.arrowLength / root.tapTargetSpacing))

                delegate: Item {
                    required property int index

                    property int totalCount: Math.max(10, Math.floor(phasorTapArea.arrowLength / root.tapTargetSpacing))
                    property real t: totalCount > 1 ? index / (totalCount - 1) : 0
                    property real posX: root.centerX + t * (phasorTapArea.tipX - root.centerX)
                    property real posY: root.centerY + t * (phasorTapArea.tipY - root.centerY)
                    property real halfTarget: root.tapTargetSize / 2

                    x: posX - halfTarget
                    y: posY - halfTarget
                    width: root.tapTargetSize
                    height: root.tapTargetSize

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            const modelIndex = AppData.signalDataModel.index(phasorTapArea.index, 0);
                            AppData.signalDataModel.selectionModel.select(modelIndex, ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Rows);
                        }
                    }
                }
            }
        }
    }

    // Phasor labels - Repeater iterates over the signal model rows
    Repeater {
        model: AppData.signalDataModel

        delegate: Rectangle {
            id: labelDelegate

            required property int index
            required property color decoration
            required property real magnitude
            required property real phaseAngle
            required property string typeSymbol
            required property string toolTip

            // Calculate phasor tip position for label placement
            property real cutoff: typeSymbol === "V" ? AppData.signalDataModel.voltageCutoff : AppData.signalDataModel.currentCutoff
            property real normalizedMagnitude: magnitude / cutoff
            property real phaseRad: phaseAngle * root.degreesToRadians
            property real tipX: root.centerX + normalizedMagnitude * root.plotRadius * Math.cos(phaseRad)
            property real tipY: root.centerY - normalizedMagnitude * root.plotRadius * Math.sin(phaseRad)

            // Position tooltip at phasor tip
            x: tipX + AppTheme.typography.size.large
            y: tipY - height / 2

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
