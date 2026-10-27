# Third-party components and services

Moly is an independent Minecraft Java Edition launcher. It is not an official Minecraft product and is not approved by or associated with Mojang or Microsoft. Game binaries, mods, packs, and Java runtimes are downloaded from their respective providers. The default character textures used by the skin preview are credited below.

## Components

| Component | Use | License / notices |
|---|---|---|
| Qt 6 | Desktop interface, networking, storage, concurrency, and tests | Qt modules in this project are available under Qt's commercial terms or applicable open-source terms, including LGPLv3/GPLv3. See the exact installed module notices. |
| Qt private ZIP reader | Archive inspection and extraction | Part of Qt Core. The application must ship the matching Qt runtime build because this is a private API. |
| Qt Image Formats / `qwebp.dll` | Decodes provider icons and thumbnails in WebP format | Official Qt 6.11.2 MinGW plugin, supplied under the applicable Qt open-source or commercial terms. Its bundled libwebp 1.6.0 uses the BSD 3-Clause license; see `licenses/libwebp-COPYING.txt`. |
| MinGW-w64 / GCC runtime | Windows runtime libraries in MinGW builds | GCC Runtime Library Exception and the licenses of the distributed MinGW-w64 runtime components apply. |
| Inter | Locally bundled interface font at `assets/Inter.ttf` | SIL Open Font License 1.1; the license is included in `assets/Inter-OFL.txt` and `licenses/Inter-OFL.txt`. |
| Inno Setup, when used | Builds the optional Windows installer | Inno Setup license; it is a build tool, not a launcher runtime dependency. |

Qt is linked dynamically in the packaged application. Preserve its license notices and the corresponding library files in distribution packages. Review the exact runtime components produced by Qt's deployment tool before publishing a release.

Upstream references:

- [Qt licensing](https://www.qt.io/licensing/)
- [Qt source](https://code.qt.io/)
- [Qt WebP attribution](https://doc.qt.io/qt-6/qtimageformats-attribution-libwebp.html)
- [GCC Runtime Library Exception](https://www.gnu.org/licenses/gcc-exception-3.1.html)
- [MinGW-w64 licenses](https://www.mingw-w64.org/license/)
- [Inter font](https://github.com/rsms/inter)
- [Inno Setup](https://jrsoftware.org/isinfo.php)

## Download and account services

| Service | Use |
|---|---|
| Mojang / Minecraft | Game version metadata, libraries, assets, account ownership, and profiles |
| Microsoft / Xbox | Browser-based account authentication |
| Eclipse Adoptium | Automatic Java runtime downloads |
| Fabric Meta / Quilt Meta | Loader versions and launcher profiles |
| Forge / NeoForge | Official loader installers and libraries |
| Modrinth | Public project search, versions, packs, and files |
| CurseForge | API-key-authenticated project search and permitted downloads |

Downloaded files retain their creators' licenses and distribution restrictions. Moly follows provider download URLs and checks available hashes. A provider's browser-only download step is preserved when automatic distribution is unavailable.

The Moly SVG mark is project artwork. No Modrinth or FastClient branding is used as Moly's logo.

## Minecraft default skins

The Steve, Alex, Ari, Efe, Kai, Makena, Noor, Sunny, and Zuri texture variants are official Mojang/Microsoft Minecraft assets, extracted from the checksum-verified Java 1.20.1 client. They replace the former Moly placeholder. See `licenses/Minecraft-default-skins.txt` and `assets/default-skins/provenance.json` for provenance. Character overview: https://www.minecraft.net/en-us/article/introducing-new-default-skins

Provider marks: Modrinth and CurseForge SVGs are from [Simple Icons](https://github.com/simple-icons/simple-icons), CC0. They identify content sources; their names and marks belong to their respective owners. Native ArcSpinner is an original Qt implementation inspired by [Loading UI Arc](https://www.loading-ui.com/docs/components/arc).

Import-source logos identify their respective launchers. Prism mark: https://github.com/PrismLauncher/PrismLauncher/blob/develop/program_info/org.prismlauncher.PrismLauncher.svg ; MultiMC mark: https://github.com/MultiMC/Launcher/blob/develop/branding/logo.svg ; ATLauncher mark: https://github.com/ATLauncher/ATLauncher/blob/master/src/main/resources/assets/image/icon.png ; Lunar Client favicon: https://support.lunarclient.com/favicon.ico . These marks belong to their respective owners; Moly is independent of those projects. The generic Minecraft-folder pictogram is original Moly artwork.
