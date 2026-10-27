import QtQuick
import QtQuick.Controls

MenuItem {
    id: control
    property string iconName: ""
    property bool destructive: false
    implicitHeight: 39
    leftPadding: 12; rightPadding: 12
    contentItem: Row {
        spacing: 10
        Icon { visible: !!control.iconName; name: control.iconName; size: 16; color: control.destructive ? Theme.danger : Theme.textSecondary; anchors.verticalCenter: parent.verticalCenter }
        Text { text: control.text; color: !control.enabled ? Theme.textMuted : control.destructive ? Theme.danger : Theme.text; font.family: Theme.fontFamily; font.pixelSize: 12; verticalAlignment: Text.AlignVCenter; anchors.verticalCenter: parent.verticalCenter }
    }
    background: Rectangle { radius: 9; color: control.highlighted ? (control.destructive ? Qt.rgba(Theme.danger.r,Theme.danger.g,Theme.danger.b,0.12) : Theme.accentSoft) : Theme.transparent }
}
