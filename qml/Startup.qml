import QtQuick
import QtQuick.Window
import QtQuick.Layouts

Window {
    id: boot
    objectName:"startupWindow"
    width:420;height:260
    x:Screen.virtualX+(Screen.width-width)/2;y:Screen.virtualY+(Screen.height-height)/2
    flags:Qt.FramelessWindowHint|Qt.SplashScreen
    color:"transparent"
    visible:!startup.ready
    title:"Starting Moly"
    Rectangle {
        anchors.fill:parent;radius:23;color:"#17151b";border.color:"#302b39"
        ColumnLayout {
            anchors.fill:parent;anchors.margins:30;spacing:12
            Image {source:"../assets/moly.svg";sourceSize.width:112;sourceSize.height:112;Layout.preferredWidth:56;Layout.preferredHeight:56;Layout.alignment:Qt.AlignHCenter}
            Text {text:"Moly";color:"#ece9f4";font.family:"Inter";font.pixelSize:23;font.weight:Font.DemiBold;Layout.alignment:Qt.AlignHCenter}
            Item {Layout.fillHeight:true}
            RowLayout {
                Layout.fillWidth:true
                Text {text:startup.phase;color:"#a7a2b6";font.family:"Inter";font.pixelSize:12;Layout.fillWidth:true}
                Text {text:startup.completed+" / "+startup.total;color:"#7d7889";font.family:"Inter";font.pixelSize:11}
            }
            Rectangle {Layout.fillWidth:true;height:5;radius:3;color:"#2a2733";Rectangle {height:parent.height;radius:3;width:parent.width*startup.progress;color:startup.accent}}
        }
    }
}
