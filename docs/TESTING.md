# Testing Moly

Use an isolated data directory for manual and live-service tests. Do not test imports, updates, or recovery on your only copy of a valued instance.

## Engine tests

From PowerShell in the project directory:

```powershell
.\scripts\build.ps1
```

Or, after configuring and building:

```powershell
ctest --preset windows
```

The engine suite covers offline UUIDs, archive path restrictions, Windows launch rules, Maven paths, argument conditions, log redaction, atomic JSON writes, valid/unsafe archives, content rollback, world preservation, and cancellation before content commit. These tests use temporary folders and do not require account credentials.

## Opt-in live checks

`moly_smoke.exe` makes real network requests and may download substantial game files. Its test instance IDs are based on the loader, so use a dedicated test folder and do not run two commands for the same loader at once.

```powershell
.\build\moly_smoke.exe .\test-data\live catalog
.\build\moly_smoke.exe .\test-data\live install 1.20.1 Vanilla
.\build\moly_smoke.exe .\test-data\live launch 1.20.1 Vanilla
.\build\moly_smoke.exe .\test-data\live install 1.20.1 Fabric
.\build\moly_smoke.exe .\test-data\live content 1.20.1 Fabric
.\build\moly_smoke.exe .\test-data\live launch 1.20.1 Fabric
```

Run from the same shell used by `scripts/build.ps1`, or add the installed Qt and MinGW `bin` folders to `PATH` first.

The current launch helper stops the game after 45 seconds and returns success for that timed stop. **Its exit code alone does not prove a successful launch.** Check that the game reaches its menu and inspect the game log for initialization progress or errors. Do not use this helper to play or edit a real world.

## Interface checks

The application can capture its own window for visual inspection:

```powershell
.\build\MolyLauncher.exe --data-dir .\test-data\ui --page library --screenshot C:\Temp\moly-library.png
```

Supported page names include `library`, `discover`, `instance`, `accounts`, and `settings`. Create an instance in the same isolated folder before checking its instance screen. A screenshot confirms rendering only, not the behavior of its buttons.

Before accepting the refreshed interface, check:

1. Resize to 1024 × 640 and inspect each page for clipped controls and unwanted horizontal scrolling.
2. Use Tab, Shift+Tab, Enter, and Escape through navigation, tabs, and installation dialogs. Verify visible focus.
3. Switch the accent color, restart, and check that the saved choice appears immediately. Enable reduced motion and verify transitions stop.
4. Open **Browse mods** from an instance. Confirm Mods is selected and the correct instance remains targeted.
5. Search both providers. Check initial popular results, loading state, no-result state, and a network/API error.
6. Install a compatible mod with dependencies. Confirm its target, review required files, and verify the app remains open after success or failure.
7. Drag a local JAR onto a selected instance and confirm that import review appears.
8. Toggle a mod, remove it, and restore its content backup. Verify worlds are unchanged.
9. Launch an installed instance offline, inspect live logs, and open/close the detached log window.
10. Close and restart Moly. Confirm instance names, account selection, content state, and appearance persist.

Provider thumbnails may use WebP. For the supplied Qt 6.11.2 MinGW build, `scripts/ensure-webp.ps1` prepares the official matching decoder in `build/imageformats/qwebp.dll`. Verify that those thumbnails render in Discover and that the packaged application carries the decoder too.

## Verification record

Evidence from the initial implementation session:

