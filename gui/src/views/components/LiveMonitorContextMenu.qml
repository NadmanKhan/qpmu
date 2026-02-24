pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import qpmu
import "controls"

/**
 * LiveMonitorContextMenu - Explicit context menu for LiveMonitor screen
 *
 * Contains all controls from the original ControlPanel:
 * - DATA CONTROLS: Magnitude, Phase Reference, Playback
 * - PLOT CONTROLS: Voltage/Current Scaling
 */
ScrollView {
    id: root

    required property ViewStateModel viewStateModel

    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
    ScrollBar.vertical.policy: ScrollBar.AsNeeded
    clip: true

    ColumnLayout {
        width: root.availableWidth
        spacing: AppTheme.spacing.medium

        // DATA CONTROLS Section
        SectionHeader {
            Layout.fillWidth: true
            title: "DATA CONTROLS"
        }

        // Magnitude Display Toggle
        ColumnLayout {
            Layout.fillWidth: true
            Layout.leftMargin: AppTheme.spacing.medium
            Layout.rightMargin: AppTheme.spacing.medium
            spacing: AppTheme.spacing.small

            Text {
                text: "Magnitude"
                font.pixelSize: AppTheme.typography.size.small
                font.weight: Font.Medium
                font.family: AppTheme.typography.fontFamily
                color: AppTheme.colors.textSecondary
            }

            ToggleButton {
                Layout.fillWidth: true
                leftLabel: "RMS"
                rightLabel: "Peak"
                isRightActive: root.viewStateModel.magnitudeMode === ViewStateModel.Peak
                onToggled: {
                    root.viewStateModel.magnitudeMode = isRightActive ? ViewStateModel.Peak : ViewStateModel.RMS;
                }
            }
        }

        // Phase Reference Dropdown
        ColumnLayout {
            Layout.fillWidth: true
            Layout.leftMargin: AppTheme.spacing.medium
            Layout.rightMargin: AppTheme.spacing.medium
            spacing: AppTheme.spacing.small

            Text {
                text: "Phase Reference"
                font.pixelSize: AppTheme.typography.size.small
                font.weight: Font.Medium
                font.family: AppTheme.typography.fontFamily
                color: AppTheme.colors.textSecondary
            }

            Dropdown {
                Layout.fillWidth: true
                model: ["Absolute", "VA", "VB", "VC", "IA", "IB", "IC"]
                currentIndex: root.viewStateModel.phaseReferenceSignalIndex + 1  // +1 because Absolute is index 0 in dropdown
                onActivated: (index) => {
                    root.viewStateModel.phaseReferenceSignalIndex = index - 1;  // -1 because Absolute is -1 in model
                }
            }
        }

        // Pause Button
        ColumnLayout {
            Layout.fillWidth: true
            Layout.leftMargin: AppTheme.spacing.medium
            Layout.rightMargin: AppTheme.spacing.medium
            spacing: AppTheme.spacing.small

            Text {
                text: "Playback"
                font.pixelSize: AppTheme.typography.size.small
                font.weight: Font.Medium
                font.family: AppTheme.typography.fontFamily
                color: AppTheme.colors.textSecondary
            }

            Button {
                Layout.fillWidth: true
                label: root.viewStateModel.isPausedLocal ? "Resume" : "Pause"
                icon: root.viewStateModel.isPausedLocal ? "▶" : "⏸"
                active: root.viewStateModel.isPausedLocal
                onClicked: root.viewStateModel.isPausedLocal = !root.viewStateModel.isPausedLocal
            }
        }

        Item { Layout.preferredHeight: AppTheme.spacing.medium }

        // PLOT CONTROLS Section
        SectionHeader {
            Layout.fillWidth: true
            title: "PLOT CONTROLS"
        }

        // Voltage Scaling
        ColumnLayout {
            Layout.fillWidth: true
            Layout.leftMargin: AppTheme.spacing.medium
            Layout.rightMargin: AppTheme.spacing.medium
            spacing: AppTheme.spacing.small

            Text {
                text: "Voltage Scaling"
                font.pixelSize: AppTheme.typography.size.small
                font.weight: Font.Medium
                font.family: AppTheme.typography.fontFamily
                color: AppTheme.colors.textSecondary
            }

            ToggleButton {
                Layout.fillWidth: true
                leftLabel: "Dynamic"
                rightLabel: "Manual"
                isRightActive: root.viewStateModel.voltageScalingMode === ViewStateModel.Manual
                onToggled: {
                    root.viewStateModel.voltageScalingMode = isRightActive ? ViewStateModel.Manual : ViewStateModel.Dynamic;
                }
            }

            SliderWithValue {
                Layout.fillWidth: true
                visible: root.viewStateModel.voltageScalingMode === ViewStateModel.Manual
                from: 10
                to: 500
                value: root.viewStateModel.voltageCutoff
                stepSize: 5
                unit: "V"
                decimals: 0
                onValueChanged: {
                    if (root.viewStateModel.voltageScalingMode === ViewStateModel.Manual) {
                        root.viewStateModel.voltageCutoff = value;
                    }
                }
            }
        }

        // Current Scaling
        ColumnLayout {
            Layout.fillWidth: true
            Layout.leftMargin: AppTheme.spacing.medium
            Layout.rightMargin: AppTheme.spacing.medium
            spacing: AppTheme.spacing.small

            Text {
                text: "Current Scaling"
                font.pixelSize: AppTheme.typography.size.small
                font.weight: Font.Medium
                font.family: AppTheme.typography.fontFamily
                color: AppTheme.colors.textSecondary
            }

            ToggleButton {
                Layout.fillWidth: true
                leftLabel: "Dynamic"
                rightLabel: "Manual"
                isRightActive: root.viewStateModel.currentScalingMode === ViewStateModel.Manual
                onToggled: {
                    root.viewStateModel.currentScalingMode = isRightActive ? ViewStateModel.Manual : ViewStateModel.Dynamic;
                }
            }

            SliderWithValue {
                Layout.fillWidth: true
                visible: root.viewStateModel.currentScalingMode === ViewStateModel.Manual
                from: 1
                to: 50
                value: root.viewStateModel.currentCutoff
                stepSize: 0.5
                unit: "A"
                decimals: 1
                onValueChanged: {
                    if (root.viewStateModel.currentScalingMode === ViewStateModel.Manual) {
                        root.viewStateModel.currentCutoff = value;
                    }
                }
            }
        }

        // Spacer at bottom
        Item { Layout.fillHeight: true }
    }
}
