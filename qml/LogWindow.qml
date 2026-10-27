import QtQuick
import QtQuick.Controls
Window {
    id: logWindow
    property var shell
    width: 1000; height: 640; minimumWidth: 700; minimumHeight: 400
    title: "Moly · " + (launcher.selected.name || "Minecraft") + " · Live log"
    color: Theme.surface
    LogView { anchors.fill:parent; anchors.margins:20; shell:logWindow.shell; popped:true }
    onClosing: destroy()
}
