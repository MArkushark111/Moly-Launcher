import QtQuick
import QtQuick.Controls

SpinBox {
    id: control
    implicitWidth: 128; implicitHeight: 38
    leftPadding: 33; rightPadding: 33
    font.family: Theme.fontFamily; font.pixelSize: 12
    contentItem: TextInput {
        text: control.displayText
        color: Theme.text; font: control.font
        horizontalAlignment: TextInput.AlignHCenter; verticalAlignment: TextInput.AlignVCenter
        readOnly: !control.editable; validator: control.validator
        inputMethodHints: Qt.ImhDigitsOnly
        onEditingFinished: control.value=control.valueFromText(text,control.locale)
    }
    background: Rectangle { radius: 10; color: Theme.surface2; border.color: control.visualFocus ? Theme.accent : Theme.border }
    up.indicator: Rectangle {
        x: control.width-width-3; y: 3; width: 27; height: control.height-6; radius: 7
        color: control.up.hovered ? Theme.accentSoft : Theme.surface
        Text { anchors.centerIn:parent;text:"+";color:control.up.pressed?Theme.accent:Theme.textSecondary;font.pixelSize:16 }
    }
    down.indicator: Rectangle {
        x: 3; y: 3; width: 27; height: control.height-6; radius: 7
        color: control.down.hovered ? Theme.accentSoft : Theme.surface
        Text { anchors.centerIn:parent;text:"−";color:control.down.pressed?Theme.accent:Theme.textSecondary;font.pixelSize:16 }
    }
}
