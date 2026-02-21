import QtQuick
import QtQuick.Shapes

Rectangle {
    id: root
    color: "#0a0e14"  // Deeper dark blue-gray background

    required property var dataModel

    readonly property real plotRadius: Math.min(width, height) * 0.38
    readonly property real centerX: width / 2
    readonly property real centerY: height / 2

    // Helper function: convert polar to Cartesian
    function polarToCartesian(magnitude, angleDeg) {
        let angleRad = angleDeg * Math.PI / 180.0;
        return Qt.point(
            centerX + magnitude * plotRadius * Math.cos(angleRad),
            centerY - magnitude * plotRadius * Math.sin(angleRad)
        );
    }

    // Background concentric circles with glow effect
    Repeater {
        model: [1.0, 0.75, 0.5, 0.25]
        delegate: Shape {
            anchors.fill: parent
            layer.enabled: true
            layer.samples: 4

            ShapePath {
                strokeWidth: modelData === 1.0 ? 2.5 : 1.2
                strokeColor: modelData === 1.0 ? "#3d5a80" : "#1b2838"
                fillColor: "transparent"

                PathAngleArc {
                    centerX: root.centerX
                    centerY: root.centerY
                    radiusX: modelData * root.plotRadius
                    radiusY: modelData * root.plotRadius
                    startAngle: 0
                    sweepAngle: 360
                }
            }
        }
    }

    // Radial lines (every 30 degrees)
    Repeater {
        model: 12
        delegate: Shape {
            anchors.fill: parent
            ShapePath {
                strokeWidth: (index % 3 === 0) ? 1.5 : 0.8
                strokeColor: (index % 3 === 0) ? "#2a3f5f" : "#1b2838"
                fillColor: "transparent"

                startX: root.centerX
                startY: root.centerY

                PathLine {
                    x: root.centerX + root.plotRadius * Math.cos(index * 30 * Math.PI / 180)
                    y: root.centerY - root.plotRadius * Math.sin(index * 30 * Math.PI / 180)
                }
            }
        }
    }

    // Angle labels (0°, 90°, 180°, 270°)
    Repeater {
        model: [0, 90, 180, 270]
        delegate: Text {
            property point labelPos: root.polarToCartesian(1.15, modelData)
            x: labelPos.x - width/2
            y: labelPos.y - height/2
            text: modelData + "°"
            color: "#6b8cae"
            font.pixelSize: 13
            font.weight: Font.Medium
            font.family: "SF Pro Text, Segoe UI, sans-serif"
        }
    }

    // Phasor arrows with improved rendering
    Repeater {
        model: 6
        delegate: Item {
            id: phasorItem
            anchors.fill: parent

            property real magnitude: dataModel.phasorMagnitudes[index]
            property real phase: dataModel.phasorPhases[index]
            property color signalColor: dataModel.signalColors[index]
            property string signalName: dataModel.signalNames[index]
            property real normalizedMag: index < 3 ? magnitude / 300.0 : magnitude / 20.0
            property point tipPoint: root.polarToCartesian(normalizedMag, phase)

            // Phasor line with glow
            Shape {
                anchors.fill: parent
                layer.enabled: true
                layer.samples: 8

                ShapePath {
                    strokeWidth: 3.5
                    strokeColor: phasorItem.signalColor
                    fillColor: "transparent"
                    capStyle: ShapePath.RoundCap

                    startX: root.centerX
                    startY: root.centerY

                    PathLine {
                        x: phasorItem.tipPoint.x
                        y: phasorItem.tipPoint.y
                    }
                }
            }

            // Arrowhead (filled triangle)
            Shape {
                anchors.fill: parent
                layer.enabled: true
                layer.samples: 4

                ShapePath {
                    id: arrowPath
                    strokeWidth: 0
                    fillColor: phasorItem.signalColor

                    property real arrowSize: 12
                    property real arrowAngle: 20
                    property real angleRad: (phasorItem.phase + 180) * Math.PI / 180.0
                    property point left: Qt.point(
                        phasorItem.tipPoint.x + arrowSize * Math.cos(angleRad + arrowAngle * Math.PI / 180),
                        phasorItem.tipPoint.y - arrowSize * Math.sin(angleRad + arrowAngle * Math.PI / 180)
                    )
                    property point right: Qt.point(
                        phasorItem.tipPoint.x + arrowSize * Math.cos(angleRad - arrowAngle * Math.PI / 180),
                        phasorItem.tipPoint.y - arrowSize * Math.sin(angleRad - arrowAngle * Math.PI / 180)
                    )

                    startX: phasorItem.tipPoint.x
                    startY: phasorItem.tipPoint.y

                    PathLine { x: arrowPath.left.x; y: arrowPath.left.y }
                    PathLine { x: arrowPath.right.x; y: arrowPath.right.y }
                    PathLine { x: phasorItem.tipPoint.x; y: phasorItem.tipPoint.y }
                }
            }

            // Label with background
            Rectangle {
                x: phasorItem.tipPoint.x + 18
                y: phasorItem.tipPoint.y - height/2
                width: labelText.width + 16
                height: labelText.height + 8
                color: "#0a0e1499"
                radius: 6
                border.color: Qt.lighter(phasorItem.signalColor, 1.3)
                border.width: 1.5

                Text {
                    id: labelText
                    anchors.centerIn: parent
                    text: phasorItem.signalName + " " +
                          phasorItem.magnitude.toFixed(1) + (index < 3 ? "V" : "A") + " ∠" +
                          phasorItem.phase.toFixed(0) + "°"
                    color: "#e8f0f8"
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                    font.family: "SF Mono, Consolas, monospace"
                }
            }
        }
    }

    // Center dot
    Rectangle {
        x: root.centerX - 5
        y: root.centerY - 5
        width: 10
        height: 10
        radius: 5
        color: "#3d5a80"
        border.color: "#6b8cae"
        border.width: 2
    }

    // Title with better styling
    Rectangle {
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 20
        width: titleText.width + 24
        height: titleText.height + 12
        color: "#0a0e1499"
        radius: 8
        border.color: "#3d5a80"
        border.width: 1.5

        Text {
            id: titleText
            anchors.centerIn: parent
            text: "Phasor Diagram"
            font.pixelSize: 17
            font.weight: Font.DemiBold
            color: "#d0dae8"
            font.family: "SF Pro Display, Segoe UI, sans-serif"
        }
    }
}
