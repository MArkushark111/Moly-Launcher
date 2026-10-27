import QtQuick
import QtQuick.Controls

Switch {
    id: control
    property bool animationReady: false
    property real thumbPosition: checked ? 1 : 0
    implicitWidth: 44
    implicitHeight: 32
    padding: 0
    Component.onCompleted: animationReady = true

    // The delegate keeps this visual state while its model row is updated. The
    // instance content model must update roles in place rather than reset itself.
    Behavior on thumbPosition {
        enabled: control.animationReady
        NumberAnimation {
            duration: Theme.motion ? 230 : 0
            easing.type: Easing.OutQuint
        }
    }
    indicator: Rectangle {
        id: track
        x: 1
        y: (control.height - height) / 2
        width: 40
        height: 22
        radius: height / 2
        color: control.checked ? Theme.accent : Theme.surface2
        border.color: control.checked ? Theme.accent : Theme.border
        opacity: control.enabled ? 1 : 0.5
        Behavior on color {
            ColorAnimation {
                duration: Theme.motion ? 190 : 0
                easing.type: Easing.OutCubic
            }
        }
        Behavior on border.color {
            ColorAnimation {
                duration: Theme.motion ? 190 : 0
            }
        }
        Rectangle {
            anchors.fill: parent
            anchors.margins: -4
            radius: 15
            color: Theme.transparent
            border.width: 2
            border.color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.22)
            visible: control.activeFocus
        }
        Rectangle {
            id: knob
            x: 3 + (track.width - width - 6) * control.thumbPosition
            y: 3
            width: control.pressed && Theme.motion ? 18 : 16
            height: 16
            radius: height / 2
            color: control.checked ? Theme.onAccent : Theme.textSecondary
            Behavior on width {
                NumberAnimation {
                    duration: Theme.motion ? 120 : 0
                    easing.type: Easing.OutCubic
                }
            }
            Behavior on color {
                ColorAnimation {
                    duration: Theme.motion ? 170 : 0
                }
            }
        }
    }
    contentItem: Item {}
    background: Item {}
}
