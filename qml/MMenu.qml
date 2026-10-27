import QtQuick
import QtQuick.Controls

Menu {
    id: control
    padding: 6
    implicitWidth: 238
    background: Rectangle { radius: 14; color: Theme.surface; border.color: Theme.border }
    enter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.motion ? 130 : 0 } }
    exit: Transition { NumberAnimation { property: "opacity"; to: 0; duration: Theme.motion ? 90 : 0 } }
}
