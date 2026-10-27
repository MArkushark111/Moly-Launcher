import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: page
    property var shell
    property bool gridMode: launcher.settings.libraryGrid === true
    readonly property var selectedInstance: launcher.selected
    readonly property var visibleInstances: {
        const needle = searchField.text.toLowerCase().trim();
        const list = launcher.instances.filter(function (i) {
            return (i.name + " " + i.loader + " " + i.version + " " + i.group).toLowerCase().indexOf(needle) !== -1;
        });
        const sort = sortChoice.currentIndex;
        if(sort===0)return list;
        list.sort(function (a, b) {
            if (sort === 2)
                return new Date(b.lastPlayed || 0).getTime() - new Date(a.lastPlayed || 0).getTime();
            if (sort === 3)
                return (a.loader + a.version).localeCompare(b.loader + b.version);
            return a.name.localeCompare(b.name);
        });
        return list;
    }
    function openInstance(i) {
        if(i._pending){shell.showPendingInstance();return;}
        launcher.selectInstance(i.id);
        shell.navigate("instance");
    }
    function playInstance(i) {
        if(i._pending){shell.showPendingInstance();return;}
        launcher.selectInstance(i.id);
        if (i.installed && !i.running)
            launcher.play();
        else
            shell.navigate("instance");
    }
    function lastPlayed(value) {
        if (!value)
            return "Never played";
        const date = new Date(value);
        if (isNaN(date.getTime()))
            return "Never played";
        const days = Math.floor(Math.max(0, Date.now() - date.getTime()) / 86400000);
        return days === 0 ? "Today" : days === 1 ? "Yesterday" : days < 30 ? days + " days ago" : date.toLocaleDateString(Qt.locale(), "dd MMM yyyy");
    }

    ScrollView {
        id: libraryScroll
        anchors.fill: parent
        anchors.margins: 24
        clip: true
        contentWidth: availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
    ColumnLayout {
        width: libraryScroll.availableWidth
        spacing: 16
        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            ColumnLayout {
                spacing: 4
                Text {
                    text: "Library"
                    color: Theme.text
                    font.family: Theme.fontFamily
                    font.pixelSize: 26
                    font.weight: Font.Bold
                    font.letterSpacing: -0.5
                }
                Text {
                    text: launcher.instances.length + (launcher.instances.length === 1 ? " instance" : " instances")
                    color: Theme.textSecondary
                    font.pixelSize: 12
                }
            }
            Item {
                Layout.fillWidth: true
            }
            MButton {
                text: "Import"
                implicitHeight: 36
                enabled: !launcher.busy
                onClicked: importMenu.open()
                MMenu {
                    id: importMenu
                    MMenuItem {
                        text: "Modrinth or CurseForge pack"
                        onTriggered: shell.chooseFile("pack")
                    }
                    MMenuItem {
                        text: "Import instance from this computer"
                        onTriggered: shell.importInstances()
                    }
                }
            }
            MButton {
                text: "Create instance"
                kind: "primary"
                implicitHeight: 36
                enabled: !launcher.busy
                onClicked: shell.create()
            }
        }
        PlayTargets {
            visible: launcher.settings.quickAccess !== false && launcher.instances.length > 0
            Layout.fillWidth: true
            title: "Jump back in"
            items: { const all=playTargets.items; const worlds=all.filter(function(i){return i.type==="world"});const servers=all.filter(function(i){return i.type==="server"});return worlds.length&&servers.length?[worlds[0],servers[0]].concat(worlds.slice(1),servers.slice(1)):all }
            loading: playTargets.loading
            checkingStatus: playTargets.checkingStatus
            maximumRows: launcher.settings.quickAccessRows||3
            pingColor: Theme.serverAccent
            actionsEnabled: !launcher.busy
            onActivated: function(entry) { page.shell.launchTarget(entry) }
            onChangeWorldIcon: function(entry) { page.shell.chooseWorldIcon(entry) }
            onRefreshRequested: playTargets.refresh()
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            MField {
                id: searchField
                Layout.fillWidth: true
                implicitHeight: 36
                placeholderText: "Search instances…"
                Accessible.name: "Search instances"
            }
            MComboBox {
                id: sortChoice
                Layout.preferredWidth: 156
                implicitHeight: 36
                model: ["My order", "Name (A-Z)", "Last played", "Loader"]
                currentIndex:Math.max(0,["manual","name","recent","loader"].indexOf(launcher.settings.librarySort||"manual"))
                onActivated:launcher.setUiPreferences({librarySort:["manual","name","recent","loader"][currentIndex]})
                Accessible.name: "Sort instances"
            }
            MButton {
                text: "List"
                implicitWidth: 54
                implicitHeight: 36
                kind: !page.gridMode ? "primary" : "secondary"
                onClicked: { page.gridMode = false; launcher.setUiPreferences({libraryGrid:false}) }
                Accessible.name: "List view"
            }
            MButton {
                text: "Grid"
                implicitWidth: 54
                implicitHeight: 36
                kind: page.gridMode ? "primary" : "secondary"
                onClicked: { page.gridMode = true; launcher.setUiPreferences({libraryGrid:true}) }
                Accessible.name: "Grid view"
            }
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: page.visibleInstances.length === 0 ? 220 : Math.min(Math.max(180, page.height - 290), page.gridMode ? Math.ceil(page.visibleInstances.length / Math.max(2, Math.floor(width / 260))) * 160 : 36 + page.visibleInstances.length * 64 + 4)
            radius: 16
            color: Theme.surface
            border.color: Theme.border
            ColumnLayout {
                anchors.fill: parent
                spacing: 0
                Rectangle {
                    visible: !page.gridMode && page.visibleInstances.length > 0
                    Layout.fillWidth: true
                    Layout.preferredHeight: 36
                    color: "transparent"
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16
                        spacing: 12
                        Text {
                            text: "INSTANCE"
                            color: Theme.textMuted
                            font.pixelSize: 10
                            font.weight: Font.DemiBold
                            font.letterSpacing: 0.7
                            Layout.fillWidth: true
                            leftPadding: 48
                        }
                        Text {
                            text: "VERSION"
                            color: Theme.textMuted
                            font.pixelSize: 10
                            font.weight: Font.DemiBold
                            font.letterSpacing: 0.7
                            Layout.preferredWidth: 156
                        }
                        Text {
                            visible: launcher.settings.showModCounts !== false
                            text: "MODS"
                            color: Theme.textMuted
                            font.pixelSize: 10
                            font.weight: Font.DemiBold
                            font.letterSpacing: 0.7
                            Layout.preferredWidth: 48
                        }
                        Text {
                            visible: page.width > 850
                            text: "LAST PLAYED"
                            color: Theme.textMuted
                            font.pixelSize: 10
                            font.weight: Font.DemiBold
                            font.letterSpacing: 0.7
                            Layout.preferredWidth: 102
                        }
                        Item {
                            Layout.preferredWidth: 104
                        }
                    }
                    Rectangle {
                        anchors.bottom: parent.bottom
                        width: parent.width
                        height: 1
                        color: Theme.border
                    }
                }
                ListView {
                    id: instanceList
                    visible: !page.gridMode
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: page.visibleInstances
                    clip: true
                    ScrollBar.vertical: ScrollBar {}
                    delegate: ItemDelegate {
                        id: row
                        required property var modelData
                        width: ListView.view.width
                        height: 64
                        padding: 0
                        hoverEnabled: true
                        Accessible.name: modelData.name + ", " + modelData.loader + " " + modelData.version
                        onClicked: {if(modelData._pending)shell.showPendingInstance();else launcher.selectInstance(modelData.id)}
                        onDoubleClicked: page.openInstance(modelData)
                        background: Rectangle {
                            anchors.fill: parent
                            anchors.margins: 4
                            radius: 12
                            color: row.modelData.installing ? Theme.app : launcher.selectedId === row.modelData.id ? Theme.accentSoft : row.hovered ? Theme.surface2 : "transparent"
                            border.width: row.activeFocus ? 2 : 0
                            border.color: Theme.accent
                            Behavior on color {
                                ColorAnimation {
                                    duration: launcher.settings.reducedMotion ? 0 : 150
                                }
                            }
                        }
                        DropArea {anchors.fill:parent;keys:["moly-instance"];onDropped:function(drop){if(drop.source&&!row.modelData._pending){const id=drop.source.instanceId,target=row.modelData.id,after=drop.y>height/2;Qt.callLater(function(){launcher.moveInstance(id,target,after)});drop.acceptProposedAction()}};Rectangle {visible:parent.containsDrag;anchors.top:parent.top;anchors.margins:4;width:parent.width;height:3;radius:2;color:Theme.accent}}
                        contentItem: Item {
                            RowLayout {
                                height: 40
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 12
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.leftMargin: 16
                                anchors.rightMargin: 16
                                ReorderHandle {instanceId:row.modelData.id;available:!row.modelData._pending;Layout.preferredWidth:18;Layout.preferredHeight:30}
                                InstanceAvatar {
                                    opacity:row.modelData.installing?0.45:1
                                    Layout.preferredWidth: 36
                                    Layout.preferredHeight: 36
                                    width: 36
                                    height: 36
                                    Layout.alignment: Qt.AlignVCenter
                                    imageSource: row.modelData.iconUrl || ""
                                }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: false
                                    Layout.alignment: Qt.AlignVCenter
                                    spacing: 3
                                    Text {
                                        text: row.modelData.name
                                        color: Theme.text
                                        font.family: Theme.fontFamily
                                        font.pixelSize: 14
                                        font.weight: Font.DemiBold
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                    Text {
                                        text: row.modelData.installing ? "Installing…" : row.modelData._pending ? row.modelData.status : row.modelData.running ? "Running" : row.modelData.group || "My instances"
                                        color: row.modelData.running ? Theme.success : Theme.textMuted
                                        font.pixelSize: 11
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                }
                                ColumnLayout {
                                    Layout.preferredWidth: 156
                                    Layout.fillHeight: false
                                    Layout.alignment: Qt.AlignVCenter
                                    spacing: 3
                                    Text {
                                        text: row.modelData.loader
                                        color: Theme.textSecondary
                                        font.pixelSize: 12
                                    }
                                    Text {
                                        text: row.modelData.version
                                        color: Theme.textMuted
                                        font.pixelSize: 11
                                    }
                                }
                                Text {
                                    visible: launcher.settings.showModCounts !== false
                                    text: row.modelData.modCount !== undefined ? row.modelData.modCount : "—"
                                    color: Theme.textSecondary
                                    font.pixelSize: 12
                                    Layout.preferredWidth: 48
                                }
                                Text {
                                    visible: page.width > 850
                                    text: page.lastPlayed(row.modelData.lastPlayed)
                                    color: Theme.textMuted
                                    font.pixelSize: 12
                                    Layout.preferredWidth: 102
                                }
                                Item {
                                    Layout.preferredWidth: 104
                                    Layout.preferredHeight: 32
                                    MButton {
                                        anchors.fill: parent
                                        loading: row.modelData.installing || (launcher.busy && launcher.taskInstanceId === row.modelData.id)
                                        text: loading ? (launcher.taskOperation === "launch" ? "Starting" : "Install") : row.modelData.running ? "Open" : row.modelData.installed ? "Play" : "Set up"
                                        kind: row.modelData.running || !row.modelData.installed ? "secondary" : "primary"
                                        opacity: 1
                                        enabled: !launcher.busy
                                        onClicked: page.playInstance(row.modelData)
                                        Accessible.name: "Play " + row.modelData.name
                                        Behavior on opacity {
                                            NumberAnimation {
                                                duration: launcher.settings.reducedMotion ? 0 : 150
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                GridView {
                    id: instanceGrid
                    visible: page.gridMode
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: page.visibleInstances
                    clip: true
                    cellWidth: width / Math.max(2, Math.floor(width / 260))
                    cellHeight: 160
                    ScrollBar.vertical: ScrollBar {}
                    delegate: Item {
                        required property var modelData
                        width: instanceGrid.cellWidth
                        height: instanceGrid.cellHeight
                        ItemDelegate {
                            id: card
                            anchors.fill: parent
                            anchors.margins: 8
                            padding: 16
                            hoverEnabled: true
                            scale: hovered && Theme.motion ? 1.015 : 1
                            onClicked: {if(modelData._pending)shell.showPendingInstance();else launcher.selectInstance(modelData.id)}
                            onDoubleClicked: page.openInstance(modelData)
                            Accessible.name: modelData.name
                            Behavior on scale {
                                NumberAnimation {
                                    duration: Theme.motion ? 190 : 0
                                    easing.type: Easing.OutCubic
                                }
                            }
                            background: Rectangle {
                                radius: 18
                                color: modelData.installing ? Theme.app : launcher.selectedId === modelData.id ? Theme.accentSoft : card.hovered ? Theme.surface2 : Theme.app
                                border.color: card.activeFocus ? Theme.accent : card.hovered ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.38) : Theme.border
                                border.width: card.activeFocus ? 2 : 1
                                ArtworkView {anchors.fill:parent;radius:18;source:modelData.backgroundUrl||"";opacity:modelData.installing?0.4:1;shade:launcher.settings.backgroundDim===undefined?0.5:launcher.settings.backgroundDim;animated:Theme.motion&&launcher.settings.animateArtwork!==false}
                                Behavior on color {
                                    ColorAnimation {
                                        duration: Theme.motion ? 160 : 0
                                    }
                                }
                                Behavior on border.color {
                                    ColorAnimation {
                                        duration: Theme.motion ? 160 : 0
                                    }
                                }
                            }
                            DropArea {anchors.fill:parent;keys:["moly-instance"];onDropped:function(drop){if(drop.source&&!modelData._pending){const id=drop.source.instanceId,target=modelData.id,after=drop.y>height/2;Qt.callLater(function(){launcher.moveInstance(id,target,after)});drop.acceptProposedAction()}};Rectangle {visible:parent.containsDrag;anchors.top:parent.top;width:parent.width;height:3;radius:2;color:Theme.accent}}
                            contentItem: ColumnLayout {
                                spacing: 10
                                RowLayout {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 36
                                    InstanceAvatar {
                                        Layout.preferredWidth: 32
                                        Layout.preferredHeight: 32
                                        width: 32
                                        height: 32
                                        imageSource: modelData.iconUrl || ""
                                        opacity:modelData.installing?0.45:1
                                    }
                                    Text {
                                        text: modelData.name
                                        color: Theme.text
                                        font.pixelSize: 14
                                        font.weight: Font.DemiBold
                                        verticalAlignment: Text.AlignVCenter
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                }
                                Text {
                                    text: modelData.loader + " " + modelData.version
                                    color: Theme.textSecondary
                                    font.pixelSize: 12
                                }
                                RowLayout {
                                    Text {
                                        text: modelData.installing ? "Installing…" : modelData._pending ? modelData.status : modelData.running ? "Running" : page.lastPlayed(modelData.lastPlayed)
                                        color: modelData.running ? Theme.success : Theme.textMuted
                                        font.pixelSize: 11
                                        Layout.fillWidth: true
                                    }
                                    ReorderHandle {instanceId:modelData.id;available:!modelData._pending;Layout.preferredWidth:20;Layout.preferredHeight:28}
                                    MButton {
                                        loading: modelData.installing || (launcher.busy && launcher.taskInstanceId === modelData.id)
                                        text: loading ? "Starting" : modelData.running ? "Open" : modelData.installed ? "Play" : "Set up"
                                        kind: modelData.installed && !modelData.running ? "primary" : "secondary"
                                        implicitHeight: 30
                                        implicitWidth: 104
                                        enabled: !launcher.busy
                                        onClicked: page.playInstance(modelData)
                                    }
                                }
                            }
                        }
                    }
                }
            }
            ColumnLayout {
                visible: page.visibleInstances.length === 0
                anchors.centerIn: parent
                spacing: 12
                Icon {
                    name: "library"
                    size: 32
                    color: Theme.textMuted
                    Layout.alignment: Qt.AlignHCenter
                }
                Text {
                    text: searchField.text ? "No matching instances" : "No instances yet"
                    color: Theme.text
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                    Layout.alignment: Qt.AlignHCenter
                }
                Text {
                    text: searchField.text ? "Try a different name, version, or loader." : "Create a Minecraft setup or import a modpack."
                    color: Theme.textSecondary
                    font.pixelSize: 13
                    Layout.alignment: Qt.AlignHCenter
                }
                RowLayout {
                    visible: !searchField.text
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 8
                    MButton {
                        text: "Create instance"
                        kind: "primary"
                        implicitHeight: 34
                        enabled: !launcher.busy
                        onClicked: shell.create()
                    }
                    MButton {
                        text: "Browse modpacks"
                        implicitHeight: 34
                        onClicked: shell.browse("modpack")
                    }
                }
            }
        }
        Text {
            visible: !!page.selectedInstance.id
            text: "Drag the six-dot handle to arrange instances. Double-click to manage; drop a .jar file to add a mod."
            color: Theme.textMuted
            font.pixelSize: 11
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }
    }
    }
    DropArea {
        id: dropArea
        anchors.fill: parent
        keys: ["text/uri-list"]
        onDropped: function (drop) {
            if (!drop.hasUrls || !launcher.selectedId || launcher.busy)
                return;
            for (let i = 0; i < drop.urls.length; ++i) {
                const path = drop.urls[i].toString();
                if (path.toLowerCase().endsWith(".jar")) {
                    launcher.importFile(path, "mod");
                    drop.acceptProposedAction();
                    break;
                }
            }
        }
    }
    Rectangle {
        visible: dropArea.containsDrag
        anchors.fill: parent
        anchors.margins: 16
        radius: 10
        color: Theme.accentSoft
        border.color: Theme.accent
        border.width: 2
        Text {
            anchors.centerIn: parent
            text: launcher.selectedId ? "Add mod to " + (launcher.selected.name || "selected instance") : "Create or select an instance first"
            color: Theme.text
            font.pixelSize: 16
            font.weight: Font.DemiBold
        }
    }
}
