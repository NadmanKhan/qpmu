pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "controls"
import qpmu

ScrollView {
    id: root

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
                isRightActive: AppData.signalDataModel.magnitudeMode === SignalDataModel.Peak
                onToggled: {
                    AppData.signalDataModel.magnitudeMode = isRightActive ? SignalDataModel.Peak : SignalDataModel.RMS;
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
                currentIndex: AppData.signalDataModel.phaseReferenceIndex + 1  // +1 because Absolute is index 0 in dropdown
                onActivated: index => {
                    AppData.signalDataModel.phaseReferenceIndex = index - 1;  // -1 because Absolute is -1 in model
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
                label: AppData.signalDataModel.isPaused ? "Resume" : "Pause"
                icon: AppData.signalDataModel.isPaused ? "▶" : "⏸"
                active: AppData.signalDataModel.isPaused
                onClicked: AppData.signalDataModel.isPaused = !AppData.signalDataModel.isPaused
            }
        }

        Item {
            Layout.preferredHeight: AppTheme.spacing.medium
        }

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
                isRightActive: AppData.signalDataModel.voltageScalingMode === SignalDataModel.Manual
                onToggled: {
                    AppData.signalDataModel.voltageScalingMode = isRightActive ? SignalDataModel.Manual : SignalDataModel.Dynamic;
                }
            }

            SliderWithValue {
                Layout.fillWidth: true
                visible: AppData.signalDataModel.voltageScalingMode === SignalDataModel.Manual
                from: 10
                to: 500
                value: AppData.signalDataModel.voltageCutoff
                stepSize: 5
                unit: "V"
                decimals: 0
                onValueChanged: {
                    if (AppData.signalDataModel.voltageScalingMode === SignalDataModel.Manual) {
                        AppData.signalDataModel.voltageCutoff = value;
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
                isRightActive: AppData.signalDataModel.currentScalingMode === SignalDataModel.Manual
                onToggled: {
                    AppData.signalDataModel.currentScalingMode = isRightActive ? SignalDataModel.Manual : SignalDataModel.Dynamic;
                }
            }

            SliderWithValue {
                Layout.fillWidth: true
                visible: AppData.signalDataModel.currentScalingMode === SignalDataModel.Manual
                from: 1
                to: 50
                value: AppData.signalDataModel.currentCutoff
                stepSize: 0.5
                unit: "A"
                decimals: 1
                onValueChanged: {
                    if (AppData.signalDataModel.currentScalingMode === SignalDataModel.Manual) {
                        AppData.signalDataModel.currentCutoff = value;
                    }
                }
            }
        }

        // Spacer at bottom
        Item {
            Layout.fillHeight: true
        }
    }
}
