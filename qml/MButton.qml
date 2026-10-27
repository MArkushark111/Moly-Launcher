import QtQuick
import QtQuick.Controls

Button {
    id: control

    property string kind: "secondary"
    property string iconName: ""
    property bool compact: false
    property bool loading: false
    property color labelColor: !enabled && !loading ? Theme.textMuted : kind === "danger" ? Theme.danger : kind === "primary" ? Theme.onAccent : Theme.text

    implicitHeight: compact ? 32 : 38
    implicitWidth: Math.max(iconName && !text ? implicitHeight : 68, label.implicitWidth + (glyph.visible ? glyph.width : 0) + (glyph.visible && label.visible ? contentRow.spacing : 0) + leftPadding + rightPadding)
    leftPadding: iconName && !text ? 8 : 14
    rightPadding: leftPadding
    topPadding: 0
    bottomPadding: 0
    hoverEnabled: true
    font.family: Theme.fontFamily
    font.pixelSize: 13
    font.weight: Font.Medium
    Accessible.name: text || iconName

    // A natural-width group is centered as one unit. Stretching a RowLayout label
    // left the icon at one edge and made the visible button content look off-centre.
    contentItem: Item {
        implicitWidth: label.implicitWidth + (glyph.visible ? glyph.width : 0) + (glyph.visible && label.visible ? contentRow.spacing : 0)
        implicitHeight: Math.max(label.implicitHeight, glyph.visible ? glyph.height : 0)

        ArcSpinner { visible: control.loading; x: contentRow.x; anchors.verticalCenter: parent.verticalCenter; width: 16; height: 16; color: control.labelColor }
        Row {
            id: contentRow
            anchors.centerIn: parent
            spacing: 7

            Icon {
                id: glyph
                visible: !!control.iconName || control.loading
                name: control.loading ? "" : control.iconName
                color: control.loading ? Theme.transparent : control.labelColor
                size: 16
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                id: label
                visible: !!control.text
                text: control.text
                color: control.labelColor
                font: control.font
                width: Math.min(implicitWidth, Math.max(0, control.availableWidth - (glyph.visible ? glyph.width + contentRow.spacing : 0)))
                height: implicitHeight
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    scale: down && enabled && Theme.motion ? 0.975 : 1
    Behavior on scale {
        NumberAnimation {
            duration: Theme.motion ? 120 : 0
            easing.type: Easing.OutCubic
        }
    }

    background: Rectangle {
        radius: control.height / 3
        color: control.kind === "primary" ? ((control.enabled || control.loading) ? (control.hovered || control.down ? Theme.accentHover : Theme.accent) : Theme.surface2) : control.kind === "danger" ? Qt.rgba(Theme.danger.r, Theme.danger.g, Theme.danger.b, control.hovered ? 0.14 : 0.06) : control.kind === "ghost" && !control.hovered ? Theme.transparent : control.hovered || control.down ? Theme.surface2 : Theme.surface
        border.color: control.kind === "primary" && control.enabled || control.kind === "ghost" ? Theme.transparent : Theme.border
        opacity: control.enabled || control.loading ? 1 : 0.6
        Behavior on color {
            ColorAnimation {
                duration: Theme.motion ? 140 : 0
            }
        }
        Rectangle {
            anchors.fill: parent
            anchors.margins: 2
            radius: parent.radius - 2
            color: Theme.transparent
            border.color: Theme.accent
            border.width: 2
            visible: control.visualFocus
        }
    }
}
