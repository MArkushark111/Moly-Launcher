import QtQuick
import QtQuick.Controls

CheckBox {
    id: control
    spacing: 9
    implicitHeight: 36
    indicator: Rectangle {
        x: control.leftPadding; y: control.height/2-height/2
        implicitWidth: 21; implicitHeight: 21; radius: 6
        color: control.checked ? Theme.accent : Theme.surface2
        border.color: control.visualFocus ? Theme.text : control.checked ? Theme.accent : Theme.border
        Icon { anchors.centerIn: parent; name: "check"; size: 14; visible: control.checked; color: Theme.onAccent }
        Behavior on color { ColorAnimation { duration: Theme.motion ? 130 : 0 } }
    }
    contentItem: Text { text: control.text; leftPadding: control.indicator.width+control.spacing; color: control.enabled ? Theme.text : Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: 12; verticalAlignment: Text.AlignVCenter; wrapMode: Text.Wrap }
}
