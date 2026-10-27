import QtQuick
import QtQuick.Layouts

ColumnLayout {
    id: panel
    property var items: []
    property color pingColor: Theme.accent
    property bool loading: false
    property bool checkingStatus: false
    property bool showInstanceName: true
    property bool showTitle: true
    property string title: "Jump in"
    property int maximumRows: 5
    property bool allowWorldIconChanges: true
    property bool actionsEnabled: true
    signal activated(var entry)
    signal changeWorldIcon(var entry)
    signal refreshRequested()
    readonly property var visibleItems: maximumRows > 0 ? items.slice(0, maximumRows) : items
    spacing: 10
    RowLayout {
        visible: panel.showTitle
        Layout.fillWidth: true
        Text { text: panel.title; color: Theme.text; font.pixelSize: 17; font.weight: Font.DemiBold; Layout.fillWidth: true; textFormat: Text.PlainText }
        Text { visible: panel.checkingStatus; text: "Checking servers…"; color: Theme.textMuted; font.pixelSize: 11 }
        MButton { text: "Refresh"; iconName: "refresh"; kind: "ghost"; compact: true; enabled: !panel.loading; onClicked: panel.refreshRequested() }
    }
    Repeater {
        model: panel.visibleItems
        delegate: Rectangle {
            id: targetRow
            required property var modelData
            Layout.fillWidth: true
            implicitHeight: 78
            radius: 14
            color: rowHover.hovered ? Theme.surface2 : Theme.surface
            border.color: Theme.border
            Behavior on color { ColorAnimation { duration: Theme.motion ? 130 : 0 } }
            HoverHandler { id: rowHover }
            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 12
                Rectangle {
                    Layout.preferredWidth: 48; Layout.preferredHeight: 48
                    radius: 11; color: Theme.surface2
                    Image { id: targetIcon; anchors.fill: parent; anchors.margins: 3; source: targetRow.modelData.iconUrl || ""; asynchronous: true; sourceSize.width: 96; sourceSize.height: 96; fillMode: Image.PreserveAspectFit; visible: status === Image.Ready }
                    Icon { anchors.centerIn: parent; name: targetRow.modelData.type === "world" ? "world" : "servers"; size: 23; visible: targetIcon.status !== Image.Ready }
                }
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 4
                    Text { text: targetRow.modelData.name || ""; color: Theme.text; font.pixelSize: 14; font.weight: Font.DemiBold; elide: Text.ElideRight; Layout.fillWidth: true; textFormat: Text.PlainText }
                    Text {
                        text: (panel.showInstanceName ? (targetRow.modelData.instanceName || "") + " · " : "") + (targetRow.modelData.type === "world" ? (targetRow.modelData.subtitle || "Singleplayer") : (targetRow.modelData.address || ""))
                        color: Theme.textMuted; font.pixelSize: 11; elide: Text.ElideRight; Layout.fillWidth: true; textFormat: Text.PlainText
                    }
                }
                ColumnLayout {
                    visible: targetRow.modelData.type === "server"
                    Layout.preferredWidth: panel.width > 700 ? 155 : 120
                    spacing: 5
                    RowLayout {
                        spacing: 6
                        Item {
                            Layout.preferredWidth: 22; Layout.preferredHeight: 16
                            readonly property int bars: targetRow.modelData.status === "online" ? targetRow.modelData.latencyMs < 100 ? 4 : targetRow.modelData.latencyMs < 250 ? 3 : targetRow.modelData.latencyMs < 600 ? 2 : 1 : 0
                            Repeater {
                                model: 4
                                Rectangle { required property int index; x: index * 5; y: 14 - height; width: 3; height: 4 + index * 3; radius: 1.5; color: index < parent.bars ? panel.pingColor : Theme.border }
                            }
                        }
                        Text { text: targetRow.modelData.status === "online" ? targetRow.modelData.latencyMs + " ms" : targetRow.modelData.status === "checking" ? "Checking…" : targetRow.modelData.status === "offline" ? "Unavailable" : "Not checked"; color: targetRow.modelData.status === "online" ? Theme.textSecondary : Theme.textMuted; font.pixelSize: 11; textFormat: Text.PlainText }
                    }
                    Text { text: targetRow.modelData.status === "online" ? targetRow.modelData.onlinePlayers + " / " + targetRow.modelData.maxPlayers + " online" : ""; color: Theme.textMuted; font.pixelSize: 10; textFormat: Text.PlainText }
                }
                MButton { visible: panel.allowWorldIconChanges && targetRow.modelData.type === "world"; text: ""; iconName: "file"; kind: "ghost"; compact: true; enabled: panel.actionsEnabled; Accessible.name: "Change world icon for " + targetRow.modelData.name; onClicked: panel.changeWorldIcon(targetRow.modelData) }
                MButton { text: targetRow.modelData.type === "server" ? "Join" : "Play"; iconName: "play"; compact: true; kind: "primary"; enabled: panel.actionsEnabled; onClicked: panel.activated(targetRow.modelData); Accessible.name: (targetRow.modelData.type === "server" ? "Join " : "Play ") + targetRow.modelData.name }
            }
        }
    }
    Text {
        visible: panel.items.length === 0
        text: panel.loading ? "Reading saved worlds and servers…" : "Your saved worlds and Minecraft servers will appear here."
        Layout.fillWidth: true; color: Theme.textMuted; font.pixelSize: 12; wrapMode: Text.Wrap; textFormat: Text.PlainText
    }
}
