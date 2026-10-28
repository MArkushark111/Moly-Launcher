import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

LiquidDialog {
    id:dialog
    property var entry:({})
    parent:Overlay.overlay;anchors.centerIn:parent;width:Math.min(480,parent.width-48);modal:true;padding:24
    title:entry.id?"Edit server shortcut":"Add a server"
    height:Math.min(body.implicitHeight+header.implicitHeight+topPadding+bottomPadding,parent.height-48)
    function edit(value){entry=value;serverName.text=value.name||"";address.text=value.address||"";instance.currentIndex=Math.max(0,launcher.instances.findIndex(function(i){return i.id===(value.instanceId||launcher.selectedId)}));open()}
    contentItem:ColumnLayout {
        id:body;spacing:12
        Text{text:"A quick-launch shortcut in Moly. Minecraft's own Multiplayer list stays intact.";color:Theme.textSecondary;font.pixelSize:12;wrapMode:Text.Wrap;Layout.fillWidth:true}
        MField{id:serverName;placeholderText:"Server name";maximumLength:100;Layout.fillWidth:true}
        MField{id:address;placeholderText:"play.example.com or IP:port";maximumLength:255;Layout.fillWidth:true}
        MComboBox{id:instance;model:launcher.instances.filter(function(i){return !i._pending});textRole:"name";valueRole:"id";Layout.fillWidth:true;enabled:!dialog.entry.id}
        RowLayout {Layout.alignment:Qt.AlignRight;MButton{text:"Cancel";onClicked:dialog.close()}MButton{text:"Save server";kind:"primary";enabled:serverName.text.trim().length>0&&address.text.trim().length>0&&instance.count>0;onClicked:{if(playTargets.saveServer({id:dialog.entry.id||"",instanceId:instance.currentValue,name:serverName.text,address:address.text}))dialog.close()}}}
    }
}
