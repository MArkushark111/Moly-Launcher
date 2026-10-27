import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: page
    property var shell
    property var instance: launcher.selected
    property int revision: 0
    property string exportKind: "mlypack"
    property int tab: 0
    property int slideDir: 1
    property string type: contentChoice.currentValue || "mod"
    property var installedFiles: []
    property string fileInstanceId: ""
    ListModel { id:contentRows; dynamicRoles:true }
    function refreshFiles() {
        if(fileInstanceId !== (instance.id||"")) { contentRows.clear(); fileInstanceId=instance.id||"" }
        installedFiles=instance.id?launcher.contentFiles(type):[]
        refilterFiles()
    }
    function refilterFiles() {
        const query=contentSearch.text.toLowerCase()
        const desired=installedFiles.filter(function(file){return (file.name+" "+(file.title||"")).toLowerCase().indexOf(query)>=0})
        for(let i=0;i<desired.length;i++) {
            const file=desired[i]
            const stableKey=file.path.replace(/\.disabled$/i,"")
            let found=-1
            for(let j=i;j<contentRows.count;j++)if(contentRows.get(j).stableKey===stableKey){found=j;break}
            if(found<0) contentRows.insert(i,{stableKey:stableKey,entry:file})
            else {
                if(found!==i)contentRows.move(found,i,1)
                contentRows.setProperty(i,"entry",file)
            }
        }
        while(contentRows.count>desired.length)contentRows.remove(contentRows.count-1)
    }
    Component.onCompleted: Qt.callLater(refreshFiles)
    onTypeChanged: Qt.callLater(refreshFiles)
    function switchTab(index) {
        if (index === tab)
            return;
        slideDir = index > tab ? 1 : -1;
        tab = index;
        tabAnimation.restart();
    }
    function browseContent() { shell.browse(type) }
    Connections { target:launcher; function onChanged(){page.revision++;Qt.callLater(page.refreshFiles)} }
    DropArea {anchors.fill:parent;onDropped:function(drop){if(!drop.hasUrls||launcher.busy||instance.running)return;for(let url of drop.urls)if(url.toString().toLowerCase().endsWith(".jar")){launcher.importFile(url.toString(),"mod");drop.acceptProposedAction();break}}}
    ColumnLayout {
        anchors.fill:parent;anchors.margins:24;spacing:16
        RowLayout {Layout.fillWidth:true;spacing:16
            Item {
                width:64;height:64
                InstanceAvatar {anchors.fill:parent;imageSource:instance.iconUrl||""}
                HoverHandler {id:avatarHover}
                Rectangle {anchors.fill:parent;radius:16;visible:avatarHover.hovered;color:Qt.rgba(Theme.app.r,Theme.app.g,Theme.app.b,0.7);Text{anchors.centerIn:parent;text:"Edit";color:Theme.text;font.pixelSize:12;font.weight:Font.DemiBold}}
                MouseArea {anchors.fill:parent;cursorShape:Qt.PointingHandCursor;enabled:!launcher.busy&&!instance.running;onClicked:avatarMenu.open()}
                MMenu {id:avatarMenu;MMenuItem {text:"Choose instance image…";onTriggered:shell.chooseFile("icon")}MMenuItem {text:"Reset instance image";enabled:!!instance.iconUrl;onTriggered:launcher.resetInstanceIcon()}}
                Accessible.role:Accessible.Button
                Accessible.name:"Change instance image"
            }
            ColumnLayout{spacing:7;Layout.fillWidth:true
                Text{text:instance.name||"Instance";color:Theme.text;font.family:Theme.fontFamily;font.pixelSize:26;font.weight:Font.Bold;font.letterSpacing:-0.5;elide:Text.ElideRight;Layout.fillWidth:true}
                RowLayout{spacing:8
                    Text{text:(instance.loader||"Vanilla")+" "+(instance.version||"");color:Theme.textSecondary;font.pixelSize:12}
                    Text{text:"·";color:Theme.textMuted}
                    Icon{name:"clock";size:13;color:Theme.textMuted}
                    Text{text:(instance.playtimeMinutes||0)+" minutes";color:Theme.textMuted;font.pixelSize:12}
                    Text{visible:instance.running;text:"· Running";color:Theme.success;font.pixelSize:12}
                }
            }
            MButton{id:playButton;objectName:"instancePlayButton";loading:launcher.busy&&launcher.taskInstanceId===instance.id;text:loading?(launcher.taskOperation==="launch"?"Starting":"Installing…"):instance.running?"Stop":instance.installed?"Play":"Install game";iconName:instance.running?"stop":instance.installed?"play":"download";kind:instance.running?"danger":"primary";implicitHeight:42;implicitWidth:loading?148:instance.installed||instance.running?110:138;enabled:!launcher.busy;onClicked:instance.running?shell.confirm("stop","Stop Minecraft?","Save and close your world in Minecraft first. Stopping the process can lose unsaved changes."):instance.installed?launcher.play():launcher.installSelected()}
            MButton{iconName:"settings";kind:tab===5?"primary":"secondary";implicitHeight:42;implicitWidth:42;onClicked:switchTab(5);Accessible.name:"Instance settings"}
            MButton{iconName:"more";kind:"ghost";onClicked:instanceMenu.open();Accessible.name:"More instance actions"
                MMenu {id:instanceMenu;objectName:"instanceMenu"
                    MMenuItem {text:"Open instance folder";onTriggered:launcher.openFolder()}
                    MMenuItem {text:"Play offline";enabled:instance.installed&&!launcher.busy&&!instance.running;onTriggered:launcher.play(true)}
                    MMenuItem {text:"Repair / Install game";enabled:!launcher.busy&&!instance.running;onTriggered:launcher.installSelected()}
                    MMenuItem {text:"Duplicate instance";enabled:!launcher.busy&&!instance.running;onTriggered:launcher.duplicateInstance()}
                    MenuSeparator{}
                    MMenuItem {destructive:true;iconName:"trash";text:"Delete instance";enabled:!launcher.busy&&!instance.running;onTriggered:shell.confirm("delete","Delete instance?","The instance, worlds, and backups will be moved to the Recycle Bin when possible.")}
                }
            }
        }
        Panel {
            visible:launcher.busy&&launcher.taskInstanceId===instance.id
            Layout.fillWidth:true
            padding:16
            ColumnLayout {anchors.fill:parent;spacing:10
                RowLayout {Layout.fillWidth:true;spacing:12
                    Icon {name:launcher.taskOperation==="launch"?"play":"download";size:20;color:Theme.accent}
                    ColumnLayout {Layout.fillWidth:true;spacing:4
                        Text {text:launcher.taskOperation==="launch"?"Starting Minecraft":"Installing your instance";color:Theme.text;font.pixelSize:14;font.weight:Font.DemiBold}
                        Text {text:launcher.status.replace(/\n/g," ");color:Theme.textSecondary;font.pixelSize:12;elide:Text.ElideRight;Layout.fillWidth:true}
                    }
                    ColumnLayout {spacing:2;Text {text:launcher.progress>=0?Math.round(launcher.progress*100)+"%":"Preparing…";color:Theme.text;font.pixelSize:18;font.weight:Font.DemiBold;Layout.alignment:Qt.AlignRight}Text {visible:launcher.progress>=0;text:"Current step";color:Theme.textMuted;font.pixelSize:10;Layout.alignment:Qt.AlignRight}}
                    MButton {text:"Cancel";compact:true;kind:"ghost";onClicked:launcher.cancel()}
                }
                MProgressBar {Layout.fillWidth:true;value:Math.max(0,launcher.progress);indeterminate:launcher.progress<0}
            }
        }
        TabStrip{Layout.fillWidth:true;Layout.topMargin:6;tabs:[{label:"Content",icon:"box"},{label:"Files",icon:"folder"},{label:"Worlds & servers",icon:"world"},{label:"Logs",icon:"logs"},{label:"Share",icon:"share"}];currentIndex:page.tab<5?page.tab:-1;onActivated:function(index){page.switchTab(index)}}
        StackLayout {
            id:stack;Layout.fillWidth:true;Layout.fillHeight:true;currentIndex:page.tab
            transform:Translate{id:tabShift}
            ParallelAnimation{
                id:tabAnimation
                NumberAnimation{target:stack;property:"opacity";from:Theme.motion?0:1;to:1;duration:Theme.motion?210:0;easing.type:Easing.OutQuint}
                NumberAnimation{target:tabShift;property:"x";from:Theme.motion?18*page.slideDir:0;to:0;duration:Theme.motion?290:0;easing.type:Easing.OutQuint}
            }
            ColumnLayout {
                spacing:12
                RowLayout{Layout.fillWidth:true;spacing:8
                    MField{id:contentSearch;onTextChanged:Qt.callLater(page.refilterFiles);placeholderText:"Search "+page.installedFiles.length+" installed files…";Layout.fillWidth:true}
                    MButton{text:"Add files";iconName:"plus";enabled:!launcher.busy&&!instance.running;onClicked:shell.chooseFile("content",page.type)}
                    MButton{text:"Browse "+(page.type==="mod"?"mods":page.type==="shader"?"shaders":"packs");iconName:"discover";kind:"primary";enabled:!launcher.busy&&!instance.running;onClicked:page.browseContent()}
                }
                RowLayout{Layout.fillWidth:true;spacing:8
                    MComboBox {id:contentChoice;Layout.preferredWidth:170;model:[{text:"Mods",value:"mod"},{text:"Resource packs",value:"resourcepack"},{text:"Shaders",value:"shader"}];textRole:"text";valueRole:"value"}
                    Text{text:page.type==="shader"?"A compatible shader mod is required.":page.type==="resourcepack"?"Enable packs in Minecraft's Resource Packs menu.":"Compatible with "+(instance.loader||"")+" "+(instance.version||"");color:Theme.textSecondary;font.pixelSize:12;Layout.fillWidth:true;elide:Text.ElideRight}
                    MButton{text:"Updates";iconName:"refresh";kind:"ghost";compact:true;enabled:!launcher.busy&&!instance.running;onClicked:launcher.checkUpdates()}
                }
                Rectangle{Layout.fillWidth:true;Layout.fillHeight:true;radius:9;color:Theme.surface;border.color:Theme.border;clip:true
                    ColumnLayout{anchors.fill:parent;spacing:0
                        Rectangle{Layout.fillWidth:true;height:36;color:Theme.surface2
                            RowLayout{anchors.fill:parent;anchors.leftMargin:58;anchors.rightMargin:18
                                Text{text:"Project";color:Theme.textSecondary;font.pixelSize:12;Layout.fillWidth:true}
                                Text{text:"Version";color:Theme.textSecondary;font.pixelSize:12;Layout.preferredWidth:150}
                                Text{text:"Actions";color:Theme.textSecondary;font.pixelSize:12;Layout.preferredWidth:95;horizontalAlignment:Text.AlignRight}
                            }
                        }
                        ListView{id:fileList;Layout.fillWidth:true;Layout.fillHeight:true;clip:true;model:contentRows;reuseItems:true;ScrollBar.vertical:ScrollBar{}
                            delegate:Rectangle{
                                id:fileRow
                                required property var entry
                                property var modelData:entry
                                width:ListView.view.width;height:70
                                color:rowHover.hovered?Theme.surface2:Theme.surface
                                Behavior on color{ColorAnimation{duration:Theme.motion?140:0}}
                                HoverHandler{id:rowHover}
                                Rectangle{anchors.bottom:parent.bottom;width:parent.width;height:1;color:Theme.border}
                                RowLayout{anchors.fill:parent;anchors.leftMargin:14;anchors.rightMargin:14;spacing:12
                                    Rectangle{width:34;height:34;radius:9;color:Theme.surface2;border.color:Theme.border;clip:true
                                        Image{id:fileLogo;anchors.fill:parent;anchors.margins:3;source:fileRow.modelData.icon||"";sourceSize.width:64;sourceSize.height:64;fillMode:Image.PreserveAspectFit;asynchronous:true;visible:status===Image.Ready;opacity:fileRow.modelData.enabled?1:0.4;Behavior on opacity{NumberAnimation{duration:Theme.motion?180:0}}}
                                        Icon{visible:fileLogo.status!==Image.Ready;anchors.centerIn:parent;name:"box";size:17;color:Theme.textSecondary}
                                    }
                                    ColumnLayout{Layout.fillWidth:true;spacing:5
                                        Text{text:(fileRow.modelData.title||fileRow.modelData.name).replace(/\.jar(\.disabled)?$/i,"");color:fileRow.modelData.enabled?Theme.text:Theme.textMuted;font.family:Theme.fontFamily;font.pixelSize:13;font.weight:Font.DemiBold;elide:Text.ElideRight;Layout.fillWidth:true;Behavior on color{ColorAnimation{duration:Theme.motion?180:0}}}
                                        Text{text:fileRow.modelData.name;color:Theme.textMuted;font.family:"Consolas";font.pixelSize:11;elide:Text.ElideMiddle;Layout.fillWidth:true}
                                    }
                                    Text{text:fileRow.modelData.versionName||"Local";color:Theme.textSecondary;font.pixelSize:12;elide:Text.ElideRight;Layout.preferredWidth:150}
                                    MSwitch{objectName:"installedModSwitch";checked:fileRow.modelData.enabled;enabled:!launcher.busy&&!instance.running;onClicked:launcher.toggleFile(fileRow.modelData.path);Accessible.name:"Enable "+fileRow.modelData.name}
                                    MButton{iconName:"trash";kind:"ghost";compact:true;opacity:rowHover.hovered||activeFocus?1:0.35;enabled:!launcher.busy&&!instance.running;labelColor:Theme.danger;onClicked:launcher.removeFile(fileRow.modelData.path);Accessible.name:"Remove "+fileRow.modelData.name;Behavior on opacity{NumberAnimation{duration:Theme.motion?140:0}}}
                                }
                            }
                            ColumnLayout{visible:fileList.count===0;anchors.centerIn:parent;spacing:10
                                Icon{name:"box";size:28;color:Theme.textMuted;Layout.alignment:Qt.AlignHCenter}
                                Text{text:contentSearch.text?"No matching files":"No "+(page.type==="mod"?"mods":page.type==="shader"?"shaders":"resource packs")+" installed";color:Theme.textSecondary;font.pixelSize:14;Layout.alignment:Qt.AlignHCenter}
                                MButton{text:"Browse content";kind:"primary";Layout.alignment:Qt.AlignHCenter;onClicked:page.browseContent();enabled:!launcher.busy&&!instance.running}
                                Text{text:"You can also drop a .jar here.";visible:page.type==="mod";color:Theme.textMuted;font.pixelSize:12;Layout.alignment:Qt.AlignHCenter}
                            }
                        }
                    }
                }
            }
            ColumnLayout{spacing:12
                Text{text:"Instance files";color:Theme.text;font.pixelSize:16;font.weight:Font.DemiBold}
                Text{text:"Open a folder in Windows to manage the files for this instance.";color:Theme.textSecondary;font.pixelSize:13}
                Repeater{model:[{name:"Instance folder",path:"",icon:"folder"},{name:"Mods",path:"mods",icon:"box"},{name:"Resource packs",path:"resourcepacks",icon:"file"},{name:"Shader packs",path:"shaderpacks",icon:"file"},{name:"Configuration",path:"config",icon:"settings"},{name:"Screenshots",path:"screenshots",icon:"file"}]
                    delegate:Rectangle{required property var modelData;Layout.fillWidth:true;height:50;radius:7;color:Theme.surface;border.color:Theme.border
                        RowLayout{anchors.fill:parent;anchors.leftMargin:14;anchors.rightMargin:10;spacing:12;Icon{name:modelData.icon;size:18}Text{text:modelData.name;color:Theme.text;font.pixelSize:13;Layout.fillWidth:true}MButton{text:"Open folder";compact:true;kind:"ghost";iconName:"external";onClicked:launcher.openFolder(modelData.path)}}
                    }
                }
                Item{Layout.fillHeight:true}
            }
            ScrollView {
                id:placesScroll;clip:true;contentWidth:availableWidth
                ColumnLayout {
                    width:placesScroll.availableWidth;spacing:14
                    RowLayout {Layout.fillWidth:true;Text{text:"Your places";color:Theme.text;font.pixelSize:17;font.weight:Font.DemiBold;Layout.fillWidth:true}MButton{text:"Open saves";iconName:"folder";compact:true;onClicked:launcher.openFolder("saves")}MButton{text:"Back up worlds";iconName:"download";compact:true;enabled:!launcher.busy&&!instance.running;onClicked:launcher.backupWorlds()}}
                    PlayTargets {
                        Layout.fillWidth:true;items:playTargets.items;pingColor:Theme.serverAccent;loading:playTargets.loading;checkingStatus:playTargets.checkingStatus;maximumRows:0;showInstanceName:false;actionsEnabled:!launcher.busy&&!instance.running
                        title:"Worlds & saved servers";onActivated:function(entry){shell.launchTarget(entry)};onChangeWorldIcon:function(entry){shell.chooseWorldIcon(entry)};onRefreshRequested:playTargets.refresh()
                    }
                    Text{text:"Servers are read from this instance's Multiplayer list. Direct world and server launch uses Minecraft's quick-play support.";color:Theme.textMuted;font.pixelSize:11;wrapMode:Text.Wrap;Layout.fillWidth:true}
                    Item{implicitHeight:12}
                }
            }
            LogView{shell:page.shell}
            ColumnLayout {
                spacing:10
            ScrollView {
                id:shareScroll;Layout.fillWidth:true;Layout.fillHeight:true;clip:true;contentWidth:availableWidth
                ColumnLayout {
                    width:shareScroll.availableWidth;spacing:14
                    Text{text:"Take your setup with you";color:Theme.text;font.pixelSize:21;font.weight:Font.DemiBold}
                    Text{text:"Pick a format. Account details and launcher secrets are excluded.";color:Theme.textSecondary;font.pixelSize:12;wrapMode:Text.Wrap;Layout.fillWidth:true}
                    Repeater {
                        model:[{format:"mlypack",name:"Moly pack",hint:"Mods, packs, settings, and artwork in one portable file."},{format:"mrpack",name:"Modrinth pack",hint:"Exact download references with local overrides; checks distribution rules."},{format:"zip",name:"Portable ZIP",hint:"The Moly setup in a ZIP, ready to import on another computer."}]
                        delegate:ItemDelegate {
                            required property var modelData
                            Layout.fillWidth:true;implicitHeight:76;padding:14
                            onClicked:page.exportKind=modelData.format
                            background:Rectangle{radius:14;color:page.exportKind===modelData.format?Theme.accentSoft:Theme.surface;border.color:parent.visualFocus?Theme.textSecondary:Theme.border}
                            contentItem:RowLayout {spacing:14;Icon{name:"share";size:23;color:page.exportKind===modelData.format?Theme.accent:Theme.textMuted}ColumnLayout{Layout.fillWidth:true;spacing:5;Text{text:modelData.name+" · ."+modelData.format;color:Theme.text;font.pixelSize:14;font.weight:Font.DemiBold}Text{text:modelData.hint;color:Theme.textSecondary;font.pixelSize:12;wrapMode:Text.Wrap;Layout.fillWidth:true}}Icon{name:page.exportKind===modelData.format?"check":"chevron";size:18;color:Theme.textSecondary}}
                        }
                    }
                    Text {text:"Minecraft and Java files are prepared after import. ZIP uses Moly's portable layout.";color:Theme.textMuted;font.pixelSize:11;wrapMode:Text.Wrap;Layout.fillWidth:true}
                    Item{implicitHeight:12}
                }
            }
                RowLayout {
                    Layout.fillWidth:true;spacing:8
                    MCheckBox {id:exportWorlds;enabled:page.exportKind!=="mrpack";text:"Include worlds"}
                    MButton{text:"Content list";kind:"ghost";compact:true;onClicked:shell.chooseFile("share")}
                    Item{Layout.fillWidth:true}
                    MButton{text:"Export ."+page.exportKind;iconName:"share";kind:"primary";enabled:!launcher.busy&&!instance.running;onClicked:shell.exportSetup(page.exportKind,page.exportKind!=="mrpack"&&exportWorlds.checked)}
                }
            }
            InstanceSettings { instance: page.instance; shell: page.shell; revision: page.revision }
        }
    }
}
