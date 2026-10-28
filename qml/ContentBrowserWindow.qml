import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: browser
    objectName:"contentBrowser"
    property var owner
    property string discoverType: "mod"
    property string discoverQuery: ""
    property string discoverProvider: "Modrinth"
    property string targetInstanceId: ""
    width:1080;height:720;minimumWidth:940;minimumHeight:600
    title:"Moly · Add content";color:Theme.app;modality:Qt.WindowModal
    function chooseFile(action,type) {close();owner.chooseFile(action,type)}
    function installProject(project) {close();owner.installProject(project)}
    DiscoverPage {anchors.fill:parent;shell:browser;instantInstall:true;targetInstanceId:browser.targetInstanceId}
    Connections {target:launcher;function onPlanChanged(){if(launcher.pendingPlan.title&&!launcher.installAutomatic)browser.close()}}
    onVisibleChanged: if(visible)discordPresence.setPage("discover:"+discoverType)
    onDiscoverTypeChanged: if(visible)discordPresence.setPage("discover:"+discoverType)
    onClosing:{discordPresence.setPage(owner.page);destroy()}
}
