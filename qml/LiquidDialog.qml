import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: popup
    modal: true
    padding: 24
    topPadding: 18
    closePolicy: Popup.CloseOnEscape
    property bool showCloseButton: true
    background: Rectangle {
        radius: 24
        color: Theme.surface
        border.color: Theme.border
        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: 23
            color: Theme.transparent
            border.color: Qt.rgba(Theme.text.r, Theme.text.g, Theme.text.b, 0.035)
        }
    }
    header: Item {
        implicitHeight: 58
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 24
            anchors.rightMargin: 18
            anchors.topMargin: 12
            spacing: 12
            Text {
                text: popup.title
                color: Theme.text
                font.family: Theme.fontFamily
                font.pixelSize: 17
                font.weight: Font.DemiBold
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
            MButton {
                visible: popup.showCloseButton
                iconName: "close"
                kind: "ghost"
                compact: true
                Accessible.name: "Close dialog"
                onClicked: popup.reject()
            }
        }
    }
    enter: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.motion ? 160 : 0 }
            NumberAnimation { property: "scale"; from: Theme.motion ? 0.95 : 1; to: 1; duration: Theme.motion ? 260 : 0; easing.type: Easing.OutBack; easing.overshoot: 0.8 }
        }
    }
    exit: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 1; to: 0; duration: Theme.motion ? 120 : 0 }
            NumberAnimation { property: "scale"; from: 1; to: Theme.motion ? 0.97 : 1; duration: Theme.motion ? 120 : 0; easing.type: Easing.InQuad }
        }
    }
}
