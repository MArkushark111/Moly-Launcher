import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ScrollView {
    id: page
    property var shell
    readonly property bool microsoftConfigured: launcher.microsoftAvailable
    clip:true;contentWidth:availableWidth
    ColumnLayout {
        width: page.availableWidth
        spacing: 16
        Item { Layout.preferredHeight: 10 }
        ColumnLayout {
            Layout.leftMargin: 24
            Layout.rightMargin: 24
            spacing: 7
            Text { text: "Accounts"; color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: 26; font.weight: Font.Bold }
            Text { text: "Microsoft accounts and local profiles."; color: Theme.textSecondary; font.pixelSize: 13 }
            MButton{text:"Choose account type";iconName:"accounts";compact:true;onClicked:shell.showAccountChooser()}
        }
        RowLayout {
            Layout.leftMargin: 24
            Layout.rightMargin: 24
            Layout.fillWidth: true
            spacing: 16
            Panel {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.alignment: Qt.AlignTop
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 14
                    RowLayout {
                        spacing: 10
                        Icon { name: "user"; size: 22; color: Theme.textSecondary }
                        Text { text: "Microsoft"; color: Theme.text; font.pixelSize: 17; font.weight: Font.DemiBold }
                    }
                    Text {
                        text: "Sign in in your browser with the Microsoft account that owns Minecraft Java Edition. Moly never asks for your password."
                        color: Theme.textSecondary
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                        font.pixelSize: 13
                        lineHeight: 1.15
                    }
                    MButton {
                        text: "Sign in with Microsoft"
                        iconName:"external"
                        kind: "primary"
                        enabled: !launcher.busy
                        onClicked:launcher.signIn()
                    }
                    Text {
                        visible: !page.microsoftConfigured
                        text: "Microsoft sign-in is not available in this build yet. Moly’s owner must finish application registration. Local accounts are available."
                        color: Theme.textMuted
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                        font.pixelSize: 12
                    }
                    Rectangle {
                        visible: launcher.loginCode.length > 0
                        Layout.fillWidth: true
                        implicitHeight: codeLayout.implicitHeight + 24
                        radius: 14
                        color: Theme.surface2
                        border.color: Theme.border
                        ColumnLayout {
                            id: codeLayout
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 9
                            Text { text: "Enter this code on Microsoft's page"; color: Theme.textSecondary; font.pixelSize: 12; wrapMode: Text.Wrap; Layout.fillWidth: true }
                            Text { text: launcher.loginCode; color: Theme.text; font.family: "Consolas"; font.pixelSize: 26; font.weight: Font.Bold; font.letterSpacing: 2 }
                            Text { text: "Finish signing in in your browser. Moly will continue automatically."; color: Theme.textSecondary; font.pixelSize: 12; wrapMode: Text.Wrap; Layout.fillWidth: true }
                            MButton { text: "Cancel sign-in"; kind: "ghost"; compact: true; onClicked: launcher.cancel() }
                        }
                    }
                }
            }
            Panel {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.alignment: Qt.AlignTop
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 14
                    RowLayout {
                        spacing: 10
                        Icon { name: "box"; size: 22; color: Theme.textSecondary }
                        Text { text: "Offline profile"; color: Theme.text; font.pixelSize: 17; font.weight: Font.DemiBold }
                    }
                    Text { text: "Choose a nickname for local play. Servers that check Microsoft accounts need an online session."; color: Theme.textSecondary; wrapMode: Text.Wrap; Layout.fillWidth: true; font.pixelSize: 13; lineHeight: 1.15 }
                    MField { id: nickname; Layout.fillWidth: true; placeholderText: "Your nickname"; maximumLength: 16; onAccepted: addProfile.clicked() }
                    MButton { id: addProfile; text: "Add offline profile"; enabled: !launcher.busy; onClicked: { launcher.addOfflineAccount(nickname.text); nickname.clear() } }
                }
            }
        }
        Text { text: "Saved profiles"; color: Theme.text; font.pixelSize: 16; font.weight: Font.DemiBold; Layout.leftMargin: 24 }
        Repeater {
            model: launcher.accounts
            delegate: Panel {
                required property var modelData
                Layout.leftMargin: 24
                Layout.rightMargin: 24
                Layout.fillWidth: true
                RowLayout {
                    anchors.fill: parent
                    spacing: 14
                    Rectangle {
                        width: 42; height: 42; radius: 14; color: Theme.surface2
                        Text { anchors.centerIn: parent; text: modelData.name.charAt(0).toUpperCase(); font.pixelSize: 20; font.weight: Font.DemiBold; color: Theme.text }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4
                        Text { text: modelData.name; color: Theme.text; font.pixelSize: 15; font.weight: Font.DemiBold }
                        Text { text: modelData.type === "microsoft" ? "Microsoft · Java Edition" : "Offline · local profile"; color: Theme.textSecondary; font.pixelSize: 12 }
                    }
                    Badge { visible: launcher.activeAccount.id === modelData.id; text: "Active"; tint: Theme.textSecondary }
                    MButton { visible: launcher.activeAccount.id !== modelData.id; text: "Use profile"; onClicked: launcher.selectAccount(modelData.id) }
                    MButton { text: "Remove"; kind: "danger"; enabled: !launcher.busy; onClicked: shell.confirm("account", "Remove account?", "This removes the saved profile and its login token from this computer. Your Minecraft worlds remain.", modelData.id) }
                }
            }
        }
        Text {
            Layout.leftMargin: 24
            Layout.rightMargin: 24
            Layout.fillWidth: true
            Layout.bottomMargin: 24
            text: "Your Microsoft password stays in the browser. Saved login tokens use Windows Credential Manager."
            color: Theme.textMuted
            font.pixelSize: 12
            wrapMode: Text.Wrap
        }
    }

}
