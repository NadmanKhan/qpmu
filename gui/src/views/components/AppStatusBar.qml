import QtQuick
import QtQuick.Layouts
import qpmu 1.0

Rectangle {
    id: root

    height: AppTheme.sizing.statusBarHeight
    color: AppTheme.colors.surface
    border.color: appInstance.isPaused ? AppTheme.colors.error : AppTheme.colors.primary
    border.width: 0

    // Top border only
    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: AppTheme.border.thick
        color: appInstance.isPaused ? AppTheme.colors.error : AppTheme.colors.primary

        Behavior on color {
            ColorAnimation {
                duration: AppTheme.motion.normal
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: AppTheme.spacing.medium
        spacing: AppTheme.spacing.large

        // Status indicator
        Rectangle {
            Layout.preferredWidth: 110
            Layout.preferredHeight: AppTheme.sizing.medium
            color: appInstance.isPaused ? AppTheme.withAlpha(AppTheme.colors.error, AppTheme.opacity.overlayLight) : AppTheme.withAlpha(AppTheme.colors.primary, AppTheme.opacity.overlayLight)
            radius: AppTheme.radius.large
            border.color: appInstance.isPaused ? AppTheme.colors.error : AppTheme.colors.primary
            border.width: AppTheme.border.medium

            Behavior on color {
                ColorAnimation {
                    duration: AppTheme.motion.normal
                }
            }
            Behavior on border.color {
                ColorAnimation {
                    duration: AppTheme.motion.normal
                }
            }

            RowLayout {
                anchors.centerIn: parent
                spacing: AppTheme.spacing.small

                Rectangle {
                    width: AppTheme.spacing.small
                    height: AppTheme.spacing.small
                    radius: AppTheme.spacing.small / 2
                    color: appInstance.isPaused ? AppTheme.colors.error : AppTheme.colors.primary

                    SequentialAnimation on opacity {
                        running: !appInstance.isPaused
                        loops: Animation.Infinite
                        NumberAnimation {
                            from: 1.0
                            to: 0.3
                            duration: AppTheme.motion.pulse
                        }
                        NumberAnimation {
                            from: 0.3
                            to: 1.0
                            duration: AppTheme.motion.pulse
                        }
                    }
                }

                Text {
                    text: appInstance.isPaused ? "PAUSED" : "LIVE"
                    font.pixelSize: AppTheme.typography.size.normal
                    font.weight: Font.Bold
                    font.family: AppTheme.typography.fontFamilyMonospace
                    color: appInstance.isPaused ? AppTheme.colors.error : AppTheme.colors.primary
                }
            }
        }

        // Spacer
        Item {
            Layout.fillWidth: true
        }

        // Metrics display
        Row {
            spacing: AppTheme.spacing.huge

            // Time
            Column {
                spacing: AppTheme.spacing.tiny
                Text {
                    text: "TIME"
                    font.pixelSize: AppTheme.typography.size.tiny
                    font.weight: Font.Bold
                    color: AppTheme.colors.textTertiary
                    font.family: AppTheme.typography.fontFamily
                    elide: Text.ElideRight
                }
                Text {
                    text: appInstance.lastSampleTime ?? "--:--:--"
                    font.pixelSize: AppTheme.typography.size.normal
                    font.weight: Font.DemiBold
                    font.family: AppTheme.typography.fontFamilyMonospace
                    color: AppTheme.colors.textPrimary
                    elide: Text.ElideRight
                }
            }

            Rectangle {
                width: AppTheme.border.thin
                height: 40
                color: AppTheme.colors.border
            }

            // Sampling Rate
            Column {
                spacing: AppTheme.spacing.tiny
                Text {
                    text: "SAMPLING RATE"
                    font.pixelSize: AppTheme.typography.size.tiny
                    font.weight: Font.Bold
                    color: AppTheme.colors.textTertiary
                    font.family: AppTheme.typography.fontFamily
                    elide: Text.ElideRight
                }
                Text {
                    text: appInstance.samplingRate.toFixed(1) + " Hz"
                    font.pixelSize: AppTheme.typography.size.normal
                    font.weight: Font.DemiBold
                    font.family: AppTheme.typography.fontFamilyMonospace
                    color: AppTheme.colors.info
                    elide: Text.ElideRight
                }
            }

            Rectangle {
                width: AppTheme.border.thin
                height: 40
                color: AppTheme.colors.border
            }

            // Frequency
            Column {
                spacing: AppTheme.spacing.tiny
                Text {
                    text: "FREQUENCY"
                    font.pixelSize: AppTheme.typography.size.tiny
                    font.weight: Font.Bold
                    color: AppTheme.colors.textTertiary
                    font.family: AppTheme.typography.fontFamily
                    elide: Text.ElideRight
                }
                Text {
                    text: appInstance.systemFrequency.toFixed(3) + " Hz"
                    font.pixelSize: AppTheme.typography.size.normal
                    font.weight: Font.DemiBold
                    font.family: AppTheme.typography.fontFamilyMonospace
                    color: AppTheme.colors.primary
                    elide: Text.ElideRight
                }
            }
        }
    }
}
