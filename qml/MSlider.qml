import QtQuick
import QtQuick.Controls

Slider {
    id: control
    implicitHeight: 32
    implicitWidth: 180
    padding: 8
    background: Rectangle {
        x: control.leftPadding; y: control.topPadding + control.availableHeight/2-height/2
        width: control.availableWidth; height: 5; radius: 3; color: Theme.surface2
        Rectangle { width: control.visualPosition*parent.width; height: parent.height; radius: 3; color: Theme.accent }
    }
    handle: Rectangle {
        x: control.leftPadding + control.visualPosition*(control.availableWidth-width)
        y: control.topPadding + control.availableHeight/2-height/2
        implicitWidth: 18; implicitHeight: 18; radius: 9
        color: control.pressed ? Theme.accentHover : Theme.accent
        border.width: control.visualFocus ? 2 : 0; border.color: Theme.text
        scale: control.pressed && Theme.motion ? 1.15 : 1
        Behavior on scale { NumberAnimation { duration: Theme.motion ? 120 : 0 } }
    }
}
