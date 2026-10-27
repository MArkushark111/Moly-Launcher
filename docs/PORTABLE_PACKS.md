# Portable setup exports

Moly exports three ZIP-based formats:

- **`.mlypack`** is a complete Moly setup: installed mods (including disabled files), resource and shader packs, game configuration, options, server list, instance settings, and PNG/GIF artwork. Worlds are included only when selected.
- **`.zip`** uses the same portable layout and can be imported by Moly. It is not a CurseForge pack manifest.
- **`.mrpack`** follows the [Modrinth pack format](https://support.modrinth.com/en/articles/8802351-modrinth-modpack-format-mrpack). Managed content uses its exact HTTPS download, SHA-1/SHA-512 hashes, size, game version, and loader version. Local files become overrides. Worlds and Moly artwork/settings are omitted.

Minecraft binaries, Java, assets, libraries, launcher accounts, machine-specific Java paths/JVM arguments, known account/session/token files, logs, and screenshots are excluded. Ordinary mod configuration remains part of the setup; it is not a generic secret-redaction service for arbitrary mod settings.

Restricted CurseForge files are not embedded. `.mrpack` cannot represent a managed file without an exact compatible public download, or a managed file whose bytes have been edited locally; use a portable format for edited files. Files whose provider restricts export must be handled separately according to the author's rules.

## Moly layout, version 1

```text
moly-pack.json
instance.json
content.json
icon.png or icon.gif             (optional)
background.png or background.gif (optional)
game/
  mods/
  config/
  resourcepacks/
  shaderpacks/
  options.txt
  servers.dat
  saves/                        (optional)
```

`moly-pack.json` identifies `format: "moly-instance-pack"`, `schemaVersion: 1`, the portable instance descriptor, sanitized managed-content metadata, whether worlds were included, and every payload file's path, size, and SHA-512 hash. Descriptor copies make the ZIP easy to inspect; Moly imports the descriptor and content metadata from `moly-pack.json`.

Imports validate paths, archive limits, the manifest version, allowed files, sizes, and hashes before creating an instance. They receive a fresh instance ID and artwork revisions. Import commits by directory rename after all payload files have been staged, and cancellation does not leave a half-created instance. Original instances are never modified.

Exports write a temporary replacement through `QSaveFile`, then commit only after the ZIP is complete and source files have been checked again. Cancellation preserves an existing export at the same destination. Current limits match Moly's archive reader: 512 MiB per payload file and 8 GiB total.

Minecraft and Java are deliberately not redistributed. After import, Moly must prepare the matching game/runtime before Play; reusable files already present in the launcher's cache can be shared.
