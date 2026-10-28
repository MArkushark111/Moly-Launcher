import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

LiquidDialog {
    id:dialog
    property var shell
    parent:Overlay.overlay;anchors.centerIn:parent;width:Math.min(480,parent.width-48);modal:true;padding:24
    title:"Welcome to Moly"
    height:Math.min(body.implicitHeight+header.implicitHeight+topPadding+bottomPadding,parent.height-48)
    contentItem:ColumnLayout {
        id:body;spacing:16
        Text{text:"Choose how you want to play.";color:Theme.textSecondary;font.pixelSize:14;Layout.fillWidth:true}
        MButton {text:"Sign in with Microsoft";iconName:"external";kind:"primary";Layout.fillWidth:true;enabled:!launcher.busy;onClicked:{if(launcher.microsoftAvailable){launcher.setUiPreferences({accountSetupDone:true});dialog.close();launcher.signIn()}else message.visible=true}}
        Text{id:message;visible:false;text:"Microsoft sign-in is not available in this build yet. Moly's owner needs to finish its app registration. You can use a local account now.";color:Theme.textSecondary;font.pixelSize:12;Layout.fillWidth:true;wrapMode:Text.Wrap}
        Rectangle{height:1;color:Theme.border;Layout.fillWidth:true}
        Text{text:"Local account";color:Theme.text;font.pixelSize:15;font.weight:Font.DemiBold}
        MField{id:nickname;placeholderText:"Minecraft nickname";maximumLength:16;Layout.fillWidth:true;onAccepted:localButton.clicked()}
        Text{text:"For local play. Authenticated servers require Microsoft sign-in.";color:Theme.textMuted;font.pixelSize:12;Layout.fillWidth:true;wrapMode:Text.Wrap}
        MButton{id:localButton;text:"Create local account";iconName:"user";Layout.fillWidth:true;enabled:/^[A-Za-z0-9_]{3,16}$/.test(nickname.text)&&!launcher.busy;onClicked:{launcher.addOfflineAccount(nickname.text);launcher.setUiPreferences({accountSetupDone:true});dialog.close()}}
        MCheckBox{text:"Show this chooser on startup";checked:launcher.settings.showAccountChooser===true;onClicked:launcher.setUiPreferences({showAccountChooser:checked})}
        MButton{text:"Later";kind:"ghost";Layout.alignment:Qt.AlignRight;onClicked:{launcher.setUiPreferences({accountSetupDone:true});dialog.close()}}
    }
}
