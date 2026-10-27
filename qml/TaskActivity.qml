import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

AbstractButton {
    id: control
    objectName: "taskActivity"
    property real maximumWidth: 210
    readonly property bool activeTask: launcher.busy && ["catalog","loaders","search","search-more"].indexOf(launcher.taskOperation)<0
    readonly property string taskTitle: launcher.jobs.length && launcher.jobs[0].state === "Working" ? launcher.jobs[0].title : launcher.status
    width: activeTask ? Math.min(210, maximumWidth) : 94
    height: 40
    hoverEnabled: true
    Accessible.name: activeTask ? "Open task center: " + taskTitle : "Open task center"
    Behavior on width { NumberAnimation { duration: Theme.motion ? 300 : 0; easing.type: Easing.OutQuint } }
    background: Rectangle {
        radius: 13; color: control.hovered ? Theme.surface2 : Theme.surface; border.color: Theme.border
        Rectangle { visible: control.activeTask && launcher.progress>=0; anchors.bottom: parent.bottom; anchors.bottomMargin: 3; x: 11; height: 3; radius: 2; color: Theme.accent; width: Math.max(0,(parent.width-22)*launcher.progress); Behavior on width { NumberAnimation { duration: Theme.motion ? 150 : 0 } } }
    }
    contentItem: Item {
        clip: true
        ArcSpinner { visible: control.activeTask; x:12; anchors.verticalCenter:parent.verticalCenter; width:18;height:18 }
        Icon { visible: !control.activeTask; name:"download";size:16;x:12;anchors.verticalCenter:parent.verticalCenter;color:Theme.textSecondary }
        Text { x:39; anchors.verticalCenter:parent.verticalCenter; width:Math.max(0,parent.width-x-12);text:control.activeTask?(launcher.downloadSpeed>0?(launcher.downloadSpeed/1048576).toFixed(1)+" MB/s · ":launcher.progress>=0?Math.round(launcher.progress*100)+"% · ":"")+control.taskTitle:"Tasks";elide:Text.ElideRight;color:Theme.text;font.family:Theme.fontFamily;font.pixelSize:11;verticalAlignment:Text.AlignVCenter }
    }
    ToolTip.visible: hovered
    ToolTip.text: activeTask ? launcher.status : "Open task center"
}
