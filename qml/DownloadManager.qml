import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id:panel
    objectName:"downloadPanel"
    property var shell
    readonly property var completed: launcher.jobs.filter(function(job){return job.state!=="Working"})
    readonly property bool activeTask: launcher.busy&&["catalog","loaders","search","search-more"].indexOf(launcher.taskOperation)<0
    readonly property var visibleCompleted: launcher.settings.showCompletedTasks===false?[]:completed
    property double now: Date.now()
    width:Math.min(420,parent.width-32)
    height:Math.min(content.implicitHeight+36,parent.height-110)
    x:parent.width-width-18
    y:parent.height-height-90
    padding:18
    closePolicy:Popup.CloseOnEscape|Popup.CloseOnPressOutside
    z:8
    background:Rectangle {radius:20;color:Theme.surface;border.color:Theme.border}
    enter:Transition {NumberAnimation{property:"opacity";from:0;to:1;duration:Theme.motion?150:0}}
    exit:Transition {NumberAnimation{property:"opacity";to:0;duration:Theme.motion?100:0}}
    function amount(bytes){return bytes<1024*1024?(bytes/1024).toFixed(0)+" KB":(bytes/(1024*1024)).toFixed(1)+" MB"}
    function iconFor(id){const instance=launcher.instances.find(function(i){return i.id===id});return instance?instance.iconUrl||"":""}
    function elapsed(){const job=launcher.jobs.find(function(j){return j.state==="Working"});return job?Math.max(0,Math.floor((now-new Date(job.startedAt).getTime())/1000)):0}
    Timer{interval:1000;repeat:true;running:panel.visible&&panel.activeTask;onTriggered:panel.now=Date.now()}
    contentItem:ColumnLayout {
        id:content
        spacing:14
        RowLayout {
            Layout.fillWidth:true
            Icon{name:"download";size:19;color:Theme.accent}
            Text{text:"Task center";color:Theme.text;font.pixelSize:17;font.weight:Font.DemiBold;Layout.fillWidth:true}
            Text{visible:launcher.downloadSpeed>0;text:panel.amount(launcher.downloadSpeed)+"/s";color:Theme.textSecondary;font.pixelSize:11}
            MButton{iconName:"close";kind:"ghost";compact:true;Accessible.name:"Hide tasks";onClicked:panel.close()}
        }
        Rectangle {
            visible:panel.activeTask
            Layout.fillWidth:true
            implicitHeight:active.implicitHeight+24
            radius:13;color:Theme.surface2
            ColumnLayout {
                id:active;anchors.fill:parent;anchors.margins:12;spacing:10
                RowLayout {
                    Layout.fillWidth:true;spacing:10
                    InstanceAvatar{Layout.preferredWidth:34;Layout.preferredHeight:34;imageSource:panel.iconFor(launcher.taskInstanceId)}
                    Text{text:launcher.jobs.length&&launcher.jobs[0].state==="Working"?launcher.jobs[0].title:launcher.status;color:Theme.text;font.pixelSize:13;font.weight:Font.DemiBold;elide:Text.ElideRight;Layout.fillWidth:true}
                    MButton{iconName:"close";kind:"ghost";compact:true;labelColor:Theme.danger;onClicked:launcher.cancel();Accessible.name:"Cancel current task"}
                }
                Text{text:launcher.status.replace(/\n/g," ");color:Theme.textSecondary;font.pixelSize:11;wrapMode:Text.Wrap;Layout.fillWidth:true;maximumLineCount:3;elide:Text.ElideRight}
                MProgressBar{Layout.fillWidth:true;value:Math.max(0,launcher.progress);indeterminate:launcher.progress<0}
                RowLayout {
                    Layout.fillWidth:true
                    Text{text:launcher.downloadTotal>0?panel.amount(launcher.downloadedBytes)+" / "+panel.amount(launcher.downloadTotal):launcher.downloadedBytes>0?panel.amount(launcher.downloadedBytes)+" received":launcher.progress>=0?Math.round(launcher.progress*100)+"% of current step":"Preparing…";color:Theme.textMuted;font.pixelSize:11;Layout.fillWidth:true}
                    Text{text:panel.elapsed()+"s";color:Theme.textMuted;font.pixelSize:11}
                }
            }
        }
        RowLayout {
            visible:launcher.settings.showCompletedTasks!==false
            Layout.fillWidth:true
            Text{text:"Recent tasks";color:Theme.textSecondary;font.pixelSize:12;Layout.fillWidth:true}
            MButton{text:"Clear";kind:"ghost";compact:true;enabled:panel.completed.length>0;onClicked:launcher.clearCompletedJobs()}
        }
        ListView {
            visible:panel.visibleCompleted.length>0
            Layout.fillWidth:true;Layout.preferredHeight:Math.min(panel.visibleCompleted.length*65,240);clip:true;spacing:3
            model:panel.visibleCompleted
            ScrollBar.vertical:ScrollBar{}
            delegate:Item {
                required property var modelData
                width:ListView.view.width;height:62
                RowLayout {
                    anchors.fill:parent;spacing:10
                    InstanceAvatar{Layout.preferredWidth:32;Layout.preferredHeight:32;imageSource:panel.iconFor(modelData.instanceId)}
                    ColumnLayout{Layout.fillWidth:true;spacing:5;Text{text:modelData.title;color:Theme.text;font.pixelSize:12;elide:Text.ElideRight;Layout.fillWidth:true}Text{text:modelData.state+(modelData.finishedAt?" · "+new Date(modelData.finishedAt).toLocaleTimeString(Qt.locale(),"hh:mm"):"");color:modelData.state==="Failed"?Theme.danger:Theme.textMuted;font.pixelSize:11}}
                    MButton{iconName:"close";compact:true;kind:"ghost";onClicked:launcher.dismissJob(modelData.id);Accessible.name:"Dismiss task"}
                }
            }
        }
        Text {visible:panel.visibleCompleted.length===0&&!panel.activeTask;text:"Installations, exports, and backups appear here.";color:Theme.textMuted;font.pixelSize:12;wrapMode:Text.Wrap;Layout.fillWidth:true}
        MButton {visible:!launcher.busy&&launcher.canRetry;text:"Retry last task";iconName:"refresh";kind:"primary";Layout.alignment:Qt.AlignRight;onClicked:launcher.retry()}
    }
}
