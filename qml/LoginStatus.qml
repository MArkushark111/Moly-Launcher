import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id:status
    objectName:"loginDesktopOverlay"
    property var owner
    transientParent:null
    flags:Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.WindowDoesNotAcceptFocus
    color:"transparent";visible:false;title:"Moly · Microsoft sign-in"
    width:370;height:body.implicitHeight+32
    screen:owner?owner.screen:null
    x:screen.virtualX+screen.width-width-18;y:screen.virtualY+18
    readonly property bool pending:launcher.loginState==="requesting"||launcher.loginState==="waiting"
    readonly property bool connected:launcher.loginState==="success"
    function open(){fadeOut.stop();visible=true;panel.opacity=0;fadeIn.restart()}
    function dismissCard(){fadeIn.stop();fadeOut.restart()}
    NumberAnimation{id:fadeIn;target:panel;property:"opacity";to:1;duration:Theme.motion?180:0}
    SequentialAnimation{id:fadeOut;NumberAnimation{target:panel;property:"opacity";to:0;duration:Theme.motion?350:0}ScriptAction{script:status.hide()}}
    property bool copied:false
    Timer{id:copyReset;interval:1800;onTriggered:status.copied=false}
    Connections{target:launcher;function onLoginCodeCopied(){status.copied=true;copyReset.restart()}function onLoginChanged(){status.copied=false;copyReset.stop()}}
    property string lastUpdate:""
    Connections{target:launcher;function onLoginChanged(){const key=launcher.loginState+"/"+launcher.loginCode+"/"+launcher.loginMessage;if(key===status.lastUpdate)return;status.lastUpdate=key;dismiss.stop();if(launcher.loginState!=="idle"){status.open();if(!status.pending&&launcher.loginState!=="error")dismiss.start()}}}
    Timer{id:dismiss;interval:2800;onTriggered:status.dismissCard()}
    Rectangle {
        id:panel;anchors.fill:parent;radius:18;color:Theme.surface;border.color:status.connected?Theme.success:Theme.border
        Behavior on border.color{ColorAnimation{duration:Theme.motion?200:0}}
    ColumnLayout {
        anchors.fill:parent;anchors.margins:16
        id:body;spacing:12
        RowLayout {
            Layout.fillWidth:true;spacing:10
            Icon{name:"microsoft";size:20}
            Text{text:status.connected?"Microsoft connected":status.pending?"Microsoft sign-in":launcher.loginState==="error"?"Sign-in needs attention":"Sign-in cancelled";color:status.connected?Theme.success:Theme.text;font.pixelSize:14;font.weight:Font.DemiBold;Layout.fillWidth:true}
            ArcSpinner{visible:status.pending;width:18;height:18}
            Icon{visible:status.connected;name:"check";size:20;color:Theme.success}
            MButton{visible:!status.pending;iconName:"close";kind:"ghost";compact:true;implicitWidth:28;Accessible.name:"Dismiss sign-in status";onClicked:status.dismissCard()}
        }
        Text{text:launcher.loginMessage;color:launcher.loginState==="error"?Theme.danger:Theme.textSecondary;font.pixelSize:12;wrapMode:Text.Wrap;Layout.fillWidth:true}
        Rectangle {
            visible:!!launcher.loginCode;Layout.fillWidth:true;implicitHeight:58;radius:11;color:Theme.surface2
            RowLayout{anchors.fill:parent;anchors.margins:10;TextEdit{text:launcher.loginCode;readOnly:true;selectByMouse:true;color:Theme.text;font.family:"Consolas";font.pixelSize:23;font.weight:Font.Bold;Layout.fillWidth:true}MButton{text:status.copied?"Copied":"";iconName:status.copied?"check":"copy";kind:"ghost";compact:true;labelColor:status.copied?Theme.success:Theme.text;Accessible.name:status.copied?"Code copied":"Copy Microsoft code";onClicked:launcher.copyLoginCode()}}
        }
        RowLayout {
            Layout.fillWidth:true
            MButton{visible:!!launcher.loginCode;text:"Open browser";compact:true;kind:"ghost";onClicked:launcher.openLoginBrowser()}
            Item{Layout.fillWidth:true}
            MButton{visible:status.pending;text:"Cancel sign-in";compact:true;onClicked:launcher.cancelSignIn()}
            MButton{visible:launcher.loginState==="error"&&launcher.loginNeedsReview;text:"Access help";compact:true;kind:"primary";onClicked:launcher.openLink("https://aka.ms/mce-reviewappid")}
            MButton{visible:launcher.loginState==="error"&&!launcher.loginNeedsReview;text:"Try again";compact:true;kind:"primary";onClicked:launcher.signIn()}
        }
    }
}
}
