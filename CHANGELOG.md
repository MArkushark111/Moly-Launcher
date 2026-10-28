# Changelog

## 0.8.1 - Sign in and keep moving

- Give Microsoft sign-in its own background worker and cancellation state instead of disabling the launcher through the global task slot.
- Show a non-modal top-right sign-in card with selectable/copyable browser code, reopen-browser and Cancel actions. Connected sessions show a green check then fade; errors remain actionable.
- Keep the account-page code and cancel actions available and clear codes on cancellation or completion.

## 0.8.0 - Welcome home

- Add a four-step animated welcome experience with instance guidance, live accent/motion preferences, and local/Microsoft account choices. Replay it from Advanced settings.
- Repair empty build-cache and stale saved Microsoft IDs using the bundled Moly registration; add the four-color Microsoft mark to sign-in actions.
- Remove the CurseForge settings section and register Moly pack files automatically when the normal launcher executable changes. Isolated diagnostic runs do not change Windows associations.

## 0.7.0 - Make it yours

- Stop treating shared Minecraft and Feather folders as installed Badlion, LabyMod or Dawn instances.
- Add editable Moly server shortcuts, persistent drag order, removable/restorable quick access, right-aligned connection info, and optional server descriptions/addresses.
- Add Discord desktop Rich Presence, server-detail controls, and numeric/local-address privacy.
- Separate Discover confirmation from the instance’s one-click content browser window; mark known installed projects. Identify untracked Modrinth content by exact file hashes.
- Add compatible release/beta/alpha version selection with Installed, Upgrade and Downgrade states, transactional replacements and backups.
- Extract local resource/shader pack icons and enrich identified content artwork. Round card artwork, add a subtle fallback cover, and adjust task text alignment.
- Color warning/error log lines and escape log markup.
- Add first-run Local / Microsoft account chooser and remove client-ID fields from player screens. Microsoft still requires owner-supplied registration.

## 0.6.0 - A cleaner start

- Add Dawn Client, Fast Client, NoRisk Client, Badlion Client, LabyMod, Feather and GDLauncher import choices with identifying logos and a searchable source rail.
- Read Fast Client profile indexes and current NoRisk profile databases without importing account fields; other client game folders keep explicit version review and client-feature notices.
- Fix malformed UTF-8 in instance/install dialogs and keep loading button icons/spinners in one centered group.
- Decode validated server PNG data in rounded artwork, support raw servers.dat base64, keep server icons across refresh/restarts, and show a real fallback when an image is unavailable.
- Add per-server checks, address copying, and a compact MOTD preview alongside measured latency/player counts.
- Show a compact startup window with four real local initialization stages, no minimum delay or catalog dependency, then fade in the workspace.

## 0.5.0 - Bring your setups along

- Move Tasks to the bottom right; expand it left during work, retain the native arc spinner, and remove its tooltip.
- Add local instance discovery/import for Modrinth databases, Prism/MultiMC, CurseForge, ATLauncher, and ordinary Minecraft/Lunar game folders. Show version corrections and client-only feature limits before copying.
- Copy game data into separate instances with optional worlds, cancellation cleanup, and no launcher accounts, tokens, or external commands.
- Show pending pack/import instances with their title, icon, dimmed artwork, and installing indicator before files are ready.
- Drag instance handles in the rail, list, or grid to save a custom order; support Alt+Up/Down on focused handles.
- Add useful modpack search starting points, more appearance/startup/log/notification settings, stable world-icon hover, and clipped rounded world/server pictures.
- Register .mlypack Explorer icons and opening commands through the installer or Settings. Forward pack-open requests to an existing Moly window and require confirmation.

## 0.4.0 — Confirm, then play

- Load version metadata without preparing downloads; start dependency resolution and installation after one confirmation. Pause for restricted downloads and locally modified replacements.
- Keep the task center manual. Expand a compact footer activity button horizontally with a native rotating arc, current-step progress, and measured transfer speed.
- Show launch/install loading states on Play buttons.
- Add tabbed global and per-instance settings, window/fullscreen and launcher-visibility overrides, sidebar/mod-count preferences, and a remembered Library view.
- Replace black sliders, square dropdowns, and plain menus with accent-colored controls and rounded popups; show provider icons in source selectors.
- Slide creation choices/settings sections, keep focus outlines inside buttons and keyboard-only, and center notification content.
- Remove the large Library selection banner and its duplicate Play action. Remove the CurseForge key editor while retaining secure existing credentials.

## 0.3.1 — Tile artwork, pack icons, and classic skins

- Put instance backgrounds inside the rounded left-rail/grid tiles and remove the page-wide background.
- Apply project icons automatically to new and existing modpack instances while preserving custom images and running instances.
- Replace the placeholder player with Steve, Alex, and seven other official default characters, plus PNG uploads and both arm models.
- Display only the active account name above the player; replace the globe-style Worlds glyph with a landscape symbol.
- Prepare only client overrides directly into private staging instead of unpacking and copying them twice. Preserve archive path/CRC checks, cancellation, hashes, and transactional installation.
- Benchmark on the same cached Cobblemon 1.8.4 archive: 2,513 planned files prepared in 5.9 seconds versus 16.5 seconds before; all 2,436 staged override payloads matched byte for byte. Download/game-install time is separate.

## 0.3.0 — Your setup, your space