| Check | Result |
|---|---|
| CMake / MinGW build | Completed successfully |
| Engine test suite | Passed after the UI refresh |
| Launcher behavior tests | Passed: accent persistence, invalid targets, unsafe paths, export privacy, world listing |
| Mojang version catalog | Live response received |
| Modrinth project search | Live results received |
| Vanilla 1.20.1 | Installed and launched |
| Fabric 1.20.1 + Sodium | Installed; launch log reached texture-atlas initialization |
| Quilt 1.20.1 | Installer completed; launch validation outstanding |
| Forge 1.20.1 | Installer completed; initial launch revealed a module exclusion issue, with a fix awaiting retest |
| NeoForge 1.21.1 | Installer completed; launch validation outstanding |
| Microsoft account sign-in / refresh | Requires a real approved application client ID and account; not yet verified end to end |
| CurseForge with supplied API key | API search verified; headless JEI install completed and all file hashes verified |
| Modrinth install flow | Headless Iris + dependency install completed; event-loop heartbeat stayed responsive |
| Refreshed interface | Library, Instance, Discover, and Settings screenshots checked; 1024 × 640 layouts render |
| Manual click-through | Stopped when the user pressed Escape; remaining manual interaction checks are not claimed |
| WebP image decoder | Official matching plugin installed locally; Qt's image reader reports WebP support |
| Portable dependency check | Packaged executable starts successfully with Qt absent from PATH |
| Separate clean Windows machine | Not yet verified |

Update this record only with observed results. A completed install does not establish a successful game launch, and a compiled UI does not establish a working install flow.

## Live install regression (opt-in)

This helper contacts real providers and installs into a new test instance. It does not open or control the desktop app, authenticate an account, or launch Minecraft.

```powershell
.\build-release\install_integration.exe .\test-data\install-check-modrinth Modrinth iris Fabric
.\build-release\install_integration.exe .\test-data\install-check-curseforge CurseForge 238222 Forge
```

The CurseForge run reads the saved key from Credential Manager. The helper checks the captured target, required-file manifest, every installed checksum, completed transaction cleanup, and responsiveness during resolution and installation. Both runs passed on October 26, 2026.

Screenshots of the refreshed layout are in [screenshots/](screenshots/). These use isolated test instances.

## 0.2 verification

- Both automated CTest suites pass, including new embedded-logo, disabled-file metadata, icon, and pagination-state cases.
- Modrinth and CurseForge each loaded 30 then 60 unique projects through the live pagination helper, preserving the first page's order.
- The four-argument QML-style create call automatically installed a new Minecraft 1.20.1 instance and committed real launch files.
- The rendered switch test preserved the same delegate, observed an intermediate thumb position, and restored the original file state.
- The supplied SVG was rendered with Qt and embedded as a native Windows ICO resource.

The switch test is opt-in and requires an isolated data folder containing an installed mod:

```powershell
.\build-release\MolyLauncher.exe --data-dir .\test-data\packaged-render --verify-switch
.\build-release\pagination_integration.exe .\test-data\pagination-modrinth Modrinth
.\build-release\pagination_integration.exe .\test-data\pagination-curseforge CurseForge
```

For headless rendering, use Qt's installed offscreen platform plugin and software Quick renderer. The production package uses the Windows platform plugin.

## 0.2.2 verification

- Both CTest suites pass, including immediate reduced-motion notification and persistence across reloads.
- The rendered `--verify-motion` check clicks the actual Settings switch without pressing Save, verifies the saved value, checks that another switch snaps with reduced motion enabled, then verifies an intermediate animated position after switching it off again. It restores the initial preference.
- Rendered and inspected the selected-instance overview, a single-row list, multiple instances, empty Library, and Settings at the 1024 × 640 minimum size using isolated data.
- Packaged 0.2.2 starts with Qt absent from PATH and passes the rendered motion check using the installed offscreen plugin.

```powershell
.\build-release\MolyLauncher.exe --data-dir .\test-data\ui-overview --verify-motion
```

## 0.2.3 verification

- Both CTest suites pass, including a ZIP with an empty folder entry incorrectly marked as a file and a nested JSON file beneath it.
- Prepared Sodium and the real Fabulously Optimized Modrinth pack in isolated storage and inspected their compact confirmations at 1024 × 640. Pack preparation successfully resolved 108 files after the ZIP-folder fix; these previews do not apply or launch the pack.
- The optional `--dialog install` and `--dialog pack` previews require `--data-dir` and contact Modrinth to prepare a real review. The main install button still invokes the existing transactional install path once.

