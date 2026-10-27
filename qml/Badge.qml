import QtQuick
Rectangle {
    id: badge
    property string text: ""
    property color tint: Theme.textSecondary
    implicitWidth: label.implicitWidth + 14
    implicitHeight: 22
    radius: 5
    color: Qt.rgba(tint.r,tint.g,tint.b,0.08)
    Text { id:label; anchors.centerIn:parent; text:badge.text; color:badge.tint; font.family:Theme.fontFamily; font.pixelSize:11; font.weight:Font.Medium }
}
