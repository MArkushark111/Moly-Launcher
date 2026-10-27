import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

LiquidDialog {
    id: dialog
    objectName:"createDialog"
    property var shell
    property int choice: 0
    onChoiceChanged: optionSlide.restart()
    title: "Add a Minecraft setup"
    width: Math.min(760,parent.width-48)
    height: Math.min(580,parent.height-48)
    anchors.centerIn: parent
    contentItem: RowLayout {
        spacing: 22
        ColumnLayout {
            Layout.preferredWidth: 166
            Layout.fillHeight: true
            spacing: 10
            Text { text: "START HERE"; color: Theme.textMuted; font.pixelSize: 10; font.letterSpacing: 1 }
            Repeater {
                model: [{title:"Build your own",icon:"settings",hint:"Version + loader"},{title:"Find a modpack",icon:"discover",hint:"Ready-made setups"},{title:"Bring a setup",icon:"folder",hint:"From your computer"}]
                delegate: ItemDelegate {
                    required property var modelData
                    required property int index
                    Layout.fillWidth: true
                    implicitHeight: 82
                    padding: 12
                    onClicked: dialog.choice=index
                    background: Rectangle { radius: 14; color: dialog.choice===index?Theme.accentSoft:parent.hovered?Theme.surface2:Theme.transparent; border.color: parent.activeFocus?Theme.textSecondary:Theme.transparent }
                    contentItem: ColumnLayout {
                        spacing: 5
                        RowLayout { spacing: 8; Icon { name:modelData.icon;size:17;color:dialog.choice===index?Theme.accent:Theme.textSecondary } Text { text:modelData.title;color:Theme.text;font.pixelSize:12;font.weight:Font.DemiBold } }
                        Text { text:modelData.hint;color:Theme.textMuted;font.pixelSize:11 }
                    }
                }
            }
            Item { Layout.fillHeight: true }
            Text { text: "Separate worlds.\nA setup that is yours."; color:Theme.textMuted; font.pixelSize:11; lineHeight:1.4 }
        }
        Rectangle { Layout.fillHeight:true;Layout.preferredWidth:1;color:Theme.border }
        StackLayout {
            id: choiceStack
            clip: true
            transform: Translate { id: optionOffset }
            ParallelAnimation { id: optionSlide; NumberAnimation { target: choiceStack; property: "opacity"; from: Theme.motion ? 0.25 : 1; to: 1; duration: Theme.motion ? 200 : 0 } NumberAnimation { target: optionOffset; property: "x"; from: Theme.motion ? 22 : 0; to: 0; duration: Theme.motion ? 250 : 0; easing.type: Easing.OutCubic } }
            currentIndex: dialog.choice
            Layout.fillWidth:true
            Layout.fillHeight:true
            ColumnLayout {
                spacing: 12
                Text { text:"Make it yours";color:Theme.text;font.pixelSize:21;font.weight:Font.DemiBold }
                Text { text:"Choose your game version. Moly will install the files and Java runtime for you.";color:Theme.textSecondary;font.pixelSize:12;wrapMode:Text.Wrap;Layout.fillWidth:true }
                MField { id:setupName;Layout.fillWidth:true;placeholderText:"Give this instance a name";maximumLength:80;Accessible.name:"Instance name" }
                RowLayout {
                    Layout.fillWidth:true;spacing:10
                    ColumnLayout { Layout.fillWidth:true;Text{text:"Minecraft";color:Theme.textSecondary;font.pixelSize:12} MComboBox {id:gameVersion;enabled:!launcher.busy;Layout.fillWidth:true;model:launcher.versions.map(function(v){return v.id});onActivated:if(loaderChoice.currentText!=="Vanilla")launcher.refreshLoaders(currentText,loaderChoice.currentText)} }
                    ColumnLayout { Layout.fillWidth:true;Text{text:"Loader";color:Theme.textSecondary;font.pixelSize:12} MComboBox {id:loaderChoice;enabled:!launcher.busy;Layout.fillWidth:true;model:["Vanilla","Fabric","Forge","NeoForge","Quilt"];onActivated:launcher.refreshLoaders(gameVersion.currentText,currentText)} }
                }
                MComboBox { id:loaderVersion;enabled:!launcher.busy;visible:loaderChoice.currentText!=="Vanilla";model:launcher.loaderVersions;Layout.fillWidth:true;Accessible.name:"Loader version" }
                MCheckBox { enabled:!launcher.busy;text:"Include snapshots";onToggled:launcher.refreshVersions(checked) }
                Text { text:launcher.busy?launcher.status:"You can add mods, artwork, and packs afterwards.";color:Theme.textMuted;font.pixelSize:11;wrapMode:Text.Wrap;Layout.fillWidth:true }
                Item { Layout.fillHeight:true }
                MButton { text:"Create & install";iconName:"plus";kind:"primary";Layout.alignment:Qt.AlignRight;enabled:!launcher.busy&&setupName.text.trim().length>0&&gameVersion.currentText.length>0&&(loaderChoice.currentText==="Vanilla"||loaderVersion.currentText.length>0);onClicked:{launcher.createInstance(setupName.text,gameVersion.currentText,loaderChoice.currentText,loaderChoice.currentText==="Vanilla"?"":loaderVersion.currentText);dialog.close();setupName.clear()} }
            }
            ColumnLayout {
                spacing:10
                Text { text:"Find your next setup";color:Theme.text;font.pixelSize:21;font.weight:Font.DemiBold }
                Text { text:"Explore a pack, review its contents, and install it as a separate instance.";color:Theme.textSecondary;font.pixelSize:12;wrapMode:Text.Wrap;Layout.fillWidth:true }
                MField { id:packSearch;Layout.fillWidth:true;placeholderText:"Search modpacks…";onAccepted:explore.clicked() }
                MComboBox { id:packProvider;providerIcons:true;Layout.fillWidth:true;model:["Modrinth","CurseForge"];Accessible.name:"Modpack provider" }
                Text { text:"A starting point, picked by you";color:Theme.textMuted;font.pixelSize:11;Layout.topMargin:4 }
                GridLayout {
                    columns:2;columnSpacing:8;rowSpacing:8;Layout.fillWidth:true
                    Repeater {
                        model:[{name:"Smooth & light",hint:"Keep things fast",query:"optimization",icon:"settings"},{name:"A new adventure",hint:"Quests and exploration",query:"adventure",icon:"discover"},{name:"Build something",hint:"Create and engineer",query:"create",icon:"box"},{name:"More of everything",hint:"A bigger Minecraft",query:"kitchen sink",icon:"world"}]
                        delegate:ItemDelegate {required property var modelData;Layout.fillWidth:true;implicitHeight:57;padding:10;onClicked:{packSearch.text=modelData.query;explore.clicked()}background:Rectangle {radius:12;color:parent.hovered?Theme.accentSoft:Theme.surface2;border.color:Theme.border}contentItem:RowLayout {spacing:9;Icon {name:modelData.icon;size:18;color:Theme.accent}ColumnLayout {Layout.fillWidth:true;spacing:4;Text{text:modelData.name;color:Theme.text;font.pixelSize:12;font.weight:Font.DemiBold}Text{text:modelData.hint;color:Theme.textMuted;font.pixelSize:10}}}}
                    }
                }
                Item { Layout.fillHeight:true }
                MButton { id:explore;text:"Explore packs";iconName:"discover";kind:"primary";Layout.alignment:Qt.AlignRight;onClicked:{dialog.close();shell.browsePacks(packSearch.text,packProvider.currentText)} }
            }
            ColumnLayout {
                spacing:14
                Text { text:"Pick up where you left off";color:Theme.text;font.pixelSize:21;font.weight:Font.DemiBold;wrapMode:Text.Wrap;Layout.fillWidth:true }
                Text { text:"Bring over a local setup from another launcher, or open a pack file. Moly keeps each imported setup separate.";color:Theme.textSecondary;font.pixelSize:12;wrapMode:Text.Wrap;Layout.fillWidth:true }
                MButton { text:"Choose a pack file";iconName:"file";Layout.fillWidth:true;onClicked:{dialog.close();shell.chooseFile("pack")} }
                Text { text:".mlypack · .mrpack · .zip";color:Theme.textMuted;font.pixelSize:11 }
                MButton { text:"Import instance";iconName:"folder";kind:"primary";Layout.fillWidth:true;onClicked:{dialog.close();shell.importInstances()} }
                Row {spacing:12;Repeater {model:["Modrinth","Prism","CurseForge","Lunar Client","ATLauncher"];delegate:ProviderIcon {required property string modelData;provider:modelData;width:24;height:24}}}
                Text {text:"Automatically finds local instances. Choose a source, check the version, and bring your worlds along.";color:Theme.textMuted;font.pixelSize:12;wrapMode:Text.Wrap;Layout.fillWidth:true}
                Item { Layout.fillHeight:true }
            }
        }
    }
}
