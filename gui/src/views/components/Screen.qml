import QtQuick 2.12
import QtQuick.Controls 2.12

/**
 * Screen - Base component for all application screens
 *
 * Provides standard interface for screens in StackView navigation:
 * - title: Displayed in toolbar
 * - contextMenuModel: Optional context menu for this screen
 * - Navigation signals for screen transitions
 *
 * Subclass this for all screens (LiveMonitor, Settings, etc.)
 */
Rectangle {
    id: root

    // Default property allows children to be added normally
    default property alias content: contentItem.data

    // Screen metadata
    property string title: "Screen"
    property bool canGoBack: false
    property Component contextMenu: null

    // Navigation signals
    signal navigateTo(Component screen)
    signal navigateBack

    // Lifecycle hooks (subclasses can override)
    signal activated    // When pushed onto stack
    signal deactivated  // When popped from stack

    // StackView lifecycle integration
    StackView.onActivated: root.activated()
    StackView.onDeactivated: root.deactivated()

    // Content container
    Item {
        id: contentItem
        anchors.fill: parent
    }
}
