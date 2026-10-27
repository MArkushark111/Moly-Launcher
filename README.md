# Moly Launcher

A Windows Minecraft Java Edition launcher built with C++20 and Qt Quick. Moly keeps each Minecraft setup in its own **instance**, so its worlds, mods, resource packs, and shaders stay together.

This is a **0.6.0 preview**. The launcher has working installation and local launch paths; Microsoft sign-in and CurseForge require your own service credentials.

## Run the portable build

Extract the entire `MolyLauncher-0.6.0-Windows-x64.zip` archive, then open **MolyLauncher.exe**. Keep the accompanying DLLs and folders beside it. The executable in `dist/MolyLauncher-0.6.0/` is also ready to run without a Qt installation.

## First use

1. Open Moly and go to **Accounts**. Add a local nickname, or configure Microsoft sign-in in **Settings → Connections**.
2. Select **Create instance**, then choose a Minecraft version and loader. Choose Fabric, Forge, NeoForge, or Quilt if you want mods.
3. Creating an instance starts installation automatically. Watch the current step and measured progress on the instance page.
4. Use the bottom dock to navigate, or open the instance's content browser to find compatible mods, resource packs, or shaders. Choose the instance and release, then confirm once. Dependencies and pack files are prepared only after confirmation.
5. Select **Play**. The instance's **Logs** view shows game output and can open in a separate window.

Offline profiles use installed game files. Servers that require Microsoft authentication need a valid online account session. Resource packs and shader packs must be enabled in Minecraft; shaders also need a compatible shader mod.

## Personalize and share

- Set an instance image or GIF and a tile background in **Instance settings**. Tile backgrounds appear in the left rail and grid, while the main content area stays neutral. Animated artwork pauses when Reduce motion is enabled.
- **Library → Jump back in** and **Worlds & servers** show saved worlds and servers from your instances. Server checks use the Java status protocol; Settings controls checks and signal color. Direct entry requires Minecraft quick-play support, normally Java 1.20 or later.
- **Wardrobe** includes the nine Minecraft default characters and accepts uploaded PNG skins, with a textured 3D player you can rotate. The name tag uses only the active account name. **Apply to account** requires a configured, signed-in Microsoft account. Vanilla offline profiles do not automatically use these local skins.
- **Share** exports `.mrpack`, Moly portable ZIP, or `.mlypack`. Portable exports can include worlds; Minecraft/Java downloads and account secrets are excluded. See [portable formats](docs/PORTABLE_PACKS.md).
- The **Tasks** button beside the bottom dock opens active progress and recent tasks. It can open automatically for installs; this is configurable in Settings.

## Build and run on this computer

The existing CMake preset matches the installed Qt 6.11.2 MinGW kit. It uses these tools:

| Tool | Installed location |
|---|---|
| Qt | `C:/Qt/6.11.2/mingw_64` |
| Matching C++ compiler | `C:/Qt/Tools/mingw1310_64/bin/g++.exe` |
| Ninja | `C:/Qt/Tools/Ninja/ninja.exe` |
| CMake | Available on `PATH` |

From PowerShell in the project folder:

```powershell
.\scripts\build.ps1
.\build\MolyLauncher.exe
```

The script configures the project, builds it, and runs the engine tests. To keep a development run separate from your normal launcher data:

```powershell
.\build\MolyLauncher.exe --data-dir .\test-data\manual
```

In **CLion**, open this folder and select the **Windows · installed Qt 6.11.2 / MinGW** CMake preset. Use the matching MinGW toolchain, then build and run the `MolyLauncher` target. If a run reports missing Qt DLLs, add `C:/Qt/6.11.2/mingw_64/bin` and `C:/Qt/Tools/mingw1310_64/bin` to that run configuration's `PATH`.

The MSVC preset is an alternative for a machine with Visual Studio Build Tools and a matching Qt MSVC kit. Set `QT_MSVC_PATH` to that kit. Do not mix Qt's MinGW libraries with MSVC.

## Accounts and connections

- **Modrinth:** public browsing and downloads do not need a key.
- **CurseForge:** save your API key in **Settings → Connections**, or set `MOLY_CURSEFORGE_API_KEY` in your process environment. Some authors require browser downloads; Moly asks you to select the exact downloaded file and verifies its hash.
- **Microsoft:** supply your application's public client ID in Settings or `MOLY_MICROSOFT_CLIENT_ID`. The application must support personal Microsoft accounts and public-client device sign-in. Minecraft service access may require approval. Moly opens the browser and never requests your Microsoft password.

Saved CurseForge keys and Microsoft refresh tokens use **Windows Credential Manager**. They are not stored in the source tree or ordinary settings JSON. A desktop Microsoft application does not need a client secret in the launcher.

## Where things live

Ordinary launcher data uses Windows' local application-data location by default. **Settings** shows the exact folder, and `--data-dir` selects a different one for a run.

