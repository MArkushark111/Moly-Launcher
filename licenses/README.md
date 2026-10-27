# Distribution notices

This directory accompanies packaged Moly Launcher builds. See `THIRD_PARTY.md` in the project root for the component and service inventory.

- `Inter-OFL.txt` is the SIL Open Font License supplied with the bundled Inter font.
- `libwebp-COPYING.txt` is the BSD 3-Clause notice from Qt Image Formats 6.11.2's bundled libwebp source. It accompanies the locally deployed `qwebp.dll` image decoder.
- Preserve the Qt and compiler-runtime license texts supplied with the exact runtime binaries in the package. The local Qt installation also carries notices under `C:/Qt/Licenses`; do not substitute an unrelated Qt version's notices for a distribution manifest.
- The application uses Qt's private ZIP reader and therefore requires the matching Qt build. Qt deployment copies the needed dynamic runtime libraries into the package.
- Inno Setup is an optional build tool. The generated installer does not make its compiler part of the launcher runtime.

This file is an inventory aid, not a replacement for upstream license texts.
