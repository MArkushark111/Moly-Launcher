pragma Singleton
import QtQuick

QtObject {
    readonly property color app: "#121014"
    readonly property color sidebar: "#17151b"
    readonly property color surface: "#1b1920"
    readonly property color surface2: "#221f29"
    readonly property color border: "#2a2733"
    readonly property color text: "#ece9f4"
    readonly property color textSecondary: "#a7a2b6"
    readonly property color textMuted: "#7d7889"
    readonly property color accent: launcher.settings.accentColor || "#8b5cf6"
    readonly property color accentHover: Qt.darker(accent, 1.1)
    readonly property color accentSoft: Qt.rgba(accent.r, accent.g, accent.b, 0.14)
    readonly property color onAccent: contrast(text, accent) >= contrast(app, accent) ? text : app
    readonly property color success: "#3ecf8e"
    readonly property color danger: "#f25f5c"
    readonly property color serverAccent: launcher.settings.pingColor || accent
    readonly property color transparent: "transparent"
    readonly property string fontFamily: inter.status === FontLoader.Ready ? inter.name : "Segoe UI"
    readonly property bool motion: !launcher.settings.reducedMotion
    property FontLoader inter: FontLoader { source: "../assets/Inter.ttf" }
    function luminance(color) {
        function linear(v) { return v <= 0.04045 ? v / 12.92 : Math.pow((v + 0.055) / 1.055, 2.4) }
        return 0.2126*linear(color.r)+0.7152*linear(color.g)+0.0722*linear(color.b)
    }
    function contrast(a,b) { const x=luminance(a), y=luminance(b); return (Math.max(x,y)+0.05)/(Math.min(x,y)+0.05) }
}
