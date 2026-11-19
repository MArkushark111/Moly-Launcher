import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

LiquidDialog {
    id: dialog
    objectName: "launcherUpdatesDialog"
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(650, parent.width - 48)
    height: Math.min(540, parent.height - 48)
    modal: true
    title: releaseUpdates.available ? "Moly " + releaseUpdates.release.version + " is available" : "Launcher updates"
    contentItem: ColumnLayout {
        spacing: 14
        Text { text: "Installed version: " + releaseUpdates.currentVersion; color: Theme.textSecondary; font.pixelSize: 12 }
        Text { text: releaseUpdates.status; color: Theme.text; wrapMode: Text.Wrap; Layout.fillWidth: true }
        ScrollView {
            Layout.fillWidth: true; Layout.fillHeight: true; clip: true
            TextArea {
                readOnly: true; selectByMouse: true; wrapMode: TextEdit.Wrap
                textFormat: TextEdit.PlainText
                text: releaseUpdates.release.notes || (releaseUpdates.release.version ? "No release description was provided." : "Release notes will appear here after a successful check.")
                color: Theme.textSecondary; font.family: Theme.fontFamily; font.pixelSize: 13
                background: Rectangle { color: Theme.surface2; radius: 12 }
            }
        }
        Text {
            Layout.fillWidth: true
            text: releaseUpdates.release.installerUrl ? "Update opens the official Windows installer download in your browser. Close Minecraft and Moly before running the installer." : "This release has no Windows installer attached. View its GitHub release for available downloads."
            visible: releaseUpdates.available
            color: Theme.textMuted; font.pixelSize: 12; wrapMode: Text.Wrap
        }
        RowLayout {
            Layout.fillWidth: true
            MButton { text: "Check again"; loading: releaseUpdates.checking; enabled: !releaseUpdates.checking; onClicked: releaseUpdates.check() }
            Item { Layout.fillWidth: true }
            MButton { text: "GitHub release"; onClicked: releaseUpdates.openRelease() }
            MButton { visible: releaseUpdates.available && !!releaseUpdates.release.installerUrl; text: "Update"; kind: "primary"; enabled: !releaseUpdates.checking; onClicked: releaseUpdates.openUpdate() }
        }
    }
}
