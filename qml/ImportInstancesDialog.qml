import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

LiquidDialog {
    id: dialog
    property var shell
    property string provider: "Modrinth"
    property string customFolder: ""
    property string candidateId: ""
    property bool editVersion: false
    readonly property var selected: launcher.importCandidates.find(function(i){return i.id===dialog.candidateId}) || ({})
    objectName: "importInstancesDialog"
    title: "Bring your setups to Moly"
    width: Math.min(900,parent.width-48)
    height: Math.min(680,parent.height-48)
    anchors.centerIn: parent
    onOpened: scan()
    function scan(){ candidateId="";launcher.scanLocalInstances(provider,customFolder) }
    function scanFolder(folder){customFolder=folder;scan()}
    function chooseSource(value){provider=value;customFolder="";scan()}
    Connections { target:launcher;function onImportScanChanged(){if(!launcher.importScanning&&launcher.importCandidates.length&&!dialog.candidateId)dialog.candidateId=launcher.importCandidates[0].id} }
    contentItem: RowLayout {
        spacing:20
        ColumnLayout {
            Layout.preferredWidth:170;Layout.fillHeight:true;spacing:6
            Text { text:"ON THIS COMPUTER";color:Theme.textMuted;font.pixelSize:10;font.letterSpacing:1;Layout.bottomMargin:8 }
            Repeater {
                model:["Modrinth","Prism","CurseForge","Lunar Client","ATLauncher","MultiMC","Minecraft","Folder"]
                delegate:ItemDelegate {
                    required property string modelData
                    Layout.fillWidth:true;implicitHeight:42;padding:10
                    onClicked:dialog.chooseSource(modelData)
                    background:Rectangle { radius:11;color:dialog.provider===modelData?Theme.accentSoft:parent.hovered?Theme.surface2:Theme.transparent }
                    contentItem:RowLayout { spacing:10;ProviderIcon {provider:modelData;Layout.preferredWidth:22;Layout.preferredHeight:22}Text {text:modelData==="Folder"?"Other folder":modelData;color:Theme.text;font.pixelSize:12;Layout.fillWidth:true} }
                }
            }
            Item {Layout.fillHeight:true}
            Text {text:"Local files, copied into Moly.\nYour originals stay in place.";color:Theme.textMuted;font.pixelSize:11;wrapMode:Text.Wrap;Layout.fillWidth:true;lineHeight:1.3}
        }
        Rectangle {Layout.fillHeight:true;Layout.preferredWidth:1;color:Theme.border}
        ColumnLayout {
            Layout.fillWidth:true;Layout.fillHeight:true;spacing:10
            RowLayout {
                Layout.fillWidth:true
                Text {text:dialog.provider==="Folder"?"Choose a game folder":dialog.provider+" instances";font.pixelSize:20;font.weight:Font.DemiBold;color:Theme.text;Layout.fillWidth:true}
                MButton {text:"Scan again";iconName:"refresh";compact:true;enabled:!launcher.importScanning;onClicked:dialog.scan()}
            }
            Text {text:dialog.customFolder||"Detecting the launcher's usual local folders. Choose a folder if yours lives elsewhere.";color:Theme.textSecondary;font.pixelSize:12;wrapMode:Text.Wrap;maximumLineCount:2;elide:Text.ElideMiddle;Layout.fillWidth:true}
            MButton {text:"Choose folder…";iconName:"folder";compact:true;onClicked:shell.chooseFolder("localImport")}
            Rectangle {
                Layout.fillWidth:true;Layout.fillHeight:true;Layout.minimumHeight:90;radius:13;color:Theme.app;border.color:Theme.border
                ListView {
                    anchors.fill:parent;anchors.margins:6;clip:true;spacing:4;model:launcher.importCandidates
                    ScrollBar.vertical:ScrollBar{}
                    delegate:ItemDelegate {
                        required property var modelData
                        width:ListView.view.width;implicitHeight:62;padding:10
                        onClicked:{dialog.candidateId=modelData.id;dialog.editVersion=false}
                        background:Rectangle {radius:10;color:dialog.candidateId===modelData.id?Theme.accentSoft:parent.hovered?Theme.surface2:Theme.transparent}
                        contentItem:RowLayout {
                            spacing:10
                            InstanceAvatar {imageSource:modelData.iconUrl||"";Layout.preferredWidth:34;Layout.preferredHeight:34}
                            ColumnLayout {Layout.fillWidth:true;spacing:4;Text {text:modelData.name;color:Theme.text;font.pixelSize:13;font.weight:Font.DemiBold;elide:Text.ElideRight;Layout.fillWidth:true}Text {text:(modelData.loader||"Minecraft")+" · "+(modelData.version||"Version needed");color:Theme.textSecondary;font.pixelSize:11}}
                            Icon {visible:dialog.candidateId===modelData.id;name:"check";size:16;color:Theme.accent}
                        }
                    }
                }
                ColumnLayout {
                    visible:launcher.importCandidates.length===0;anchors.centerIn:parent;width:parent.width-32;spacing:9
                    ArcSpinner {visible:launcher.importScanning;Layout.alignment:Qt.AlignHCenter;Layout.preferredWidth:22;Layout.preferredHeight:22}
                    Text {text:launcher.importScanning?"Looking for local setups…":launcher.importScanError||"No instances found here. Choose the launcher's data folder or an individual game folder.";color:Theme.textSecondary;font.pixelSize:12;wrapMode:Text.Wrap;horizontalAlignment:Text.AlignHCenter;Layout.fillWidth:true}
                }
            }
            Text {visible:!!dialog.selected.warning;text:dialog.selected.warning||"";color:Theme.textSecondary;font.pixelSize:11;wrapMode:Text.Wrap;Layout.fillWidth:true}
            RowLayout {
                visible:!!dialog.selected.id;Layout.fillWidth:true
                Text {text:dialog.selected.supported?"Check game version before importing":"Game version needed";color:Theme.textSecondary;font.pixelSize:12;Layout.fillWidth:true}
                MButton {text:dialog.editVersion?"Hide":"Edit";kind:"ghost";compact:true;onClicked:dialog.editVersion=!dialog.editVersion}
            }
            RowLayout {
                visible:!!dialog.selected.id&&(dialog.editVersion||!dialog.selected.supported);Layout.fillWidth:true;spacing:8
                MField {id:gameVersion;Layout.fillWidth:true;text:dialog.selected.version||"";placeholderText:"Minecraft version";Accessible.name:"Imported Minecraft version"}
                MComboBox {id:loader;Layout.preferredWidth:110;model:["Vanilla","Fabric","Forge","NeoForge","Quilt"];currentIndex:Math.max(0,model.indexOf(dialog.selected.loader));Accessible.name:"Imported loader"}
                MField {id:loaderVersion;visible:loader.currentText!=="Vanilla";Layout.preferredWidth:120;text:dialog.selected.loaderVersion||"";placeholderText:"Loader version";Accessible.name:"Imported loader version"}
            }
            RowLayout {Layout.fillWidth:true;Text {text:"Include worlds";color:Theme.text;font.pixelSize:12;Layout.fillWidth:true}MSwitch {id:worlds;checked:true;Accessible.name:"Include worlds in import"}}
            Text {text:"Close Minecraft for this setup first. Copies mods, packs, configuration, screenshots, and optional worlds. Moly prepares its own game and Java files.";color:Theme.textMuted;font.pixelSize:11;wrapMode:Text.Wrap;Layout.fillWidth:true}
            RowLayout {
                Layout.alignment:Qt.AlignRight;spacing:8
                MButton {text:"Cancel";onClicked:dialog.close()}
                MButton {text:"Import & prepare";iconName:"download";kind:"primary";enabled:!launcher.busy&&!!dialog.selected.id&&gameVersion.text.trim().length>0&&(loader.currentText==="Vanilla"||loaderVersion.text.trim().length>0);onClicked:{launcher.importLocalInstance(dialog.selected.id,{version:gameVersion.text.trim(),loader:loader.currentText,loaderVersion:loaderVersion.text.trim()},worlds.checked);dialog.close()}}
            }
        }
    }
}
