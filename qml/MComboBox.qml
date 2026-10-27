import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ComboBox {
    id: control
    property bool providerIcons: false
    implicitHeight: 40
    implicitWidth: 160
    leftPadding: providerIcons ? 40 : 13
    rightPadding: 30
    font.family: Theme.fontFamily; font.pixelSize: 12
    contentItem: Text {
        text: control.displayText; color: control.enabled ? Theme.text : Theme.textMuted
        font: control.font; verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight
    }
    ProviderIcon { visible: control.providerIcons; provider: control.currentText; x: 13; anchors.verticalCenter: parent.verticalCenter; width: 18; height: 18 }
    indicator: Icon { name: "down"; size: 14; color: Theme.textSecondary; x: control.width-width-11; anchors.verticalCenter: parent.verticalCenter; rotation: control.popup.visible ? 180 : 0; Behavior on rotation { NumberAnimation { duration: Theme.motion ? 150 : 0 } } }
    background: Rectangle { radius: 11; color: control.hovered ? Theme.surface2 : Theme.surface; border.color: control.visualFocus ? Theme.accent : Theme.border; opacity: control.enabled ? 1 : 0.6; Behavior on color { ColorAnimation { duration: Theme.motion ? 140 : 0 } } }
    delegate: ItemDelegate {
        id: option
        required property int index
        required property var model
        width: control.width-12; height: 38
        highlighted: control.highlightedIndex === index
        property string optionText: control.textRole ? String(model[control.textRole] || "") : String(model.modelData)
        contentItem: RowLayout {
            spacing: 10
            ProviderIcon { visible: control.providerIcons; provider: option.optionText; Layout.preferredWidth: 18; Layout.preferredHeight: 18 }
            Text { text: option.optionText; color: Theme.text; font: control.font; elide: Text.ElideRight; Layout.fillWidth: true; verticalAlignment: Text.AlignVCenter }
            Icon { name: "check"; size: 14; color: Theme.accent; visible: option.index === control.currentIndex }
        }
        background: Rectangle { radius: 8; color: option.highlighted ? Theme.accentSoft : option.hovered ? Theme.surface2 : Theme.transparent }
    }
    popup: Popup {
        y: control.height+5; width: control.width; padding: 6
        implicitHeight: Math.min(contentItem.implicitHeight+12, 300)
        background: Rectangle { radius: 13; color: Theme.surface; border.color: Theme.border }
        contentItem: ListView { clip: true; implicitHeight: contentHeight; model: control.popup.visible ? control.delegateModel : null; currentIndex: control.highlightedIndex; ScrollIndicator.vertical: ScrollIndicator {} }
        enter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.motion ? 130 : 0 } }
        exit: Transition { NumberAnimation { property: "opacity"; to: 0; duration: Theme.motion ? 90 : 0 } }
    }
}
