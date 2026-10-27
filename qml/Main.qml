import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    id: window
    width: 1280
    height: 820
    minimumWidth: 1024
    minimumHeight: 640
    visible: true
    title: "Moly Launcher"
    color: Theme.app
    font.family: Theme.fontFamily
    font.pixelSize: 13
    palette.window: Theme.surface
    palette.windowText: Theme.text
    palette.base: Theme.surface2
    palette.text: Theme.text
    palette.button: Theme.surface2
    palette.buttonText: Theme.text
    palette.highlight: Theme.accent
    palette.accent: Theme.accent
    palette.highlightedText: Theme.text
    palette.mid: Theme.border
    palette.light: Theme.border
    palette.dark: Theme.app
    property string page: "library"
    property string discoverType: "modpack"
    property string fileAction: "pack"
    property string contentType: "mod"
    property string discoverQuery: ""
    property string discoverProvider: "Modrinth"
    property string exportFormat: "mlypack"
    property bool exportWorlds: false
    property var worldIconEntry: ({})
    property int resolveIndex: -1
    property string folderAction: "prism"
    property string confirmAction: ""
    property string confirmValue: ""
    property bool forceQuit: false
    property bool pendingGameLogs: false
    property int slideDir: 1
    readonly property var pageOrder: ["library", "discover", "instance", "skins", "accounts", "settings"]
    function navigate(target) {
        if(target === "downloads")target = "library"
        if (target === page)
            return
        slideDir = pageOrder.indexOf(target) >= pageOrder.indexOf(page) ? 1 : -1
        page = target
    }
    function pageSource() {
        switch (page) {
        case "library": return "LibraryPage.qml"
        case "discover": return "DiscoverPage.qml"
        case "instance": return "InstancePage.qml"
        case "accounts": return "AccountsPage.qml"
        case "settings": return "SettingsPage.qml"
        case "skins": return "SkinsPage.qml"
        }
        return "LibraryPage.qml"
    }
    function focusGameLogs() {
        if (pendingGameLogs && page === "instance" && pageLoader.item && pageLoader.item.switchTab) {
            pageLoader.item.switchTab(3)
            pendingGameLogs = false
        }
    }
    function browse(type) { discoverQuery=""; discoverType = type || "mod"; navigate("discover") }
    function browsePacks(query,provider) { discoverQuery=query;discoverProvider=provider;discoverType="modpack";navigate("discover") }
    function exportSetup(format,worlds) { exportFormat=format;exportWorlds=worlds;chooseFile("export") }
    function launchTarget(entry) { launcher.playTarget(entry.instanceId,entry.type,entry.value) }
    function chooseWorldIcon(entry) {
        if(launcher.isInstanceRunning(entry.instanceId)){toast.show("Close this instance before changing its world icon.",true);return}
        worldIconEntry=entry;chooseFile("worldIcon")
    }
    onPageChanged: playTargets.selectedInstanceId = page==="instance" ? launcher.selectedId : ""

    function installProject(project) { installModal.openProject(project) }
    function chooseFile(action, type) {
        fileAction = action; contentType = type || "mod"
        fileDialog.fileMode = ["log","share","export"].indexOf(action)>=0 ? FileDialog.SaveFile : FileDialog.OpenFile
        fileDialog.defaultSuffix=action==="export"?exportFormat:""
        fileDialog.nameFilters = action==="skin" ? ["Minecraft skin (*.png)"] : action==="worldIcon" ? ["World image (*.png *.jpg *.jpeg *.webp)"] : ["icon","background"].indexOf(action)>=0 ? ["Artwork (*.gif *.png *.jpg *.jpeg *.webp *.bmp)"] : action==="export" ? [exportFormat.toUpperCase()+" pack (*."+exportFormat+")"] : action==="share" ? ["Content manifest (*.json)"] : action==="log" ? ["Text log (*.log *.txt)"] : action==="pack" ? ["Minecraft packs (*.mlypack *.mrpack *.zip)"] : ["Minecraft files (*.jar *.zip)","All files (*)"]
        fileDialog.open()
    }
    function chooseFolder(action) { folderAction = action; folderDialog.open() }
    function confirm(action, title, message, value) { confirmAction=action; confirmValue=value || ""; confirmDialog.title=title; confirmText.text=message; confirmDialog.open() }
    function create() { createDialog.open() }
    function openTaskCenter() { downloadPanel.open() }
    Shortcut { sequence: "F11"; onActivated: window.visibility === Window.FullScreen ? window.showNormal() : window.showFullScreen() }
    Shortcut { sequence: "Ctrl+N"; onActivated: create() }
    Shortcut { sequence: "Alt+Left"; enabled:page==="instance"; onActivated:navigate("library") }
    onClosing: function(close) {
        if (!forceQuit && (launcher.busy || launcher.instances.some(function(i) { return i.running }))) {
            close.accepted = false
            confirm("quit", "Close Moly?", "A task or game is still running. Closing Moly will cancel its tasks and stop games it launched. Save and close Minecraft first to protect unsaved progress.")
        }
    }
    Component.onCompleted: if (launcher.settings.launcherFullscreen) showFullScreen()
    ColumnLayout {
        anchors.fill:parent
        spacing:0
        RowLayout {
            Layout.fillWidth:true;Layout.fillHeight:true;spacing:0
            Rectangle {
                visible:launcher.settings.showInstanceRail!==false;Layout.preferredWidth:200;Layout.fillHeight:true;color:Theme.sidebar
                Rectangle {anchors.right:parent.right;width:1;height:parent.height;color:Theme.border}
                ColumnLayout {
                    anchors.fill:parent;anchors.margins:14;spacing:12
                    RowLayout {
                        Layout.fillWidth:true;Layout.preferredHeight:56;spacing:11
                        Image {Layout.preferredWidth:38;Layout.preferredHeight:38;source:"../assets/moly.svg";sourceSize.width:76;sourceSize.height:76;smooth:true}
                        Text {text:"Moly";color:Theme.text;font.family:Theme.fontFamily;font.pixelSize:20;font.weight:Font.Bold;Layout.fillWidth:true}
                        Text {text:"JAVA";color:Theme.textMuted;font.pixelSize:9;font.letterSpacing:1}
                    }
                    RowLayout {Layout.fillWidth:true
                        Text {text:"INSTANCES";color:Theme.textMuted;font.pixelSize:10;font.weight:Font.Medium;font.letterSpacing:0.8;Layout.fillWidth:true}
                        Text {text:launcher.instances.length.toString();color:Theme.textMuted;font.pixelSize:11}
                    }
                    ListView {
                        id:instanceRail
                        Layout.fillWidth:true;Layout.fillHeight:true
                        clip:true;spacing:9;model:launcher.instances
                        delegate:ItemDelegate {
                            id:instanceTile
                            required property var modelData
                            width:ListView.view.width;height:100;padding:0
                            Accessible.name:modelData.name
                            onClicked:{launcher.selectInstance(modelData.id);window.page="instance"}
                            background:Rectangle {
                                radius:17
                                color:window.page==="instance"&&launcher.selectedId===instanceTile.modelData.id?Theme.accentSoft:instanceTile.hovered?Theme.surface2:Theme.surface
                                border.color:instanceTile.activeFocus?Theme.accent:Theme.border
                                border.width:instanceTile.activeFocus?2:1
                                ArtworkView {anchors.fill:parent;radius:17;source:instanceTile.modelData.backgroundUrl||"";shade:launcher.settings.backgroundDim===undefined?0.5:launcher.settings.backgroundDim;animated:Theme.motion&&launcher.settings.animateArtwork!==false}
                                Behavior on color {ColorAnimation{duration:Theme.motion?160:0}}
                            }
                            contentItem:Item {
                                InstanceAvatar {width:34;height:34;anchors.horizontalCenter:parent.horizontalCenter;anchors.top:parent.top;anchors.topMargin:12;imageSource:instanceTile.modelData.iconUrl||""}
                                Text {anchors.left:parent.left;anchors.right:parent.right;anchors.leftMargin:10;anchors.rightMargin:10;y:53;height:20;text:instanceTile.modelData.name;color:Theme.text;font.family:Theme.fontFamily;font.pixelSize:12;font.weight:Font.DemiBold;horizontalAlignment:Text.AlignHCenter;verticalAlignment:Text.AlignVCenter;elide:Text.ElideRight}
                                Text {anchors.left:parent.left;anchors.right:parent.right;anchors.leftMargin:10;anchors.rightMargin:10;y:76;height:14;text:instanceTile.modelData.running?"Running":instanceTile.modelData.loader+" · "+instanceTile.modelData.version;color:instanceTile.modelData.running?Theme.success:Theme.textMuted;font.pixelSize:10;horizontalAlignment:Text.AlignHCenter;verticalAlignment:Text.AlignVCenter;elide:Text.ElideRight}
                            }
                        }
                        ScrollBar.vertical:ScrollBar{}
                        Text {visible:instanceRail.count===0;anchors.centerIn:parent;width:parent.width-20;text:"Create your first\nMinecraft instance";color:Theme.textSecondary;font.pixelSize:12;lineHeight:1.4;horizontalAlignment:Text.AlignHCenter;wrapMode:Text.Wrap}
                    }
                    MButton {text:"New instance";iconName:"plus";kind:"secondary";Layout.fillWidth:true;enabled:!launcher.busy;onClicked:window.create()}
                }
            }
            ColumnLayout {
                Layout.fillWidth:true;Layout.fillHeight:true;spacing:0
                Item {
                    Layout.fillWidth:true;Layout.fillHeight:true;clip:true
                    Loader {
                        id:pageLoader;objectName:"pageLoader";anchors.fill:parent
                        source:window.page==="instance"?"InstancePage.qml":window.page==="discover"?"DiscoverPage.qml":window.page==="accounts"?"AccountsPage.qml":window.page==="settings"?"SettingsPage.qml":window.page==="skins"?"SkinsPage.qml":"LibraryPage.qml"
                        onLoaded:{item.shell=window;pageFade.restart();Qt.callLater(window.focusGameLogs)}
                        SequentialAnimation {id:pageFade;PropertyAction{target:pageLoader;property:"opacity";value:Theme.motion?0:1}PropertyAction{target:pageLoader;property:"anchors.topMargin";value:Theme.motion?8:0}ParallelAnimation{NumberAnimation{target:pageLoader;property:"opacity";to:1;duration:Theme.motion?210:0;easing.type:Easing.OutQuint}NumberAnimation{target:pageLoader;property:"anchors.topMargin";to:0;duration:Theme.motion?270:0;easing.type:Easing.OutQuint}}}
                    }
                }
            }
        }
        Item {
            Layout.fillWidth:true;Layout.preferredHeight:80
            Rectangle {anchors.top:parent.top;height:1;width:parent.width;color:Theme.border}
            TaskActivity { id: taskActivity; anchors.left:parent.left;anchors.leftMargin:14;anchors.verticalCenter:parent.verticalCenter; maximumWidth:Math.max(80,mainDock.x-24);onClicked:downloadPanel.visible?downloadPanel.close():downloadPanel.open() }
            TabStrip {
                id:mainDock
                anchors.centerIn:parent;height:52;width:implicitWidth
                tabs:[{label:"Library",icon:"library"},{label:"Discover",icon:"discover"},{label:"Wardrobe",icon:"skin"},{label:"Accounts",icon:"accounts"},{label:"Settings",icon:"settings"}]
                currentIndex:window.page==="discover"?1:window.page==="skins"?2:window.page==="accounts"?3:window.page==="settings"?4:0
                onActivated:function(index){window.page=["library","discover","skins","accounts","settings"][index]}
            }

        }
    }
    Rectangle {
        id:toast;z:30;anchors.right:parent.right;anchors.bottom:parent.bottom
        anchors.rightMargin:20;anchors.bottomMargin:96+toast.toastRise
        width:Math.min(520,window.width-260);height:Math.max(56,toastText.implicitHeight+28);radius:10
        color:Theme.surface2;border.color:toast.error?Qt.rgba(Theme.danger.r,Theme.danger.g,Theme.danger.b,0.45):Theme.border
        property bool error:false
        property bool shown:false
        property real toastRise:14
        opacity:shown?1:0
        visible:opacity>0.001
        Behavior on opacity { NumberAnimation{duration:Theme.motion?170:0} }
        Behavior on toastRise { NumberAnimation{duration:Theme.motion?230:0;easing.type:Easing.OutQuint} }
        function show(message,isError){toastText.text=message;toast.error=isError;toast.toastRise=0;toast.shown=true;toastTimer.restart()}
        function dismiss(){toast.shown=false;toast.toastRise=14}
        RowLayout{anchors.fill:parent;anchors.margins:12;spacing:10
            Icon{name:toast.error?"close":"check";size:16;color:toast.error?Theme.danger:Theme.textSecondary}
            Text{id:toastText;Layout.fillWidth:true;color:Theme.text;font.family:Theme.fontFamily;font.pixelSize:12;wrapMode:Text.Wrap;verticalAlignment:Text.AlignVCenter;Layout.alignment:Qt.AlignVCenter}
            MButton{iconName:"close";kind:"ghost";compact:true;onClicked:toast.dismiss();Accessible.name:"Dismiss notification"}
        }
        Timer{id:toastTimer;interval:6000;onTriggered:toast.dismiss()}
    }
    Connections {
        target:launcher
        function onNavigate(target){window.navigate(target)}
        function onChanged(){if(window.page==="instance")playTargets.selectedInstanceId=launcher.selectedId}
        function onNotice(message,error){toast.show(message,error)}
        function onPlanChanged(){if(launcher.pendingPlan.title&&!launcher.installAutomatic&&!installModal.visible)installModal.openReview()}
        function onGameStarted(){window.pendingGameLogs=true;window.navigate("instance");Qt.callLater(window.focusGameLogs);if(launcher.selected.settings&&launcher.selected.settings.keepVisible!==undefined?!launcher.selected.settings.keepVisible:!launcher.settings.keepVisible)window.showMinimized()}
        function onGameEnded(){if(window.visibility===Window.Minimized)window.showNormal()}
    }
    DownloadManager {id:downloadPanel;shell:window}
    InstallDialog{id:installModal;shell:window}
    FileDialog {
        id:fileDialog
        onAccepted:{
            const path=selectedFile.toString()
            if(fileAction==="pack")launcher.importPack(path)
            else if(fileAction==="log")launcher.saveLog(path)
            else if(fileAction==="icon")launcher.setInstanceIcon(path)
            else if(fileAction==="background")launcher.setInstanceBackground(path)
            else if(fileAction==="export")launcher.exportInstance(path,exportFormat,exportWorlds)
            else if(fileAction==="skin")skinStore.importSkin(path,pageLoader.item&&pageLoader.item.importModel||"classic")
            else if(fileAction==="worldIcon"){
                if(launcher.isInstanceRunning(worldIconEntry.instanceId))toast.show("Close this instance before changing its world icon.",true)
                else playTargets.setWorldIcon(worldIconEntry.instanceId,worldIconEntry.value,path)
            }
            else if(fileAction==="share")launcher.exportContentList(path)
            else if(fileAction==="resolve")launcher.resolveFile(resolveIndex,path)
            else launcher.importFile(path,contentType)
        }
    }
    FolderDialog{id:folderDialog;onAccepted:folderAction==="prism"?launcher.importPrism(selectedFolder.toString()):launcher.relocate(selectedFolder.toString())}
    CreateInstanceDialog {id:createDialog;shell:window}
    LiquidDialog {
        id:confirmDialog;anchors.centerIn:parent;width:480;modal:true;padding:24;closePolicy:Popup.CloseOnEscape
        contentItem:ColumnLayout{spacing:20
            Text{id:confirmText;Layout.fillWidth:true;wrapMode:Text.Wrap;color:Theme.textSecondary;font.pixelSize:13}
            RowLayout{Layout.alignment:Qt.AlignRight;spacing:8
                MButton{text:"Cancel";onClicked:confirmDialog.close()}
                MButton{text:"Confirm";kind:"danger";onClicked:{confirmDialog.close();if(confirmAction==="delete")launcher.deleteInstance();else if(confirmAction==="restore")launcher.restoreBackup(confirmValue);else if(confirmAction==="account")launcher.removeAccount(confirmValue);else if(confirmAction==="stop")launcher.stopGame();else if(confirmAction==="quit"){forceQuit=true;Qt.quit()}}}
            }
        }
    }
}
