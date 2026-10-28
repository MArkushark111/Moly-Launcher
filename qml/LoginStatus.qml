import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id:status
    parent:Overlay.overlay;z:100;x:parent.width-width-18;y:18;width:Math.min(370,parent.width-36)
    height:body.implicitHeight+32;padding:16;modal:false;dim:false;focus:false;closePolicy:Popup.NoAutoClose
    readonly property bool pending:launcher.loginState==="requesting"||launcher.loginState==="waiting"
    readonly property bool connected:launcher.loginState==="success"
    background:Rectangle{radius:18;color:Theme.surface;border.color:status.connected?Theme.success:Theme.border;Behavior on border.color{ColorAnimation{duration:Theme.motion?200:0}}}
    enter:Transition{ParallelAnimation{NumberAnimation{property:"opacity";from:0;to:1;duration:Theme.motion?180:0}NumberAnimation{property:"scale";from:Theme.motion?0.95:1;to:1;duration:Theme.motion?240:0;easing.type:Easing.OutCubic}}}
    exit:Transition{NumberAnimation{property:"opacity";to:0;duration:Theme.motion?350:0}}
    property string lastUpdate:""
    Connections{target:launcher;function onLoginChanged(){const key=launcher.loginState+"/"+launcher.loginCode+"/"+launcher.loginMessage;if(key===status.lastUpdate)return;status.lastUpdate=key;dismiss.stop();if(launcher.loginState!=="idle"){status.open();if(!status.pending&&launcher.loginState!=="error")dismiss.start()}}}
    Timer{id:dismiss;interval:2800;onTriggered:status.close()}
    contentItem:ColumnLayout {
        id:body;spacing:12
        RowLayout {
            Layout.fillWidth:true;spacing:10
            Icon{name:"microsoft";size:20}
            Text{text:status.connected?"Microsoft connected":status.pending?"Microsoft sign-in":launcher.loginState==="error"?"Sign-in needs attention":"Sign-in cancelled";color:status.connected?Theme.success:Theme.text;font.pixelSize:14;font.weight:Font.DemiBold;Layout.fillWidth:true}
            ArcSpinner{visible:status.pending;width:18;height:18}
            Icon{visible:status.connected;name:"check";size:20;color:Theme.success}
            MButton{visible:!status.pending;iconName:"close";kind:"ghost";compact:true;implicitWidth:28;Accessible.name:"Dismiss sign-in status";onClicked:status.close()}
        }
        Text{text:launcher.loginMessage;color:launcher.loginState==="error"?Theme.danger:Theme.textSecondary;font.pixelSize:12;wrapMode:Text.Wrap;Layout.fillWidth:true}
        Rectangle {
            visible:!!launcher.loginCode;Layout.fillWidth:true;implicitHeight:58;radius:11;color:Theme.surface2
            RowLayout{anchors.fill:parent;anchors.margins:10;TextEdit{text:launcher.loginCode;readOnly:true;selectByMouse:true;color:Theme.text;font.family:"Consolas";font.pixelSize:23;font.weight:Font.Bold;Layout.fillWidth:true}MButton{iconName:"copy";kind:"ghost";compact:true;Accessible.name:"Copy Microsoft code";onClicked:launcher.copyLoginCode()}}
        }
        RowLayout {
            Layout.fillWidth:true
            MButton{visible:!!launcher.loginCode;text:"Open browser";compact:true;kind:"ghost";onClicked:launcher.openLoginBrowser()}
            Item{Layout.fillWidth:true}
            MButton{visible:status.pending;text:"Cancel sign-in";compact:true;onClicked:launcher.cancelSignIn()}
            MButton{visible:launcher.loginState==="error";text:"Try again";compact:true;kind:"primary";onClicked:launcher.signIn()}
        }
    }
}
