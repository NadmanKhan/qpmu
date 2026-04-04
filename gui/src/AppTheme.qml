pragma Singleton
import QtQuick 2.12

// AppTheme - Comprehensive design system singleton
// Single source of truth for all UI constants
// Qt handles DPI scaling automatically - all values are in logical pixels
QtObject {
    id: root

    // ============================================================================
    // COLORS
    // ============================================================================

    readonly property var colors: QtObject {
        // Backgrounds (layered elevation)
        readonly property color background: "#050810"
        readonly property color surface: "#0a0e14"
        readonly property color surfaceElevated: "#1b2838"
        readonly property color surfaceHover: "#1a2634"
        readonly property color surfacePressed: "#0f1823"

        // Primary brand colors
        readonly property color primary: "#06d6a0"
        readonly property color primaryHover: "#08edb8"
        readonly property color primaryPressed: "#05bd88"

        // Semantic state colors
        readonly property color success: "#06d6a0"
        readonly property color error: "#e63946"
        readonly property color warning: "#f9a825"
        readonly property color info: "#45b7d1"

        // Text colors (hierarchy)
        readonly property color textPrimary: "#d0dae8"
        readonly property color textSecondary: "#8ba3be"
        readonly property color textTertiary: "#6b8cae"
        readonly property color textInverse: "#ffffff"

        // Border colors
        readonly property color border: "#2a3f5f"
        readonly property color borderEmphasized: "#3d5a80"
        readonly property color borderSubtle: "#1b2838"

        // Overlays (with alpha)
        readonly property color overlay: "#00000040"
        readonly property color scrim: "#00000080"

        // Chart/visualization colors
        readonly property color gridPrimary: "#3d5a80"
        readonly property color gridSecondary: "#2a3f5f"
        readonly property color gridSubtle: "#1b2838"
    }

    // ============================================================================
    // TYPOGRAPHY
    // ============================================================================

    readonly property var typography: QtObject {
        // Font families
        readonly property string fontFamily: "SF Pro Text, Segoe UI, sans-serif"
        readonly property string fontFamilyMonospace: "SF Mono, Consolas, monospace"

        // Font sizes (logical pixels)
        readonly property var size: QtObject {
            readonly property int tiny: 10
            readonly property int small: 11
            readonly property int medium: 13
            readonly property int normal: 15
            readonly property int large: 18
            readonly property int huge: 22
        }

        // Font weights (semantic)
        readonly property var weight: QtObject {
            readonly property int regular: Font.Normal
            readonly property int medium: Font.Medium
            readonly property int semibold: Font.DemiBold
            readonly property int bold: Font.Bold
        }

        // Line heights (multipliers)
        readonly property var lineHeight: QtObject {
            readonly property real tight: 1.2
            readonly property real normal: 1.5
            readonly property real relaxed: 1.8
        }
    }

    // ============================================================================
    // SPACING
    // ============================================================================

    readonly property var spacing: QtObject {
        readonly property int tiny: 3
        readonly property int small: 8
        readonly property int medium: 14
        readonly property int large: 24
        readonly property int huge: 30
    }

    // ============================================================================
    // SIZING
    // ============================================================================

    readonly property var sizing: QtObject {
        // Common component heights
        readonly property int small: 30
        readonly property int medium: 44
        readonly property int large: 54

        // Interactive minimums (for touch targets)
        readonly property int touchTarget: 44

        // Layout components
        readonly property int toolbarHeight: 55
        readonly property int statusBarHeight: 72
    }

    // ============================================================================
    // RADIUS
    // ============================================================================

    readonly property var radius: QtObject {
        readonly property int none: 0
        readonly property int small: 6
        readonly property int medium: 8
        readonly property int large: 10
        readonly property int full: 9999
    }

    // ============================================================================
    // BORDER
    // ============================================================================

    readonly property var border: QtObject {
        readonly property int none: 0
        readonly property int thin: 1
        readonly property int medium: 2
        readonly property int thick: 2
    }

    // ============================================================================
    // MOTION
    // ============================================================================

    readonly property var motion: QtObject {
        // Durations (milliseconds)
        readonly property int instant: 100
        readonly property int fast: 150
        readonly property int normal: 200
        readonly property int slow: 250
        readonly property int slowest: 300
        readonly property int pulse: 800

        // Easing curves
        readonly property int easeOut: Easing.OutQuad
        readonly property int easeIn: Easing.InQuad
        readonly property int easeInOut: Easing.InOutQuad
        readonly property int easeOutCubic: Easing.OutCubic
    }

    // ============================================================================
    // ELEVATION
    // ============================================================================

    readonly property var elevation: QtObject {
        readonly property int base: 0
        readonly property int raised: 1
        readonly property int overlay: 10
        readonly property int modal: 100
        readonly property int notification: 1000
    }

    // ============================================================================
    // OPACITY
    // ============================================================================

    readonly property var opacity: QtObject {
        readonly property real disabled: 0.38
        readonly property real subtle: 0.6
        readonly property real medium: 0.75
        readonly property real opaque: 1.0

        // Overlay alphas
        readonly property real overlayLight: 0.125
        readonly property real overlayMedium: 0.25
        readonly property real overlayHeavy: 0.5
    }

    // ============================================================================
    // STATE COLORS
    // ============================================================================

    readonly property var state: QtObject {
        // Primary button states
        readonly property color primaryDefault: root.colors.primary
        readonly property color primaryHover: root.colors.primaryHover
        readonly property color primaryPressed: root.colors.primaryPressed

        // Error button states
        readonly property color errorDefault: root.colors.error
        readonly property color errorHover: Qt.lighter(root.colors.error, 1.1)
        readonly property color errorPressed: Qt.darker(root.colors.error, 1.1)

        // Surface states (for buttons/cards)
        readonly property color surfaceDefault: root.colors.surfaceElevated
        readonly property color surfaceHover: root.colors.surfaceHover
        readonly property color surfacePressed: root.colors.surfacePressed
    }

    // ============================================================================
    // HELPER FUNCTIONS
    // ============================================================================

    function withAlpha(color, alpha) {
        return Qt.rgba(color.r, color.g, color.b, alpha);
    }

    function hoverColor(baseColor, factor) {
        if (factor === undefined)
            factor = 1.1;
        return Qt.lighter(baseColor, factor);
    }

    function pressedColor(baseColor, factor) {
        if (factor === undefined)
            factor = 1.1;
        return Qt.darker(baseColor, factor);
    }
}