- Remove the breadcrumb strip and the colored outline on selected tabs; retain keyboard focus indicators.
- Give Discover covers a compact project icon badge, and use a distinct two-pane instance chooser.
- Support managed GIF icons and image/GIF instance backgrounds, with reduced-motion still frames and artwork limits.
- Add saved worlds and servers to Library and instance details, including world icons, real server latency/player counts, and configurable signal colors.
- Support direct world/server launch when the installed Minecraft profile declares quick-play arguments.
- Export and import Moly portable .mlypack/.zip setups with optional worlds; export standard .mrpack files with verified hashes and source checks.
- Automatically prepare Minecraft and Java after a new modpack or portable setup is installed.
- Add the Wardrobe: PNG skin collection, Classic/Slim selection, a textured rotatable player preview, optional idle motion, and an authenticated account-apply action.
- Add a bottom task-center button with measured transfer bytes/speed, automatic expansion, cancellation, retry, and dismissible completed tasks.
- Add immediate appearance/activity preferences for artwork, player movement, quick access, server checks, task behavior, signal color, and background shading.

## 0.2.3 — Compact install confirmation

- Fit install dialogs to their actual content instead of reserving a large empty file list.
- Show preparation/install progress in a compact panel without a misleading zero-file count.
- Confirm installation once; keep the optional file breakdown behind View files.
- Expand restricted downloads automatically and allow scrolling on small windows.
- Resume preparation if Install is opened during another background task; offer Retry for failed preparation.
- Handle ZIP folders with missing directory attributes so their nested pack files can be extracted.

## 0.2.2 — Instance overview and immediate reduced motion

- Apply and save Reduce motion as soon as its switch is clicked, including during background work.
- Hide the animated loading-more spinner in reduced-motion mode; retain its loading text.
- Add a selected-instance overview with version, mod count, last played, RAM, Play, and content/folder shortcuts.
- Size the instance list to its contents and scroll the page on smaller windows.
- Select a list/grid item to preview it; double-click or use Manage content to open its details.

## 0.2.1 — Clear mod removal

- Use a compact removal confirmation instead of an empty install review for delete-only changes.
- Show the exact file, a Remove action, and backup/recovery guidance.
- Report removal success accurately; verify removal, manifest cleanup, backup, restore, and preservation of neighboring files/worlds.

## 0.2.0 — Moly dock and liquid controls

- Move Library, Discover, Accounts, and Settings into a rounded bottom dock; remove the Downloads page.
- Show centered, rounded instance tiles in the left rail and keep current-task details beside the dock.
- Use the supplied droplet-M logo in the interface and embedded Windows executable icon.
- Center button labels and icons as one group; add soft popup and tab transitions.
- Keep content-list delegates alive when toggling mods so the thumb slides smoothly.
- Retain provider logos and read bounded, cached embedded logos from local Fabric/Quilt/legacy mods.
- Append provider search pages while preserving cards, ordering, and scroll position; defer more requests safely.
- Show measured byte/file progress for the current install step; unknown work is static Preparing with no pulsing fill.
- Automatically install new instances, allow local instance-image selection, and open the launched instance's Logs tab.
- Add a guided Microsoft app-registration explanation in Accounts and a local setup guide.
- Add unit and live regression checks for icons, pagination, auto-install, and switch animation continuity.

## Unreleased — desktop interface refresh

### Visual direction

- Replace the large promotional hero, decorative cubes, and purple surfaces with a compact neutral-dark workspace.
- Use shared theme tokens for surfaces, borders, typography, focus states, and the accent color.
- Use a small flat Moly mark and wordmark in the sidebar.
- Make the library a compact list by default, with search, sorting, and an optional grid view.
- Use clearer instance headers, compact content rows, and restrained tab/page transitions.
- Add an appearance section with preset and custom accent colors, saved in the existing settings store.
- Keep controls usable at the 1024 × 640 minimum window size and respect reduced motion.

### New interface files and assets

- `qml/Theme.qml` centralizes the color and typography tokens instead of repeating them in every page.
- `assets/moly.svg` and `assets/moly-monochrome.svg` provide flat, scalable branding.
- `assets/Inter.ttf` bundles the interface font locally; `assets/Inter-OFL.txt` preserves its license.
- No JavaScript UI framework or animation library is added. Transitions use Qt Quick, which the app already depends on.
- Add Qt's matching WebP image-format plugin for real provider thumbnails. The existing local kit omitted this optional decoder; `scripts/ensure-webp.ps1` retrieves the official 6.11.2 MinGW archive, verifies its pinned hash and repository checksum, and copies only `qwebp.dll` beside the build. It does not change the global Qt installation.

### Interaction changes

- Keep the requested content category when browsing from an instance: browsing mods opens Mods rather than Modpacks.
- Show popular projects before searching, then show explicit loading, empty, and error states.
- Present the target instance and required dependencies together during installation.
- Keep install errors in the dialog, remember the target instance, and offer compatible version selection.
- Capture the target when resolving dependencies so changing selection cannot retarget an install.
- Guard review entry points against exceptions escaping into QML.
- Verify the portable layout with Qt absent from PATH.
- Keep account, game installation, and launch behavior behind the existing engine interfaces.

The refresh builds successfully, both automated suites pass, and real Modrinth/CurseForge install regressions pass. The verification record in [docs/TESTING.md](docs/TESTING.md) distinguishes completed checks from remaining manual checks.

## 0.1.0 — initial Windows preview

- Add C++20, Qt Quick, and CMake project structure with a preset for the installed MinGW Qt kit.
- Add saved instances, local profiles, Microsoft browser sign-in, per-instance settings, and live logs.
- Add Minecraft downloads, Java setup, and loader installation paths for Fabric, Quilt, Forge, and NeoForge.
- Add Modrinth and CurseForge browsing, dependency review, local imports, and modpack imports.
- Add content backups and rollback, world backups, archive path checks, and download hash verification.
- Add engine unit tests, a live-service smoke-test helper, and packaging recipes.
