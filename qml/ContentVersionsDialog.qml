import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

LiquidDialog {
    id:dialog
    objectName:"contentVersionsDialog"
    property var file:({})
    property var chosen:({})
    property string error:""
    readonly property var versions:launcher.installVersions
    readonly property int installedIndex:versions.findIndex(function(v){return v.id===file.versionId})
    readonly property int chosenIndex:versions.findIndex(function(v){return v.id===chosen.id})
    readonly property bool same:!!chosen.id&&chosen.id===file.versionId
    parent:Overlay.overlay;anchors.centerIn:parent;width:Math.min(620,parent.width-48);height:Math.min(580,parent.height-48)
    title:"Versions · "+(file.title||file.name||"Content");modal:true;padding:22
    function openFile(value) {file=value;chosen=({});open();launcher.loadInstallChoices(value,launcher.selectedId)}
    contentItem:ColumnLayout {
        spacing:12
        Text {text:"Compatible with "+launcher.selected.loader+" "+launcher.selected.version+" · files are backed up before replacement.";color:Theme.textSecondary;font.pixelSize:12;wrapMode:Text.Wrap;Layout.fillWidth:true}
        RowLayout {visible:launcher.loadingInstallChoices;ArcSpinner{width:18;height:18}Text{text:"Loading versions…";color:Theme.textSecondary}}
        Text {visible:!!launcher.installError;text:launcher.installError;color:Theme.danger;wrapMode:Text.Wrap;Layout.fillWidth:true}
        ListView {
            Layout.fillWidth:true;Layout.fillHeight:true;clip:true;spacing:6;model:dialog.versions;ScrollBar.vertical:ScrollBar{}
            delegate:ItemDelegate {
                id:versionRow;required property var modelData;width:ListView.view.width;height:58
                onClicked:dialog.chosen=modelData
                background:Rectangle{radius:12;color:dialog.chosen.id===versionRow.modelData.id?Theme.accentSoft:versionRow.hovered?Theme.surface2:Theme.surface;border.color:Theme.border}
                contentItem:RowLayout {
                    spacing:12
                    Rectangle {width:32;height:32;radius:10;color:versionRow.modelData.channel==="release"?Qt.rgba(0.24,0.81,0.56,0.12):Theme.accentSoft;Icon{anchors.centerIn:parent;name:versionRow.modelData.channel==="release"?"check":versionRow.modelData.channel==="beta"?"flask":"sparkle";size:17;color:versionRow.modelData.channel==="release"?Theme.success:Theme.accent}}
                    ColumnLayout {Layout.fillWidth:true;spacing:3;Text{text:versionRow.modelData.name;color:Theme.text;font.pixelSize:13;elide:Text.ElideRight;Layout.fillWidth:true}Text{text:(versionRow.modelData.channel||"release")+" · "+(versionRow.modelData.published||"").slice(0,10);color:Theme.textMuted;font.pixelSize:11}}
                    Text {text:versionRow.modelData.id===dialog.file.versionId?"Installed":"";color:Theme.textMuted;font.pixelSize:12}
                }
            }
        }
        RowLayout {
            Layout.fillWidth:true
            MButton{text:"Cancel";onClicked:dialog.close()}
            Item{Layout.fillWidth:true}
            MButton {text:dialog.same?"Installed":dialog.installedIndex<0?"Install version":dialog.chosenIndex<dialog.installedIndex?"Upgrade":"Downgrade";kind:dialog.same?"secondary":"primary";enabled:!!dialog.chosen.id&&!dialog.same&&!launcher.busy;onClicked:{const project=dialog.file,version=dialog.chosen.id;dialog.close();launcher.beginInstall(project,launcher.selectedId,version,true)}}
        }
    }
}
