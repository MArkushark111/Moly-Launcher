import QtQuick

// Native Qt arc, inspired by Loading UI's Arc. No web view or React runtime.
Item {
    id: spinner
    property color color: Theme.accent
    property bool running: true
    implicitWidth: 18
    implicitHeight: 18
    Canvas {
        id: arc
        anchors.fill: parent
        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            ctx.strokeStyle = spinner.color
            ctx.lineWidth = 2
            ctx.lineCap = "round"
            ctx.beginPath()
            ctx.arc(width/2, height/2, Math.max(1, width/2-2), -Math.PI/2, Math.PI)
            ctx.stroke()
        }
        Connections { target: spinner; function onColorChanged() { arc.requestPaint() } }
        RotationAnimator on rotation { from: 0; to: 360; duration: 900; loops: Animation.Infinite; running: spinner.running && spinner.visible && Theme.motion }
    }
}
