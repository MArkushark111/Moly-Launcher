import QtQuick

Rectangle {
    id: avatar
    property string imageSource: ""
    property string label: ""
    implicitWidth: 40
    implicitHeight: 40
    radius: Math.min(16, width * 0.26)
    color: Theme.surface2
    border.color: Theme.border
    MediaImage {
        id: picture
        anchors.fill: parent
        anchors.margins: parent.width >= 48 ? 6 : 3
        source: avatar.imageSource
        fillMode: Image.PreserveAspectFit
    }
    Icon {
        anchors.centerIn: parent
        visible: !avatar.imageSource || picture.status === Image.Error
        name: "box"
        size: Math.round(parent.width * 0.48)
        color: Theme.textSecondary
    }
}