## 0.3.0 verification

- Six automated suites pass: engine, launcher behaviors, portable packs, skins, saved play targets, and quick-play arguments. Coverage includes GIF copying/reload, preference validation, safe pack roundtrips/cancellation, PNG metadata stripping, classic/slim UV rendering, bounded NBT/gzip parsing, a real local status-protocol mock, and exact launch arguments.
- The controller export/import helper passes without network access: fresh instance selection, identical files, preserved source worlds, artwork/disabled mods, sanitized launch settings, and no stale installed/task state.
- The rendered task-panel check completes two real portable exports, verifies auto-opening and its disabled preference, then manually reopens the panel. The motion check also verifies that changing appearance preserves an unsaved RAM value.
- A live Modrinth Iris install with required dependencies passed using isolated storage. This checks the resolver, downloaded file hashes, transaction cleanup, and event-loop responsiveness; it does not launch Minecraft.
- Rendered and inspected Library, the creation chooser, Wardrobe, Share, and instance content at 1280 × 820 and/or the minimum 1024 × 640 size. UI previews use isolated fixture worlds and a local test server address.
- Microsoft skin upload is implemented but has not been exercised against a signed-in account in this environment. Direct world/server arguments are tested; no live quick-play game session or separate clean Windows machine is claimed by these checks.

```powershell
.\build-release\export_integration.exe .\test-data\export-controller-030
.\build-release\MolyLauncher.exe --data-dir .\test-data\ui-030 --verify-task-ui
.\build-release\MolyLauncher.exe --data-dir .\test-data\ui-030 --verify-motion
```

## 0.3.1 verification

- The six suites cover the nine default character choices/model persistence, client override precedence, ignored server-only payloads, and cancellation cleanup of private pack staging, alongside previous archive/path/checksum cases.
- Benchmarked the same cached Cobblemon Official Modpack [Fabric] 1.8.4 archive without modifying it or installing it: 2,513 planned files; preparation changed from 16,451 ms to 5,884 ms. All 2,436 staged override paths and SHA-256 hashes matched. These times exclude downloading the archive, game files, and Java.
- An isolated existing-modpack fixture acquired its real Modrinth project icon asynchronously and persisted icon.png/iconOrigin without blocking a foreground task.
- Inspected actual default textures, the account-name label, the landscape Worlds glyph, and the chosen background inside the rounded instance tile. The main instance page has no background image.
- Texture provenance is recorded under assets/default-skins; the source client matched Mojang's published SHA-1. No live account skin upload is claimed.

## 0.4.0 verification

- All six CTest suites pass, including new validation/persistence cases for per-instance resolution and launcher visibility and an invalid-target preview that cannot start work.
- The opt-in `confirm_integration` helper uses a fresh temporary directory and real Modrinth metadata. Pack and mod version selection creates no jobs, archive downloads, staging, or game payloads. One explicit confirmation resolves dependencies, commits content, and checks installed hashes.
- `--verify-confirm-ui` changes the actual version dropdown and clicks the visible confirmation button in isolated storage. Selection stays idle; one click installs without a second modal.
- Updated `--verify-task-ui` completes two real exports, observes compact activity, verifies the full panel stays closed, and opens history manually. Actual reduced-motion and mod-switch checks pass.
- Rendered and inspected 1024 × 640 Library, settings tabs, per-instance overrides, creation chooser, rounded instance menu, provider dropdown with both logos, and compact pack confirmation. Source previews use isolated fixture data.
- The portable package is deployed with Qt and tested with a minimal system PATH. No new live Microsoft login/skin upload, game launch, or separate clean-machine test is claimed by this UI release.

