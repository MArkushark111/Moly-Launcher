import QtQuick
import QtQuick.Controls

Item {
    id: strip
    property var tabs: []
    property int currentIndex: 0
    property real targetX: 6
    property real targetWidth: 0
    property bool indicatorReady: false
    signal activated(int index)

    implicitHeight: 54
    implicitWidth: tabRow.implicitWidth + 12

    function updateIndicator() {
        const item = entries.itemAt(currentIndex);
        targetX = item ? item.x + tabRow.x : 6;
        targetWidth = item ? item.width : 0;
        if (!indicatorReady)
            Qt.callLater(function () {
                strip.indicatorReady = true;
            });
    }
    onCurrentIndexChanged: Qt.callLater(updateIndicator)
    onTabsChanged: Qt.callLater(updateIndicator)
    Component.onCompleted: Qt.callLater(updateIndicator)

    Rectangle {
        width: tabRow.width + 12
        height: parent.height
        radius: height / 2
        color: Theme.surface
        border.color: Theme.border
    }
    Rectangle {
        id: indicator
        y: 5
        height: strip.height - 10
        radius: height / 2
        visible: strip.targetWidth > 0
        color: Theme.accentSoft
        border.width: 0
        // The leading edge travels a little faster than the trailing width. That
        // slight stretch gives the selection a soft liquid motion without blur.
        x: strip.targetX
        width: strip.targetWidth
        Behavior on x {
            enabled: strip.indicatorReady
            NumberAnimation {
                duration: Theme.motion ? 300 : 0
                easing.type: Easing.OutCubic
            }
        }
        Behavior on width {
            enabled: strip.indicatorReady
            NumberAnimation {
                duration: Theme.motion ? 240 : 0
                easing.type: Easing.OutQuint
            }
        }
        Behavior on color {
            ColorAnimation {
                duration: Theme.motion ? 180 : 0
            }
        }
    }
    Row {
        id: tabRow
        x: 6
        y: 5
        height: strip.height - 10
        spacing: 2
        Repeater {
            id: entries
            model: strip.tabs
            onItemAdded: Qt.callLater(strip.updateIndicator)
            delegate: Button {
                id: tabButton
                required property var modelData
                required property int index
                readonly property bool active: strip.currentIndex === index
                readonly property string tabLabel: typeof modelData === "string" ? modelData : modelData.label
                readonly property string tabIcon: typeof modelData === "string" ? "" : modelData.icon || ""

                height: tabRow.height
                width: contents.implicitWidth + 28
                padding: 0
                hoverEnabled: true
                Accessible.name: tabLabel
                onClicked: strip.activated(index)
                contentItem: Item {
                    Row {
                        id: contents
                        anchors.centerIn: parent
                        spacing: 8
                        Icon {
                            visible: !!tabButton.tabIcon
                            name: tabButton.tabIcon
                            size: 17
                            color: tabButton.active ? Theme.accent : Theme.textMuted
                            anchors.verticalCenter: parent.verticalCenter
                            scale: tabButton.active ? 1.08 : 1
                            Behavior on scale {
                                NumberAnimation {
                                    duration: Theme.motion ? 220 : 0
                                    easing.type: Easing.OutBack
                                }
                            }
                            Behavior on color {
                                ColorAnimation {
                                    duration: Theme.motion ? 160 : 0
                                }
                            }
                        }
                        Text {
                            text: tabButton.tabLabel
                            color: tabButton.active ? Theme.text : Theme.textSecondary
                            font.family: Theme.fontFamily
                            font.pixelSize: 13
                            font.weight: tabButton.active ? Font.DemiBold : Font.Medium
                            anchors.verticalCenter: parent.verticalCenter
                            Behavior on color {
                                ColorAnimation {
                                    duration: Theme.motion ? 160 : 0
                                }
                            }
                        }
                    }
                }
                background: Rectangle {
                    radius: height / 2
                    color: tabButton.hovered && !tabButton.active ? Theme.surface2 : Theme.transparent
                    Behavior on color {
                        ColorAnimation {
                            duration: Theme.motion ? 140 : 0
                        }
                    }
                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: -2
                        radius: parent.radius + 2
                        color: Theme.transparent
                        border.color: Theme.textSecondary
                        border.width: 1
                        visible: tabButton.visualFocus
                    }
                }
            }
        }
    }
}