| Folder or file | Purpose |
|---|---|
| `qml/Main.qml` | Window, sidebar, top bar, navigation, and shared dialogs |
| `qml/LibraryPage.qml` | Instance library |
| `qml/DiscoverPage.qml` | Provider search and project browsing |
| `qml/InstancePage.qml` | Instance content, controls, settings, and logs |
| `qml/SettingsPage.qml` | Launcher settings and appearance controls |
| `qml/Theme.qml` | Shared interface colors and typography |
| `assets/` | Local branding and font assets |
| `src/launcher.*` | Connects the interface to background tasks and stored data |
| `src/minecraft.*` | Game installation, Java selection, and launch arguments |
| `src/content.*` | Provider files, dependencies, packs, backups, and recovery |
| `src/accounts.*` | Account sign-in and secure secret storage |
| `tests/` | Engine tests and opt-in live service checks |

Inside the data folder, `instances/<id>/game` holds each instance's game files. `cache/` shares reusable downloads, `runtimes/` holds downloaded Java, and each instance keeps its own content backups. Content updates preserve worlds. Use **Back up worlds** for a separate copy of saves before changing a heavily modded setup.

## Packaging

```powershell
.\scripts\build.ps1 -Package
```

This prepares the app with its Qt runtime under `dist/MolyLauncher-0.6.0` and creates a ZIP. With an installed Inno Setup compiler, pass its `ISCC.exe` path using `-InnoCompiler` to build the installer described by `packaging/moly.iss`.

The packaging recipe does not provide a code-signing certificate. A clean-machine installation test and a signed release are still release tasks. See [THIRD_PARTY.md](THIRD_PARTY.md) for runtime notices and [docs/TESTING.md](docs/TESTING.md) for verification details.

## Preview limits

- Windows x64 comes first. Linux and macOS are not release targets yet.
- The current launch implementation targets Minecraft 1.13 and newer; historical versions are not guaranteed.
- Version and loader filters reduce mismatches but cannot guarantee that a combination of mods will work.
- A pack update that changes Minecraft or the loader must be installed as a new instance.
- Microsoft sign-in, restricted CurseForge downloads, and arbitrary imported modpacks still need testing with the relevant real accounts and projects.
- Cloud sync, Bedrock, hosting, and launcher self-updates are outside this preview.

## Moly 0.2 interface

Main navigation is in a rounded bottom dock. The left rail contains centered instance tiles. A compact task button slides open in the footer during work. Open it yourself to see full progress, cancellation, retry, and history. Launching a game does not open the task panel.

Click an instance image to choose a local logo. Mod switches slide without resetting the content list. Discover loads the next provider page near the bottom and keeps earlier cards in place. Installation percentages show measured bytes/files for the current step; work with no known total says Preparing.

Microsoft sign-in setup is explained in [docs/MICROSOFT_SIGN_IN.md](docs/MICROSOFT_SIGN_IN.md) and in Accounts. The launcher needs its own registered and authorized public app ID before ordinary browser sign-in can work.

## 0.4 controls and settings

Settings is grouped into Appearance, Game, Connections, and Advanced tabs. Accent color also styles sliders, dropdowns, and focus indicators. Customize the instance sidebar, Library mod counts, GIF playback, server signals, and task history. Each instance can override memory, window size/fullscreen, Java arguments, and whether Moly stays visible during play. The Library keeps saved worlds/servers above the instance list without a duplicate selected-instance banner.

CurseForge secrets stay in Windows Credential Manager and are never bundled in this repository or the portable build. This computer keeps its existing connection; another computer needs its own configuration (the advanced `MOLY_CURSEFORGE_API_KEY` environment variable is also supported). Microsoft sign-in still requires a registered public-client application ID and API access.

## Bring local instances to Moly

Choose **Import > Import instance from this computer** or **Create instance > Bring a setup > Import instance**. Select a launcher to find its usual local data folders. Modrinth and CurseForge here mean setups already on your PC. Choose a different folder for portable installations or moved libraries. Check the displayed Minecraft/loader versions, close Minecraft for that setup, and choose whether to include worlds.

Modrinth's current SQLite database and older folder profiles, Prism/MultiMC, CurseForge and ATLauncher metadata are recognized. Other game folders can supply ordinary Minecraft files after you enter their exact game and loader versions. Lunar's own bundled client features/cosmetics and arbitrary launcher components cannot run in Moly. The importer copies known game-data folders; it does not copy launcher login databases, Java executables, or launcher commands. Imported files are local content; original launcher update links are not carried over.

Drag the six-dot handles to arrange instances. Dropping above/below an instance places it before/after; **My order** keeps that layout across restarts. Focus a handle and use **Alt+Up/Down** for keyboard ordering.

The Windows installer associates **.mlypack** with Moly. For the portable build, use **Settings > Advanced > Use Moly for .mlypack**. Explorer then shows Moly's icon and double-clicking opens a confirmation. If Moly is already open, the request goes to its existing window.

## Startup and saved servers

The compact startup screen counts four completed local stages: library, player assets, interface creation, and the first rendered frame. It has no minimum display time and does not wait for online catalog refreshes. Reduce motion removes the workspace fade.

Saved server cards show their validated icon, first MOTD line, player count and measured connection time. Use the circular arrow to check one server or the copy icon to copy its address. Icons remain available from Moly's cache after restarting; a server without a usable picture gets the server-rack symbol.

The local source list also includes Dawn Client, Fast Client, NoRisk Client, Badlion Client, LabyMod, Feather, and GDLauncher. Fast Client's local profile index and NoRisk's current profile database are recognized. Other layouts can use ordinary game folders with explicit version selection. Client accounts, cosmetics, bundled features and custom launch components are not migrated.
