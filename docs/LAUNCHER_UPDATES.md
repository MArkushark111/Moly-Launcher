# Launcher release updates

Moly checks the public GitHub endpoint
`https://api.github.com/repos/MArkushark111/Moly-Launcher/releases/latest`.
No GitHub login or personal access token is needed. Checks are asynchronous,
limited to a 2 MiB response and 20-second transfer timeout. A session ETag avoids
fetching unchanged metadata repeatedly. Automatic checking starts after eight
seconds, then repeats every six hours, and can be disabled under Settings →
Advanced. Isolated diagnostic launches do not check automatically.

A numerically newer, published stable release adds an Update available button to
the sidebar. Settings → Advanced also offers a manual check and release notes.
Equal/older versions never offer an update. Drafts, prereleases and malformed
version tags are rejected. Empty release lists, connectivity errors and rate
limits have explicit messages and never block Minecraft downloads or play.

The update dialog shows the release description as selectable plain text. Update
opens the matching Windows x64 installer asset in the user's browser. It does not
download or execute software silently, terminate Minecraft, or modify user data.
Close games and Moly, then run the installer wizard. When no matching installer
exists, only the GitHub release page is offered. Only HTTPS links belonging to
this exact repository can become actionable release/download links.

## Publishing a detectable update

1. Choose a version greater than the currently distributed version, for example
   `0.8.5` after `0.8.4`. Update CMakeLists.txt, main.cpp's application version,
   packaging/moly.iss's version fields/output filename and scripts/build.ps1's
   package paths consistently before building the new version.
2. Build and verify the Windows package and installer.
3. Create a published **stable GitHub Release**, tagged `v0.8.5` (or `0.8.5`),
   and write the release description. A Git tag or pushed commit alone does not
   create a release. Mark it as the latest release if GitHub offers that choice.
4. Attach `MolyLauncher-0.8.5-Windows-x64-Setup.exe` as a release asset. The version
   in the asset name must match the release tag. Existing releases named
   `MolyLauncher 0.8.5` (or `Moly Launcher 0.8.5`) with a generic tag such as
   `Release` and an installer named `MolyInstall.exe` are supported too.
   Versioned tags/assets are recommended for future releases. Do not upload game resources,
   credential files, local caches or build logs containing application secrets.
5. From an older Moly build, use Settings → Advanced → Check for updates.

This is a release checker with browser-assisted updating, not an unattended
self-updater. It does not provide executable signing or bypass Windows warnings.
