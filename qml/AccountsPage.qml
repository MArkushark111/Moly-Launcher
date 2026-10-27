import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ScrollView {
    id: page
    property var shell
    readonly property bool microsoftConfigured: /^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$/.test(launcher.settings.microsoftClientId || "")
    clip: true
    contentWidth: availableWidth

    function openMicrosoftSetup() {
        setupDialog.step = 0
        setupClientId.text = launcher.settings.microsoftClientId || ""
        setupDialog.open()
    }
    function saveMicrosoftClient(startSignIn) {
        const value = setupClientId.text.trim().toLowerCase()
        if (!setupClientId.acceptableInput || launcher.busy)
            return
        launcher.saveSettings({ microsoftClientId: value })
        if ((launcher.settings.microsoftClientId || "").toLowerCase() !== value)
            return
        setupDialog.close()
        if (startSignIn)
            launcher.signIn()
    }

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
                        text: page.microsoftConfigured
                              ? "Sign in in your browser, then enter the code shown here. Use the Microsoft account that owns Minecraft Java Edition."
                              : "Microsoft sign-in needs Moly's own app registration. That is launcher setup, not something every player should have to do. This development build still needs that setup."
                        color: Theme.textSecondary
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                        font.pixelSize: 13
                        lineHeight: 1.15
                    }
                    MButton {
                        text: page.microsoftConfigured ? "Sign in with Microsoft" : "Set up Microsoft sign-in"
                        iconName: page.microsoftConfigured ? "external" : "settings"
                        kind: "primary"
                        enabled: !launcher.busy
                        onClicked: page.microsoftConfigured ? launcher.signIn() : page.openMicrosoftSetup()
                    }
                    MButton {
                        visible: page.microsoftConfigured
                        text: "Sign-in setup"
                        kind: "ghost"
                        compact: true
                        enabled: !launcher.busy
                        onClicked: page.openMicrosoftSetup()
                    }
                    Text {
                        visible: !page.microsoftConfigured
                        text: "The guide explains the one-time setup. You can use an offline profile in the meantime."
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

    LiquidDialog {
        id: setupDialog
        property int step: 0
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(600, parent.width - 48)
        height: Math.min(560, parent.height - 48)
        modal: true
        title: "Microsoft sign-in setup"
        padding: 24
        closePolicy: Popup.CloseOnEscape
        contentItem: ColumnLayout {
            spacing: 16
            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                Repeater {
                    model: 4
                    delegate: Rectangle {
                        required property int index
                        Layout.fillWidth: true
                        height: 4
                        radius: 2
                        color: index <= setupDialog.step ? Theme.accent : Theme.border
                        Behavior on color { ColorAnimation { duration: Theme.motion ? 160 : 0 } }
                    }
                }
            }
            Text {
                text: ["1. Register the launcher", "2. Enable device sign-in", "3. Check Minecraft access", "4. Add the public app ID"][setupDialog.step]
                color: Theme.text
                font.family: Theme.fontFamily
                font.pixelSize: 20
                font.weight: Font.DemiBold
                Layout.fillWidth: true
                wrapMode: Text.Wrap
            }
            Text {
                text: [
                    "The launcher owner registers Moly once in Microsoft Entra. Choose Personal Microsoft accounts only and copy the Application (client) ID from the Overview page. A finished Moly release should include its own ID so players can simply sign in.",
                    "Open the app's Authentication page. Under Advanced settings, turn Allow public client flows on and save. Moly uses a short browser code; this device flow does not need a redirect URI or a client secret.",
                    "Minecraft Services access is separate from creating the Microsoft app. Check the official Minecraft API review page for your client ID. The service may reject an app that has not been approved; entering an ID here does not grant approval.",
                    "Paste Moly's Application (client) ID below. It is a public identifier, not your password, an API key, or a client secret. Save it, then start browser sign-in when you are ready."
                ][setupDialog.step]
                color: Theme.textSecondary
                font.pixelSize: 13
                wrapMode: Text.Wrap
                lineHeight: 1.2
                Layout.fillWidth: true
            }
            MButton {
                visible: setupDialog.step < 3
                text: ["Open app registration", "Open Microsoft's setup guide", "Open Minecraft API review"][Math.min(setupDialog.step, 2)]
                iconName: "external"
                enabled: !launcher.busy
                onClicked: launcher.openLink([
                    "https://entra.microsoft.com/#view/Microsoft_AAD_RegisteredApps/ApplicationsListBlade",
                    "https://learn.microsoft.com/en-us/entra/identity-platform/scenario-desktop-app-configuration#enable-public-client-flow",
                    "https://aka.ms/mce-reviewappid"
                ][setupDialog.step])
            }
            MField {
                id: setupClientId
                visible: setupDialog.step === 3
                Layout.fillWidth: true
                placeholderText: "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
                maximumLength: 36
                validator: RegularExpressionValidator { regularExpression: /[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}/ }
                onAccepted: if (acceptableInput) page.saveMicrosoftClient(false)
            }
            Text {
                text: setupDialog.step === 0 ? "Microsoft may require a directory and permission to register apps. If your account cannot create one, this step needs the launcher owner's Microsoft setup."
                      : setupDialog.step === 2 ? "This is a launcher-developer requirement. There is no ID Moly can safely invent or borrow to skip it."
                      : setupDialog.step === 3 ? "An approved registration is still required for successful Minecraft sign-in."
                      : "Do not add a client secret to this desktop launcher."
                color: Theme.textMuted
                font.pixelSize: 12
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
            Item { Layout.fillHeight: true }
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                MButton { text: "Close"; kind: "ghost"; onClicked: setupDialog.close() }
                Item { Layout.fillWidth: true }
                MButton { visible: setupDialog.step > 0; text: "Back"; onClicked: setupDialog.step-- }
                MButton { visible: setupDialog.step < 3; text: "Next"; kind: "primary"; onClicked: setupDialog.step++ }
                MButton { visible: setupDialog.step === 3; text: "Save ID"; enabled: setupClientId.acceptableInput && !launcher.busy; onClicked: page.saveMicrosoftClient(false) }
                MButton { visible: setupDialog.step === 3; text: "Save & sign in"; kind: "primary"; enabled: setupClientId.acceptableInput && !launcher.busy; onClicked: page.saveMicrosoftClient(true) }
            }
        }
    }
}
