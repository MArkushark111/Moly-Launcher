import QtQuick
import QtQuick.Layouts

ColumnLayout {
    id: panel
    property var items: []
    property var shell
    property color pingColor: Theme.accent
    property bool loading: false
    property bool checkingStatus: false
    property bool showInstanceName: true
    property bool showTitle: true
    property bool showRefresh: launcher.settings.showQuickAccessRefresh!==false
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
        MButton {text:"Add server";iconName:"plus";kind:"ghost";compact:true;enabled:!!launcher.selectedId;onClicked:panel.shell.addServer()}
        MButton {iconName:"more";kind:"ghost";compact:true;Accessible.name:"Quick-access options";onClicked:layoutMenu.open();MMenu{id:layoutMenu;MMenuItem{text:"Restore hidden shortcuts and default order";onTriggered:playTargets.resetLayout()}}}
        MButton { visible:panel.showRefresh;text: "Refresh"; iconName: "refresh"; kind: "ghost"; compact: true; enabled: !panel.loading; onClicked: panel.refreshRequested() }
    }
    Repeater {
        model: panel.visibleItems
        delegate: Rectangle {
            id: targetRow
            required property var modelData
            Layout.fillWidth: true
            implicitHeight: modelData.type==="server"&&launcher.settings.showServerMotd===true?92:78
            radius: 14
            color: rowHover.hovered ? Theme.surface2 : Theme.surface
            border.color: Theme.border
            Behavior on color { ColorAnimation { duration: Theme.motion ? 130 : 0 } }
            HoverHandler { id: rowHover }
            DropArea {anchors.fill:parent;keys:["moly-target"];onDropped:function(drop){if(drop.source&&drop.source.instanceId!==targetRow.modelData.id){const id=drop.source.instanceId,before=targetRow.modelData.id,after=drop.y>height/2;Qt.callLater(function(){playTargets.moveTarget(id,before,after)});drop.acceptProposedAction()}};Rectangle{visible:parent.containsDrag;width:parent.width;height:3;radius:2;color:Theme.accent}}
            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 12
                ReorderHandle{instanceId:targetRow.modelData.id;targetHandle:true;Layout.preferredWidth:18;Layout.preferredHeight:30}
                Rectangle {
                    Layout.preferredWidth: 48; Layout.preferredHeight: 48
                    radius: 11; color: Theme.surface2
                    ArtworkView { id:targetPicture;anchors.fill:parent;radius:11;shade:0;source:targetRow.modelData.iconUrl||"";animated:false }
                    Icon { anchors.centerIn: parent; name: targetRow.modelData.type === "world" ? "world" : "servers"; size: 23; visible: !targetPicture.ready }
                }
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 4
                    Text { text: targetRow.modelData.name || ""; color: Theme.text; font.pixelSize: 14; font.weight: Font.DemiBold; elide: Text.ElideRight; Layout.fillWidth: true; textFormat: Text.PlainText }
                    Text {
                        text: (panel.showInstanceName ? (targetRow.modelData.instanceName || "") + " · " : "") + (targetRow.modelData.type === "world" ? (targetRow.modelData.subtitle || "Singleplayer") : (launcher.settings.showServerAddress===false ? "Multiplayer" : targetRow.modelData.address || ""))
                        color: Theme.textMuted; font.pixelSize: 11; elide: Text.ElideRight; Layout.fillWidth: true; textFormat: Text.PlainText
                    }
                    Text {visible:launcher.settings.showServerMotd===true&&targetRow.modelData.type==="server"&&!!targetRow.modelData.motd;text:(targetRow.modelData.motd||"").split(/\r?\n/).filter(function(line){return line.trim().length>0})[0]||"";color:Theme.textSecondary;font.pixelSize:11;elide:Text.ElideRight;Layout.fillWidth:true;textFormat:Text.PlainText}
                }
                ColumnLayout {
                    visible: targetRow.modelData.type === "server"
                    Layout.preferredWidth: 108;Layout.alignment:Qt.AlignRight
                    spacing: 5
                    RowLayout {
                        Layout.alignment:Qt.AlignRight;spacing: 6
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
                    Text { text: targetRow.modelData.status === "online" ? targetRow.modelData.onlinePlayers + " / " + targetRow.modelData.maxPlayers + " online" : ""; color: Theme.textMuted; font.pixelSize: 10; textFormat: Text.PlainText;Layout.alignment:Qt.AlignRight }
                }
                MButton {iconName:"more";kind:"ghost";compact:true;Accessible.name:"Options for "+targetRow.modelData.name;onClicked:targetMenu.open()
                    MMenu {id:targetMenu
                        MMenuItem{visible:targetRow.modelData.type==="server";text:"Edit server shortcut";iconName:"settings";onTriggered:panel.shell.editServer(targetRow.modelData)}
                        MMenuItem{visible:targetRow.modelData.type==="server";text:"Check connection";iconName:"refresh";onTriggered:playTargets.refreshServer(targetRow.modelData.address)}
                        MMenuItem{visible:targetRow.modelData.type==="server";text:"Copy address";iconName:"copy";onTriggered:playTargets.copyAddress(targetRow.modelData.address)}
                        MMenuItem{text:"Remove from Jump in";iconName:"trash";destructive:true;onTriggered:playTargets.hideTarget(targetRow.modelData.id)}
                    }
                }
                MButton { visible: panel.allowWorldIconChanges && targetRow.modelData.type === "world"; text: ""; iconName: "file"; kind: "secondary"; compact: true; background:Rectangle {radius:10;color:rowHover.hovered?Theme.app:Theme.surface;border.color:Theme.border;Behavior on color {ColorAnimation {duration:Theme.motion?160:0}}} enabled: panel.actionsEnabled; Accessible.name: "Change world icon for " + targetRow.modelData.name; onClicked: panel.changeWorldIcon(targetRow.modelData) }
                MButton { loading:launcher.busy&&launcher.taskOperation==="launch"&&launcher.taskInstanceId===targetRow.modelData.instanceId;text:loading?"Starting":targetRow.modelData.type === "server" ? "Join" : "Play"; iconName: "play"; compact: true; kind: "primary"; enabled: panel.actionsEnabled; onClicked: panel.activated(targetRow.modelData); Accessible.name: (targetRow.modelData.type === "server" ? "Join " : "Play ") + targetRow.modelData.name }
            }
        }
    }
    Text {
        visible: panel.items.length === 0
        text: panel.loading ? "Reading saved worlds and servers…" : "Your saved worlds and Minecraft servers will appear here."
        Layout.fillWidth: true; color: Theme.textMuted; font.pixelSize: 12; wrapMode: Text.Wrap; textFormat: Text.PlainText
    }
}
