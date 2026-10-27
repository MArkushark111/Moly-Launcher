import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

LiquidDialog {
    id: dialog
    objectName: "installDialog"
    z:20
    property var shell
    property var project: ({})
    property bool projectMode: false
    property bool applying: false
    property bool resolving: false
    property bool waitingToResolve: false
    property bool removalOnly: false
    property bool detailsExpanded: false
    property string localError: ""
    property var review: launcher.pendingPlan
    property bool hasUnresolved: (review.files || []).some(function(f){return !f.url && !f.local})
    readonly property bool showFiles: !removalOnly && !launcher.busy && (hasUnresolved || detailsExpanded) && (review.files || []).length > 0
    width: Math.min(620, parent.width-64)
    height: Math.min(body.implicitHeight + header.implicitHeight + topPadding + bottomPadding, parent.height-64)
    anchors.centerIn:parent
    modal:true
    padding:24
    closePolicy:Popup.CloseOnEscape
    onClosed: { waitingToResolve=false }
    title: removalOnly ? "Remove content?" : projectMode ? "Install "+(project.title||"content") : review.title||"Review changes"
    function openProject(value) {
        project=value;projectMode=true;removalOnly=false;applying=false;detailsExpanded=false;localError=""
        launcher.dismissPlan()
        open()
        let target=launcher.installTargetId||launcher.selectedId
        let idx=launcher.instances.findIndex(function(i){return i.id===target})
        targetChoice.currentIndex=idx>=0?idx:0
        loadChoices()
    }
    function openReview(){projectMode=false;removalOnly=(review.files||[]).length===0&&(review.remove||[]).length>0;applying=false;detailsExpanded=false;localError="";open()}
    function loadChoices() {
        localError=""
        launcher.loadInstallChoices(project, project.type==="modpack" ? "" : targetChoice.currentValue || "")
    }
    function confirmInstall() {
        const target=project.type==="modpack" ? "" : targetChoice.currentValue || ""
        const version=versionChoice.currentValue || ""
        close()
        launcher.beginInstall(project,target,version,true)
    }
    Connections {
        target:launcher
        function onInstallCompleted(message){dialog.applying=false;dialog.close()}
        function onInstallFailed(message){dialog.localError=message;dialog.applying=false}
    }
    contentItem:ScrollView {
        id:contentScroll
        clip:true
        implicitHeight:body.implicitHeight
        contentWidth:availableWidth
        ScrollBar.horizontal.policy:ScrollBar.AlwaysOff
    ColumnLayout {
        id:body
        width:contentScroll.availableWidth
        spacing:14
        RowLayout {
            visible:dialog.projectMode;Layout.fillWidth:true;spacing:12
            Rectangle{width:44;height:44;radius:9;color:Theme.surface2;Icon{anchors.centerIn:parent;name:"box";size:24;visible:projectImage.status!==Image.Ready}Image{id:projectImage;anchors.fill:parent;anchors.margins:3;source:dialog.project.icon||"";asynchronous:true;fillMode:Image.PreserveAspectFit;visible:status===Image.Ready}}
            ColumnLayout{Layout.fillWidth:true;spacing:4
                Text{text:dialog.project.title||"";color:Theme.text;font.family:Theme.fontFamily;font.pixelSize:16;font.weight:Font.DemiBold;elide:Text.ElideRight;Layout.fillWidth:true}
                Text{text:dialog.project.provider||"";color:Theme.textSecondary;font.family:Theme.fontFamily;font.pixelSize:12}
            }
        }
        ColumnLayout{visible:dialog.projectMode&&dialog.project.type!=="modpack";Layout.fillWidth:true;spacing:6
            Label{text:"Install to";color:Theme.textSecondary;font.pixelSize:12}
            RowLayout{Layout.fillWidth:true;spacing:8
                MComboBox {id:targetChoice;Layout.fillWidth:true;model:launcher.instances;textRole:"name";valueRole:"id";enabled:!launcher.busy;onActivated:dialog.loadChoices()}
                MButton{visible:targetChoice.count===0;text:"New instance";onClicked:{dialog.close();shell.create()}}
            }
        }
        Text{visible:dialog.projectMode&&dialog.project.type==="modpack";text:dialog.review.instance?"Creates a separate instance: "+dialog.review.instance.loader+" · Minecraft "+dialog.review.instance.version+".":"This modpack will create a separate instance.";color:Theme.textSecondary;font.pixelSize:12;Layout.fillWidth:true;wrapMode:Text.Wrap}
        RowLayout{visible:dialog.projectMode&&launcher.installVersions.length>0;Layout.fillWidth:true
            Label{text:"Version";color:Theme.textSecondary;font.pixelSize:12;Layout.preferredWidth:58}
            MComboBox {id:versionChoice;objectName:"installVersionChoice";Layout.fillWidth:true;model:launcher.installVersions;textRole:"label";valueRole:"id";enabled:!launcher.loadingInstallChoices;currentIndex:0}
        }
        RowLayout { visible: dialog.projectMode && launcher.loadingInstallChoices; spacing: 10
            ArcSpinner { Layout.preferredWidth:18;Layout.preferredHeight:18 }
            Text { text:"Loading available versions…";color:Theme.textSecondary;font.pixelSize:12 }
        }
        Text { visible: dialog.projectMode; text:"Nothing installs until you confirm. Required dependencies are included; any restricted downloads or local changes will be shown before files are replaced.";color:Theme.textSecondary;font.pixelSize:12;wrapMode:Text.Wrap;Layout.fillWidth:true }
        Rectangle {
            visible:!!dialog.localError||(dialog.projectMode&&!!launcher.installError)
            Layout.fillWidth:true;implicitHeight:errorText.implicitHeight+24;radius:7;color:Theme.surface2;border.color:Theme.danger
            Text{id:errorText;anchors.fill:parent;anchors.margins:12;text:dialog.localError||launcher.installError;color:Theme.text;font.pixelSize:12;wrapMode:Text.Wrap}
        }
        Text{visible:dialog.removalOnly;text:"Remove the following file from this instance?";color:Theme.textSecondary;Layout.fillWidth:true;wrapMode:Text.Wrap;font.pixelSize:13}
        Repeater {
            model:dialog.removalOnly?(dialog.review.remove||[]):[]
            delegate:Rectangle {
                required property string modelData
                Layout.fillWidth:true;implicitHeight:60;radius:10;color:Theme.surface2;border.color:Theme.border
                RowLayout {anchors.fill:parent;anchors.margins:12;spacing:12
                    Icon {name:"trash";size:20;color:Theme.danger}
                    ColumnLayout {Layout.fillWidth:true;spacing:4
                        Text {text:modelData.split('/').pop();color:Theme.text;font.pixelSize:13;font.weight:Font.DemiBold;elide:Text.ElideMiddle;Layout.fillWidth:true}
                        Text {text:modelData;color:Theme.textMuted;font.pixelSize:11;elide:Text.ElideMiddle;Layout.fillWidth:true}
                    }
                }
            }
        }
        Text{visible:!dialog.removalOnly&&(dialog.review.remove||[]).length>0;text:"Remove: "+(dialog.review.remove||[]).join(", ");color:Theme.danger;Layout.fillWidth:true;wrapMode:Text.Wrap;font.pixelSize:12}
        RowLayout{visible:!dialog.removalOnly&&!launcher.busy&&(dialog.review.files||[]).length>0;Layout.fillWidth:true
            Text {
                text: {
                    const files=dialog.review.files||[]
                    const dependencies=files.filter(function(f){return f.dependency}).length
                    return files.length+(files.length===1?" file":" files")+(dependencies?", including "+dependencies+" required dependencies":"")
                }
                color:Theme.textSecondary;font.pixelSize:12;wrapMode:Text.Wrap;Layout.fillWidth:true
            }
            MButton{text:dialog.showFiles?"Hide files":"View files";kind:"ghost";compact:true;visible:!dialog.hasUnresolved;onClicked:dialog.detailsExpanded=!dialog.detailsExpanded}
        }
        ListView {
            id:reviewFiles
            visible:dialog.showFiles
            Layout.fillWidth:true;Layout.preferredHeight:Math.min(Math.max(88,dialog.parent.height-490),240,(dialog.review.files||[]).reduce(function(total,f){return total+(f.url||f.local?63:89)},0));clip:true;spacing:1;model:dialog.review.files||[]
            ScrollBar.vertical:ScrollBar{}
            delegate:Rectangle {
                required property var modelData
                required property int index
                width:ListView.view.width;height:modelData.url||modelData.local?62:88;radius:6;color:Theme.surface2
                RowLayout{anchors.fill:parent;anchors.margins:10;spacing:10
                    MCheckBox {checked:true;enabled:false;visible:!!modelData.dependency;Accessible.name:"Required dependency"}
                    Icon{visible:!modelData.dependency;name:"file";size:18;color:Theme.textMuted}
                    ColumnLayout{Layout.fillWidth:true;spacing:4
                        Text{text:modelData.title||modelData.filename;color:Theme.text;font.pixelSize:12;elide:Text.ElideRight;Layout.fillWidth:true}
                        Text{text:(modelData.dependency?"Required dependency · ":"")+(modelData.modified?"Local changes will be replaced · ":"")+(modelData.versionName||modelData.path||"");color:modelData.modified?Theme.danger:Theme.textSecondary;font.pixelSize:11;elide:Text.ElideMiddle;Layout.fillWidth:true}
                        RowLayout{visible:!modelData.url&&!modelData.local;spacing:8
                            MButton{text:"Download in browser";compact:true;onClicked:launcher.openLink(modelData.page||"https://www.curseforge.com/minecraft")}
                            MButton{text:"Select downloaded file";compact:true;onClicked:{shell.resolveIndex=index;shell.chooseFile("resolve")}}
                        }
                    }
                    Icon{visible:!!modelData.local;name:"check";size:16;color:Theme.textSecondary}
                }
            }
        }
        ColumnLayout{visible:!dialog.projectMode&&launcher.busy;Layout.fillWidth:true;spacing:6
            Text{text:dialog.waitingToResolve?"Waiting for the current task…":dialog.applying?"Installing…":"Preparing install…";color:Theme.text;font.pixelSize:13;font.weight:Font.DemiBold}
            MProgressBar{Layout.fillWidth:true;value:Math.max(0,launcher.progress);indeterminate:launcher.progress<0}
            Text{visible:launcher.progress>=0;text:Math.round(launcher.progress*100)+"%";color:Theme.text;font.pixelSize:13;font.weight:Font.DemiBold}
            Text{text:launcher.status.replace(/\n/g," ");color:Theme.textSecondary;font.pixelSize:11;elide:Text.ElideRight;Layout.fillWidth:true}
        }
        Text{visible:!dialog.projectMode&&!launcher.busy;text:dialog.removalOnly?"A backup will be kept. You can restore it from Instance settings → Content recovery.":dialog.hasUnresolved?"Some authors require a browser download. Select those files below before installing.":"Required dependencies are included. Confirm to install; replaced files are backed up automatically.";color:Theme.textSecondary;font.pixelSize:12;wrapMode:Text.Wrap;Layout.fillWidth:true}
        RowLayout{Layout.alignment:Qt.AlignRight;spacing:8
            MButton{text:!dialog.projectMode&&launcher.busy?"Cancel task":"Cancel";onClicked:{if(!dialog.projectMode&&launcher.busy)launcher.cancel();else{launcher.dismissPlan();dialog.close()}}}
            MButton{text:"Retry versions";visible:dialog.projectMode&&!!launcher.installError;onClicked:dialog.loadChoices()}
            MButton{objectName:"confirmInstallButton";text:dialog.removalOnly?"Remove":(dialog.review.remove||[]).length>0?"Apply changes":"Confirm & install";kind:dialog.removalOnly?"danger":"primary";enabled:dialog.projectMode ? !launcher.busy&&!launcher.loadingInstallChoices&&launcher.installVersions.length>0 : !launcher.busy&&!dialog.hasUnresolved&&((dialog.review.files||[]).length>0||(dialog.review.remove||[]).length>0);onClicked:{if(dialog.projectMode)dialog.confirmInstall();else{dialog.applying=true;dialog.localError="";launcher.applyPlan()}}}
        }
    }
    }
}
