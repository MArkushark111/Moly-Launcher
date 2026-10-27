import QtQuick
import QtQuick.Controls
TextField {
    id: field
    implicitHeight: 38
    color: Theme.text
    placeholderTextColor: Theme.textMuted
    selectionColor: Theme.accent
    selectedTextColor: Theme.text
    font.family: Theme.fontFamily
    font.pixelSize: 13
    leftPadding: 12
    rightPadding: 12
    Accessible.name: placeholderText
    background: Rectangle {
        color: Theme.surface2; radius: 7; border.color: Theme.border
        Rectangle { anchors.fill:parent; anchors.margins:-3; radius:9; color:Theme.transparent; border.color:Theme.accent; border.width:2; visible:field.activeFocus }
    }
}
