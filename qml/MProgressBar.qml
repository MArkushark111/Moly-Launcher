import QtQuick
import QtQuick.Controls
ProgressBar {
    id:control
    implicitWidth:160
    implicitHeight:6
    padding:0
    background:Rectangle {radius:height/2;color:Theme.border}
    contentItem:Item {
        clip:true
        Rectangle {
            visible:!control.indeterminate
            width:Math.max(0,Math.min(1,control.visualPosition))*parent.width
            height:parent.height
            radius:height/2
            color:Theme.accent
            Behavior on width {NumberAnimation{duration:Theme.motion?160:0;easing.type:Easing.OutCubic}}
        }
    }
}
