# Microsoft sign-in in Moly

Microsoft sign-in has two separate pieces: **the launcher is registered once**, then **players sign in through Microsoft's website**. Players should not normally need to create a Microsoft developer app.

This preview has no Moly-owned application ID included. The Accounts screen and first-run chooser offer Microsoft or local accounts. Application registration is an owner/build task; players are never asked for an ID. An ID is a public identifier for the launcher, not your password or a client secret.

## If you just want to play

Use an offline profile for installed local games while the launcher registration is being prepared. Microsoft-authenticated servers need a successful online session.

When Moly has its own configured registration, select **Accounts → Sign in with Microsoft**. Your browser opens Microsoft's sign-in page, and Moly displays a short code. Enter that code on Microsoft's page and finish signing in there. Moly checks the account's Java Edition entitlement and loads the Minecraft profile. Microsoft's [device-code documentation](https://learn.microsoft.com/en-us/entra/identity-platform/v2-oauth2-device-code) describes this browser flow.

## One-time setup for the launcher owner

### 1. Register Moly

Open the [Microsoft Entra admin center](https://entra.microsoft.com/#view/Microsoft_AAD_RegisteredApps/ApplicationsListBlade), then **App registrations → New registration**. Give the app a recognizable name, select **Personal Microsoft accounts only**, and register it. Copy the **Application (client) ID** from its Overview page.

Microsoft's [registration guide](https://learn.microsoft.com/en-us/entra/identity-platform/quickstart-register-app) lists the current directory, account, and permission prerequisites. If your account cannot register apps, that prerequisite needs to be resolved by the launcher owner; Moly cannot create a valid ID by itself.

### 2. Enable public-client device sign-in

In the app registration, open **Authentication**. Under **Advanced settings**, set **Allow public client flows** to **Yes**, then save. Moly uses the `consumers` device-code endpoint for personal Microsoft accounts. A redirect URI is not required for that device flow, and this desktop app does not use a client secret. See Microsoft's [public-client configuration guide](https://learn.microsoft.com/en-us/entra/identity-platform/scenario-desktop-app-configuration#enable-public-client-flow).

The existing launcher requests `XboxLive.signin` and `offline_access`, then exchanges the Microsoft token through Xbox Live/XSTS and Minecraft Services. Those are the current code's sign-in scopes; the setup screen does not add unrelated Graph permissions or change the authentication implementation.

### 3. Check Minecraft API access

An Entra app registration and Minecraft Services access are separate. Use the [official Minecraft AppID review link](https://aka.ms/mce-reviewappid) for the launcher's ID and follow the current requirements shown there. Review and any approval are handled by Microsoft/Mojang, so entering an arbitrary valid-looking ID in Moly does not guarantee Minecraft access.

The legacy [Minecraft API support article](https://help.minecraft.net/hc/en-us/articles/16254801392141) currently returns only the support-site shell to automated readers. The short review link redirects to Microsoft's form. Do not interpret a saved ID as proof that approval has been completed.

### 4. Add the ID to Moly

Supply Moly's registered public ID when configuring the launcher build:

```powershell
cmake -S . -B build-release -DMOLY_MICROSOFT_CLIENT_ID=YOUR_PUBLIC_APPLICATION_ID
cmake --build build-release --target MolyLauncher
```

For development, `MOLY_MICROSOFT_CLIENT_ID` in the process environment overrides the included ID. Existing configured development settings remain supported. A distributed build should include Moly's approved ID so players only select **Sign in with Microsoft**.

The ID is saved in the existing settings store. Login tokens, when acquired successfully, stay in Windows Credential Manager. Moly never requests the Microsoft password in its own interface.

## If sign-in fails

- **No client ID:** finish the launcher setup above. This is not an account/password error.
- **Application rejected / public-client flow:** check account type and public-client settings for the same registered ID.
- **Minecraft rejects the application:** review the launcher's Minecraft API access status; another person's or launcher's ID is not a substitute.
- **No Java Edition entitlement:** use the Microsoft account that owns or currently has access to Minecraft Java Edition.
- **Expired browser code:** cancel and start sign-in again.

The launcher checks the configured ID’s shape; it cannot validate Minecraft approval in advance. End-to-end Microsoft sign-in still needs a real authorized registration and a real account to verify.
