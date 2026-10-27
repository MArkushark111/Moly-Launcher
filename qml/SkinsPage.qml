import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: page
    property var shell
    property string importModel: modelChoice.currentIndex === 1 ? "slim" : "classic"
    property real viewYaw: -24
    property real phase: 0
    readonly property bool onlineAccount: launcher.activeAccount.type === "microsoft"
    readonly property bool idleMotion: Theme.motion && launcher.settings.skinIdle !== false
    Timer { interval: 33; repeat: true; running: page.visible && page.idleMotion && !rotateArea.pressed; onTriggered: page.phase += 0.045 }
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 18
        RowLayout {
            Layout.fillWidth: true
            ColumnLayout {
                spacing: 5
                Text { text: "Wardrobe"; color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: 26; font.weight: Font.Bold }
                Text { text: "Your skins, your player."; color: Theme.textSecondary; font.pixelSize: 13 }
            }
            Item { Layout.fillWidth: true }
            MButton {
                text: "Apply to account"
                iconName: "check"
                kind: "primary"
                enabled: page.onlineAccount && !launcher.busy
                onClicked: skinStore.requestApply()
                ToolTip.visible: hovered
                ToolTip.text: page.onlineAccount ? "Update the skin on your Minecraft account" : "Choose a Microsoft Minecraft account in Accounts to apply a skin"
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 18
            Rectangle {
                Layout.preferredWidth: Math.min(340, page.width * 0.39)
                Layout.fillHeight: true
                Layout.minimumHeight: 340
                radius: 22
                color: Theme.surface
                border.color: Theme.border
                clip: true
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 12
                    Text { text: "PLAYER PREVIEW"; color: Theme.textMuted; font.pixelSize: 10; font.letterSpacing: 1.1 }
                    Rectangle {
                        visible: !!launcher.activeAccount.name
                        Layout.alignment: Qt.AlignHCenter
                        implicitWidth: nameTag.implicitWidth + 24
                        implicitHeight: 29
                        radius: 10
                        color: Theme.surface2
                        Text { id: nameTag; anchors.centerIn: parent; text: launcher.activeAccount.name || "";textFormat:Text.PlainText; color: Theme.text; font.pixelSize: 13; font.weight: Font.DemiBold }
                    }
                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumHeight: 205
                        Rectangle { anchors.horizontalCenter: parent.horizontalCenter; anchors.bottom: parent.bottom; anchors.bottomMargin: 8; width: parent.width * 0.6; height: 17; radius: height/2; color: "#151219" }
                        SkinModel {
                            id: playerModel
                            anchors.fill: parent
                            anchors.bottomMargin: 9
                            source: skinStore.selectedSkinUrl
                            slim: skinStore.selectedModel === "slim"
                            yaw: page.viewYaw + (page.idleMotion && !rotateArea.pressed ? Math.sin(page.phase * 0.3) * 1.2 : 0)
                            pose: page.idleMotion ? page.phase : 0
                        }
                        MouseArea {
                            id: rotateArea
                            anchors.fill: parent
                            cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                            property real startX: 0
                            property real startYaw: 0
                            onPressed: function(mouse) { startX=mouse.x; startYaw=page.viewYaw }
                            onPositionChanged: function(mouse) { if(pressed) page.viewYaw=startYaw+(mouse.x-startX)*0.8 }
                            Accessible.name: "Player preview. Drag to rotate."
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Drag to rotate"; color: Theme.textMuted; font.pixelSize: 11; Layout.fillWidth: true }
                        MButton { text: "Reset view"; compact: true; kind: "ghost"; onClicked: page.viewYaw=-24 }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Model"; color: Theme.textSecondary; font.pixelSize: 12; Layout.fillWidth: true }
                        MComboBox {
                            Layout.preferredWidth: 120
                            model: ["Classic", "Slim"]
                            currentIndex: skinStore.selectedModel === "slim" ? 1 : 0
                            enabled: skinStore.skins.length > 0
                            onActivated: skinStore.setSelectedModel(currentIndex === 1 ? "slim" : "classic")
                        }
                    }
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 14
                RowLayout {
                    Layout.fillWidth: true
                    ColumnLayout {
                        spacing: 4
                        Text { text: "Default & uploaded skins"; color: Theme.text; font.pixelSize: 17; font.weight: Font.DemiBold }
                        Text { text: skinStore.skins.length + (skinStore.skins.length === 1 ? " skin" : " skins"); color: Theme.textMuted; font.pixelSize: 11 }
                    }
                    Item { Layout.fillWidth: true }
                    MComboBox { id: modelChoice; model: ["Classic", "Slim"]; implicitWidth: 110; Accessible.name: "Imported skin model" }
                    MButton { text: "Upload skin"; iconName: "plus"; enabled: !launcher.busy; onClicked: shell.chooseFile("skin") }
                }
                Text {
                    visible: skinStore.error.length > 0
                    text: skinStore.error
                    color: Theme.danger
                    font.pixelSize: 12
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
                GridView {
                    id: skinGrid
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    cellWidth: width / Math.max(2, Math.floor(width / 150))
                    cellHeight: 188
                    model: skinStore.skins
                    ScrollBar.vertical: ScrollBar {}
                    delegate: Item {
                        required property var modelData
                        width: skinGrid.cellWidth
                        height: skinGrid.cellHeight
                        ItemDelegate {
                            id: skinCard
                            anchors.fill: parent
                            anchors.rightMargin: 8
                            anchors.bottomMargin: 8
                            padding: 10
                            Accessible.name: modelData.name
                            onClicked: skinStore.selectSkin(modelData.id)
                            background: Rectangle {
                                radius: 17
                                color: skinStore.selectedSkinId === modelData.id ? Theme.accentSoft : skinCard.hovered ? Theme.surface2 : Theme.surface
                                border.color: skinCard.activeFocus ? Theme.textSecondary : Theme.border
                            }
                            contentItem: ColumnLayout {
                                spacing: 5
                                RowLayout {
                                    Layout.fillWidth: true
                                    Text { text: modelData.model === "slim" ? "SLIM" : "CLASSIC"; color: Theme.textMuted; font.pixelSize: 9; font.letterSpacing: 0.6; Layout.fillWidth: true }
                                    Icon { visible: skinStore.selectedSkinId === modelData.id; name: "check"; size: 14; color: Theme.accent }
                                }
                                SkinModel { Layout.fillWidth: true; Layout.fillHeight: true; source: modelData.url; slim: modelData.model === "slim"; yaw: -18 }
                                Text { text: modelData.name; color: Theme.text; font.pixelSize: 12; elide: Text.ElideRight; Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter }
                            }
                        }
                    }
                }
                Text {
                    text: page.onlineAccount ? "Select a saved skin, then Apply to account to update your Minecraft profile." : "Skins are saved locally. Vanilla offline profiles do not use custom skins automatically."
                    color: Theme.textSecondary
                    font.pixelSize: 12
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
                Text { text: "64 × 64 or legacy 64 × 32 PNG · up to 1 MB"; color: Theme.textMuted; font.pixelSize: 11; Layout.fillWidth: true; wrapMode: Text.Wrap }
            }
        }
    }
    DropArea {
        anchors.fill: parent
        keys: ["text/uri-list"]
        onDropped: function(drop) {
            if(launcher.busy||!drop.hasUrls)return
            for(const url of drop.urls) if(url.toString().toLowerCase().endsWith(".png")) {
                skinStore.importSkin(url.toString(),page.importModel)
                drop.acceptProposedAction()
                break
            }
        }
    }
}
