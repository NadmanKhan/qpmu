pragma ComponentBehavior: Bound
import QtQuick
import QtQml
import qpmu

// "Dumb" phasor plot view - knows nothing about signal count or types
// Just iterates over the provided model and renders each signal
Rectangle {
    id: root

    color: AppTheme.colors.surface

    required property ApplicationDataModel appDataModel

    readonly property real plotRadius: Math.min(width, height) * 0.38
    readonly property real centerX: width / 2
    readonly property real centerY: height / 2
    readonly property real lineWidth: 3
    readonly property real thinLine: 1.2
    readonly property real thickLine: 2.5

    property bool needsRepaint: false

    Connections {
        target: root.appDataModel
        function onDataUpdated() {
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

    // Single canvas for grid + phasors
    Canvas {
        id: phasorCanvas
        anchors.fill: parent

        onPaint: {
            let ctx = getContext("2d");
            ctx.clearRect(0, 0, width, height);

            // Draw concentric circles
            let circles = [1.0, 0.75, 0.5, 0.25];
            for (let i = 0; i < circles.length; i++) {
                ctx.beginPath();
                ctx.arc(root.centerX, root.centerY, circles[i] * root.plotRadius, 0, 2 * Math.PI);
                ctx.strokeStyle = (circles[i] === 1.0) ? AppTheme.colors.borderEmphasized : AppTheme.colors.borderSubtle;
                ctx.lineWidth = (circles[i] === 1.0) ? root.thickLine : root.thinLine;
                ctx.stroke();
            }

            // Draw radial lines (every 30 degrees)
            for (let angle = 0; angle < 360; angle += 30) {
                let angleRad = angle * Math.PI / 180.0;
                let endX = root.centerX + root.plotRadius * Math.cos(angleRad);
                let endY = root.centerY - root.plotRadius * Math.sin(angleRad);

                ctx.beginPath();
                ctx.moveTo(root.centerX, root.centerY);
                ctx.lineTo(endX, endY);
                ctx.strokeStyle = (angle % 90 === 0) ? AppTheme.colors.border : AppTheme.colors.borderSubtle;
                ctx.lineWidth = (angle % 90 === 0) ? root.thinLine * 1.25 : root.thinLine * 0.66;
                ctx.stroke();
            }

            // Draw phasor arrows - iterate over signals from model
            let signalCount = root.appDataModel.signalDataModel.rowCount();
            for (let i = 0; i < signalCount; i++) {
                let signal = root.appDataModel.signalDataModel.data(root.appDataModel.signalDataModel.index(i, 0), SignalDataModel.SignalDataRole);
                let magnitude = signal.magnitude;
                let phase = signal.phase;
                let color = signal.color;
                let normalizedMag = signal.normalizedMagnitude;

                let phaseRad = phase * Math.PI / 180.0;
                let tipX = root.centerX + normalizedMag * root.plotRadius * Math.cos(phaseRad);
                let tipY = root.centerY - normalizedMag * root.plotRadius * Math.sin(phaseRad);

                // Draw phasor line
                ctx.beginPath();
                ctx.moveTo(root.centerX, root.centerY);
                ctx.lineTo(tipX, tipY);
                ctx.strokeStyle = color;
                ctx.lineWidth = root.lineWidth;
                ctx.lineCap = "round";
                ctx.stroke();

                // Draw arrowhead
                let arrowSize = 12;
                let arrowAngle = 20 * Math.PI / 180.0;
                let backAngle = phaseRad + Math.PI;

                ctx.beginPath();
                ctx.moveTo(tipX, tipY);
                ctx.lineTo(tipX + arrowSize * Math.cos(backAngle + arrowAngle), tipY - arrowSize * Math.sin(backAngle + arrowAngle));
                ctx.lineTo(tipX + arrowSize * Math.cos(backAngle - arrowAngle), tipY - arrowSize * Math.sin(backAngle - arrowAngle));
                ctx.closePath();
                ctx.fillStyle = color;
                ctx.fill();
            }

            // Draw center dot
            let centerDotRadius = 5;
            ctx.beginPath();
            ctx.arc(root.centerX, root.centerY, centerDotRadius, 0, 2 * Math.PI);
            ctx.fillStyle = AppTheme.colors.borderEmphasized;
            ctx.fill();
            ctx.strokeStyle = AppTheme.colors.textTertiary;
            ctx.lineWidth = root.thinLine;
            ctx.stroke();
        }
    }

    // Angle labels
    Repeater {
        model: [0, 90, 180, 270]
        delegate: Text {
            required property var modelData
            property real angleRad: modelData * Math.PI / 180.0
            property real labelDist: root.plotRadius * 1.15
            x: root.centerX + labelDist * Math.cos(angleRad) - width / 2
            y: root.centerY - labelDist * Math.sin(angleRad) - height / 2
            text: modelData + "°"
            color: AppTheme.colors.textTertiary
            font.pixelSize: AppTheme.typography.size.medium
            font.weight: Font.Medium
            font.family: AppTheme.typography.fontFamily
        }
    }

    // Phasor labels - Repeater iterates over the signal model rows
    Repeater {
        model: root.appDataModel.signalDataModel.rowCount()

        delegate: Rectangle {
            id: labelRect
            required property int index

            property var signalData: root.appDataModel.signalDataModel.data(root.appDataModel.signalDataModel.index(index, 0), SignalDataModel.SignalDataRole)
            property real magnitude: signalData ? signalData.magnitude : 0
            property real phase: signalData ? signalData.phase : 0
            property string name: signalData ? signalData.name : ""
            property string unit: signalData ? signalData.unit : ""
            property real normalizedMagnitude: signalData ? signalData.normalizedMagnitude : 0
            property color signalColor: signalData ? signalData.color : "transparent"

            property real phaseRad: phase * Math.PI / 180.0
            property real tipX: root.centerX + normalizedMagnitude * root.plotRadius * Math.cos(phaseRad)
            property real tipY: root.centerY - normalizedMagnitude * root.plotRadius * Math.sin(phaseRad)

            x: tipX + AppTheme.typography.size.large
            y: tipY - height / 2
            width: labelText.width + AppTheme.spacing.medium
            height: labelText.height + AppTheme.spacing.small
            color: "transparent"
            radius: AppTheme.radius.small
            border.color: signalColor
            border.width: AppTheme.border.thin

            Text {
                id: labelText
                anchors.centerIn: parent
                text: labelRect.name + " " + labelRect.magnitude.toFixed(1) + labelRect.unit + " ∠" + labelRect.phase.toFixed(0) + "°"
                color: labelRect.signalColor
                font.pixelSize: AppTheme.typography.size.small
                font.weight: Font.Medium
                font.family: AppTheme.typography.fontFamilyMonospace
            }
        }
    }
}
