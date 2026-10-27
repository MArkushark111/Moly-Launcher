import QtQuick

Item {
    id: root
    property url source
    property int fillMode: Image.PreserveAspectFit
    property bool animate: Theme.motion && launcher.settings.animateArtwork !== false
    readonly property bool isGif: source.toString().split("?")[0].toLowerCase().endsWith(".gif")
    readonly property int status: visual.item ? visual.item.status : Image.Null
    Loader {
        id: visual
        anchors.fill: parent
        sourceComponent: root.isGif && root.animate ? animated : still
    }
    Component {
        id: still
        Image { source: root.source; fillMode: root.fillMode; asynchronous: true; cache: true; smooth: true }
    }
    Component {
        id: animated
        AnimatedImage { source: root.source; fillMode: root.fillMode; playing: visible && root.animate; cache: false; smooth: true }
    }
}