```powershell
.\build-release\confirm_integration.exe
.\build-release\MolyLauncher.exe --data-dir .\test-data\confirm-ui-040 --verify-confirm-ui
.\build-release\MolyLauncher.exe --data-dir .\test-data\ui-040 --verify-task-ui
```

The confirm UI check needs an empty compatible Fabric 1.20.1 instance; do not run live install checks against everyday instances.

## 0.5.0 verification

- Seven automated suites pass. Local-import cases cover Prism/MultiMC, CurseForge, ATLauncher, the current Modrinth SQLite schema, exact file copying, disabled mods, optional worlds, cancellation cleanup, self-import rejection, and leaving source game files/database hashes unchanged. Launcher-order persistence is tested separately.
- The actual drag-handle UI check moves an instance, verifies saved custom order, and restores the fixture order. Motion and installed-mod switch checks pass after the startup-page correction. The task UI check confirms manual-only opening.
- A real Modrinth search/preparation displayed Fabulously Optimized with its real project icon and dimmed pending cards in the rail and Library. The captured footer pill is anchored right and expands left. Cancelling the check did not create a ready instance.
- Read-only local discovery found the Modrinth profiles installed on this PC. No actual user instance was copied or changed during verification. Format import tests use temporary fixture data; this is not a claim that every historical launcher schema or proprietary client feature is supported.
- A second process forwarded a local .mlypack opening request to the first Moly window. The existing window displayed an import confirmation; no instance was copied before confirming.
- The Windows association is registered for this user against the packaged 0.5.0 executable; its Explorer ProgID, icon path, and quoted opening command were checked. The installer recipe has matching association entries; an Inno Setup installer was not compiled here.
- Inspected the importer and populated modpack chooser at 1024 x 640. The portable build is checked with a minimal system PATH. No new live Microsoft account sign-in, full Minecraft game session, or separate clean-machine test is claimed.

```powershell
.\build-release\MolyLauncher.exe --data-dir .\test-data\ui-050 --verify-reorder-ui
.\build-release\MolyLauncher.exe --data-dir .\test-data\pending-pack-050 --verify-pack-ui .\docs\screenshots\installing-050.png
```

The pack-preparation check requires empty isolated storage, contacts Modrinth, and cancels its real preparation after capturing the UI. Never run these helpers against ordinary user storage.

## 0.6.0 verification

- Nine CTest suites pass. New artwork tests paint actual PNG data URIs into rounded images, check transparent corners and reject malformed/oversized images. An encoding regression reads every QML source as strict UTF-8. Startup tests count completed stages and never mark the interface ready early.
- A real local Java-status mock supplies an icon, MOTD, player counts and pong; tests verify normalized data, disk icon caching, and reloading the cached picture. Saved servers.dat raw-base64 icons are covered too.
- Fast Client profile-index and current NoRisk SQLite fixtures verify correct game folders, loaders and exclusion of account fields. Additional client choices retain explicit version review for unidentified folders.
- The rendered startup check completed all four local stages and the workspace fade, about 430 ms in the recorded warm run on this PC. That time is not a performance promise for other systems. It has no catalog-finished dependency or minimum splash duration.
- Rendered and inspected the Play loading state, small-window import chooser, and live Donut SMP card. The live server returned its real icon, measured latency and player counts; server availability and counts naturally vary.
- Read-only discovery found this PC's existing Fast Client profile. No real client installation was imported or changed. The Play loading screenshot is a UI-state preview, not a new full-game launch test.
- The packaged runtime and SQL/image plugins are checked with a minimal system PATH. No new Microsoft login, full game session or separate clean-machine test is claimed.

```powershell
.\build-release\MolyLauncher.exe --data-dir .\test-data\ui-060 --verify-startup --capture-startup .\docs\screenshots\startup-060.png
.\build-release\MolyLauncher.exe --data-dir .\test-data\ui-060 --dialog import --import-source "Fast Client" --screenshot .\docs\screenshots\fast-import-060.png
```
