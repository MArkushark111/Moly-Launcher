import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
Item {
    id:page
    property var shell
    ScrollView {
    id:localScroll
    anchors.fill:parent
    anchors.margins:24
    clip:true
    contentWidth:availableWidth
    ColumnLayout {
        width:localScroll.availableWidth
        spacing:16
        Text {text:"Local Minecraft library";color:Theme.text;font.pixelSize:26;font.weight:Font.DemiBold}
        Text {text:"Choose your Minecraft data folder. Moly can import modern vanilla versions you have already installed.";color:Theme.textSecondary;font.pixelSize:13;wrapMode:Text.Wrap;Layout.fillWidth:true}
        MButton {
            text: "Import Minecraft files"
            kind: "primary"
            enabled: !launcher.busy
            onClicked: page.shell.chooseFolder("localLibrary")
        }
        Repeater {
            model:launcher.versions
            delegate:Panel {
                required property var modelData
                Layout.fillWidth:true
                implicitHeight:84
                RowLayout {
                    anchors.fill:parent;anchors.margins:16;spacing:14
                    Icon {name:"box";size:25;color:Theme.accent}
                    ColumnLayout {Layout.fillWidth:true;Text{text:"Minecraft "+modelData.id;color:Theme.text;font.pixelSize:16;font.weight:Font.DemiBold}Text{text:"Java Edition · local files";color:Theme.textMuted;font.pixelSize:12}}
                    MButton {text:"Create setup";enabled:!launcher.busy;kind:"primary";onClicked:launcher.createInstance("Minecraft "+modelData.id,modelData.id,"Vanilla","")}
                }
            }
        }
        Text {text:"Microsoft sign-in requires internet. For play, use the matching Java runtime from your Minecraft installation or set its path in Settings → Advanced.";color:Theme.textMuted;font.pixelSize:12;wrapMode:Text.Wrap;Layout.fillWidth:true}
    }
}

}
