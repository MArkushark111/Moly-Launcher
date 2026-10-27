import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: page
    objectName: "instanceSettings"
    property var shell
    property var instance: ({})
    property int revision: 0
    property int section: 0
    property var draft: ({})
    property string draftId: ""
    function loadDraft() { if (draftId !== (instance.id || "")) { draftId=instance.id||"";draft=instance; } }
    Component.onCompleted: loadDraft()
    onInstanceChanged: loadDraft()
    function save() {
        const settings=Object.assign({},draft.settings||{})
        delete settings.memoryMb; delete settings.width; delete settings.height; delete settings.fullscreen; delete settings.keepVisible; delete settings.javaPath; delete settings.jvmArgs
        if (memoryOverride.checked) settings.memoryMb=Math.round(memory.value)
        if (windowOverride.checked) { settings.width=gameWidth.value; settings.height=gameHeight.value; settings.fullscreen=fullscreen.checked }
        if (visibilityOverride.checked) settings.keepVisible=keepVisible.checked
        if (javaPath.text.trim()) settings.javaPath=javaPath.text.trim()
        if (jvmArgs.text.trim()) settings.jvmArgs=jvmArgs.text.trim()
        launcher.saveInstance({name:instanceName.text,group:instanceGroup.text,settings:settings})
    }
    ColumnLayout {
        anchors.fill:parent;spacing:14
        RowLayout {
            Layout.fillWidth:true
            TabStrip { tabs:[{label:"Identity & art",icon:"file"},{label:"Game",icon:"play"},{label:"Recovery",icon:"refresh"}];currentIndex:page.section;onActivated:function(index){page.section=index} }
            Item { Layout.fillWidth:true }
            MButton { text:"Save";kind:"primary";enabled:!launcher.busy&&!page.instance.running;onClicked:page.save() }
        }
        ScrollView {
            id:scroll;Layout.fillWidth:true;Layout.fillHeight:true;clip:true;contentWidth:availableWidth
            ColumnLayout {
                width:scroll.availableWidth;spacing:14
                Panel {
                    visible:page.section===0;Layout.fillWidth:true
                    contentItem:ColumnLayout {
                        spacing:16
                        Text { text:"Your instance";color:Theme.text;font.pixelSize:17;font.weight:Font.DemiBold }
                        MField { id:instanceName;Layout.fillWidth:true;text:page.draft.name||"";placeholderText:"Instance name";Accessible.name:"Instance name" }
                        MField { id:instanceGroup;Layout.fillWidth:true;text:page.draft.group||"";placeholderText:"Group";Accessible.name:"Instance group" }
                        RowLayout {
                            Layout.fillWidth:true;spacing:12
                            InstanceAvatar { Layout.preferredWidth:56;Layout.preferredHeight:56;imageSource:page.instance.iconUrl||"" }
                            MButton { text:"Change icon";iconName:"file";enabled:!launcher.busy&&!page.instance.running;onClicked:shell.chooseFile("icon") }
                            MButton { text:"Reset";kind:"ghost";enabled:!launcher.busy&&!page.instance.running;onClicked:launcher.resetInstanceIcon() }
                        }
                        Text { text:"Tile background";color:Theme.textSecondary;font.pixelSize:13 }
                        RowLayout {
                            Layout.fillWidth:true
                            MButton { text:"Choose image or GIF";iconName:"file";enabled:!launcher.busy&&!page.instance.running;onClicked:shell.chooseFile("background") }
                            MButton { text:"Clear";kind:"ghost";enabled:!launcher.busy&&!page.instance.running;onClicked:launcher.resetInstanceBackground() }
                        }
                        Text { text:"Artwork appears on the sidebar and Library cards. Reduce motion pauses GIFs.";color:Theme.textMuted;font.pixelSize:12;wrapMode:Text.Wrap;Layout.fillWidth:true }
                    }
                }
                Panel {
                    visible:page.section===1;Layout.fillWidth:true
                    contentItem:ColumnLayout {
                        spacing:12
                        Text { text:"Game overrides";color:Theme.text;font.pixelSize:17;font.weight:Font.DemiBold }
                        Text { text:"Unchecked options follow your global defaults.";color:Theme.textSecondary;font.pixelSize:12 }
                        MCheckBox { id:memoryOverride;text:"Use a different memory allocation";checked:page.draft.settings&&page.draft.settings.memoryMb!==undefined }
                        RowLayout {
                            visible:memoryOverride.checked;Layout.fillWidth:true
                            MSlider { id:memory;Layout.fillWidth:true;from:512;to:Math.max(1024,launcher.memoryTotal-1024);stepSize:256;value:page.draft.settings&&page.draft.settings.memoryMb||launcher.settings.memoryMb }
                            Text { text:(memory.value/1024).toFixed(1)+" GB";color:Theme.text;Layout.preferredWidth:64 }
                        }
                        Text { visible:memoryOverride.checked&&memory.value>launcher.memoryTotal*0.65;text:"This leaves little memory for Windows and other apps.";color:Theme.danger;font.pixelSize:12;wrapMode:Text.Wrap;Layout.fillWidth:true }
                        MCheckBox { id:windowOverride;text:"Use a different game window";checked:page.draft.settings&&(page.draft.settings.width!==undefined||page.draft.settings.fullscreen!==undefined) }
                        RowLayout {
                            visible:windowOverride.checked;Layout.fillWidth:true;spacing:10
                            MSpinBox { id:gameWidth;from:640;to:7680;stepSize:80;editable:true;value:page.draft.settings&&page.draft.settings.width||launcher.settings.width;Layout.preferredWidth:128 }
                            Text { text:"×";color:Theme.textSecondary }
                            MSpinBox { id:gameHeight;from:360;to:4320;stepSize:40;editable:true;value:page.draft.settings&&page.draft.settings.height||launcher.settings.height;Layout.preferredWidth:128 }
                            MCheckBox { id:fullscreen;text:"Fullscreen";checked:page.draft.settings&&page.draft.settings.fullscreen!==undefined?page.draft.settings.fullscreen:launcher.settings.fullscreen===true }
                        }
                        MCheckBox { id:visibilityOverride;text:"Override launcher behavior during play";checked:page.draft.settings&&page.draft.settings.keepVisible!==undefined }
                        RowLayout {
                            visible:visibilityOverride.checked;Layout.fillWidth:true
                            Text { text:"Keep Moly visible";color:Theme.text;font.pixelSize:12;Layout.fillWidth:true }
                            MSwitch { id:keepVisible;checked:page.draft.settings&&page.draft.settings.keepVisible!==undefined?page.draft.settings.keepVisible:launcher.settings.keepVisible!==false }
                        }
                        Text { text:"Advanced Java options";color:Theme.textSecondary;font.pixelSize:13;Layout.topMargin:8 }
                        MField { id:javaPath;Layout.fillWidth:true;text:page.draft.settings&&page.draft.settings.javaPath||"";placeholderText:"Java executable — automatic if blank" }
                        MField { id:jvmArgs;Layout.fillWidth:true;text:page.draft.settings&&page.draft.settings.jvmArgs||"";placeholderText:"Additional JVM arguments — global if blank" }
                    }
                }
                Panel {
                    visible:page.section===2;Layout.fillWidth:true
                    contentItem:ColumnLayout {
                        spacing:14
                        Text { text:"Content recovery";color:Theme.text;font.pixelSize:17;font.weight:Font.DemiBold }
                        Text { text:"Restore files changed by an earlier install or update. Worlds have their own backup action.";color:Theme.textSecondary;font.pixelSize:12;Layout.fillWidth:true;wrapMode:Text.Wrap }
                        MComboBox { id:backupChoice;Layout.fillWidth:true;model:{page.revision;return launcher.backups()} }
                        RowLayout {
                            MButton { text:"Restore content";iconName:"refresh";enabled:backupChoice.count>0&&!launcher.busy&&!page.instance.running;onClicked:shell.confirm("restore","Restore content backup?","Later changes to the affected files will be replaced.",backupChoice.currentText) }
                            MButton { text:"Back up worlds";iconName:"world";enabled:!launcher.busy&&!page.instance.running;onClicked:launcher.backupWorlds() }
                        }
                    }
                }
            }
        }
    }
}
