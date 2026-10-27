import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ItemDelegate {
    id: control
    property string label: ""
    property string iconName: ""
    property bool selected: false
    implicitHeight: 38
    Accessible.name: label
    leftPadding: 13
    rightPadding: 13
    topPadding: 0
    bottomPadding: 0

    background: Rectangle {
        radius: height / 3
        color: control.selected ? Theme.accentSoft : control.hovered ? Theme.surface2 : Theme.transparent
        border.color: control.selected ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.18) : Theme.transparent
        Behavior on color {
            ColorAnimation {
                duration: Theme.motion ? 150 : 0
            }
        }
        Rectangle {
            anchors.fill: parent
            anchors.margins: -2
            radius: parent.radius + 2
            color: Theme.transparent
            border.width: 2
            border.color: Theme.accent
            visible: control.activeFocus
        }
    }
    contentItem: RowLayout {
        spacing: 11
        Icon {
            name: control.iconName
            size: 19
            color: control.selected ? Theme.accent : Theme.textSecondary
        }
        Text {
            text: control.label
            color: control.selected ? Theme.text : Theme.textSecondary
            font.family: Theme.fontFamily
            font.pixelSize: 14
            font.weight: control.selected ? Font.DemiBold : Font.Medium
            Layout.fillWidth: true
        }
        Rectangle {
            width: 5
            height: 5
            radius: 3
            color: Theme.accent
            visible: control.selected
            Layout.alignment: Qt.AlignVCenter
        }
    }
}
