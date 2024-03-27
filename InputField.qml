import QtQuick 2.0
import QtQuick.VirtualKeyboard 2.0

Item {
    id: inputField
    TextInput {
        id: textInput
        x: 0
        y: 0
        width: inputField.width

        states: State {
            name: "focused"
            when: textInput.focus
            PropertyChanges {
                target: inputPanel
                active: true
            }
        }
    }

    InputPanel {
        id: inputPanel
        z: 99
        x: 0
        y: inputField.height
        width: inputField.width

        states: State {
            name: "visible"
            when: inputPanel.active
            PropertyChanges {
                target: inputPanel
                y: inputField.height - inputPanel.height
            }
        }
        transitions: Transition {
            from: ""
            to: "visible"
            reversible: true
            ParallelAnimation {
                NumberAnimation {
                    properties: "y"
                    duration: 250
                    easing.type: Easing.InOutQuad
                }
            }
        }
    }
}
