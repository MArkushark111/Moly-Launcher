# Moly — Rust Edition

Rust instances now use the actual Horizon engine, replacing the discarded creative prototype.
The upstream source is pinned as the Git submodule `third_party/horizon` at
`cc94ed7afef3ca5fa57936de1df3f111ca39cc65`. Its MIT copyright and third-party notices
ship with Moly. Moly applies `patches/horizon-moly-branding.patch` for its visible name, loading screens, icons and translated menu labels. Upstream copyright notices remain in the shipped licenses; internal engine identifiers stay compatible.

## Build

Run `git submodule update --init --recursive`, then the normal CMake build. The
`moly_rust_engine` target invokes `scripts/build-rust-engine.ps1` to compile
`horizon-client` and `horizon-prepare` from that source. Rust/MSVC tools and Java 25
are needed for preparation/exporter development. Original exporter classes may be
bundled; Minecraft jars, extracted assets and prepared resources must never be
committed or included in the installer.

## Instances

Create instance → Rust Edition. Moly prepares resources from Minecraft 26.3 into
its user-local cache, shared between instances. Each instance keeps its own game
folder, saves, options and logs. Preparation can take several minutes the first
time and is cancellable; incomplete preparation does not mark an instance ready.
Online account sessions are refreshed by Moly and passed to the engine on stdin,
never in process arguments. The engine implements server authentication.

Use the engine's normal controls: WASD, mouse look, Space jump, Shift sneak,
Ctrl sprint, E inventory, and left/right click break/use. Graphics, distant terrain,
key bindings, zoom and other engine/mod capabilities are governed by the upstream
engine and installed compatible mods, not the retired prototype's implementation.

Moly `.rjar` files are ZIP content packages using `horizon-mod.toml` and the
engine's data/assets format. Moly passes enabled `.rjar` files through the engine's
explicit content-mod API. Java JARs and the old prototype `moly.mod.json` packages
are incompatible. Native Rust mods need a separate compilation pipeline; renaming
a DLL or JAR does not make it compatible. Native `.hmod` files can also be placed
in the game folder's `horizon-mods` directory and managed by the game.

The old prototype's `saves/world.json` is preserved but is not a Minecraft world;
create a world in the actual engine. Use Repair / Install on an existing prototype
instance to prepare the new engine.


## Moly visual profiles

The instance header's Visuals button opens Moly's own Rust visuals page. Enable
Manage graphics from Moly, pick Swift/Everyday/Vista or customize the controls,
and Save. Vista selects 20 detailed chunks and a 1024-chunk distant landscape.
Distant terrain uses LOD, not full-detail blocks across that entire radius.
Some integrated GPUs cap far terrain at 128 chunks in this engine.

Saved overrides are applied before the next launch to options.txt and
horizon-graphics.txt, preserving other options and bindings. Disable management
to keep subsequent in-game edits under the game's control. No running instance
can be changed. FPS cap, render scale, FXAA, VSync and graphics API are included.

Moly Colour is an original, adjustable colour-grade shader included with the
launcher. Native .hshader files and compatible shaders/ ZIPs can be imported or
installed from Discover, then selected in the Visuals page. Their individual
options and compile errors are available in the game's Shader Packs screen.
Third-party pack compatibility varies; shader support does not make Java mod
JARs compatible with Rust Edition.

## Engine refresh — 5 November 2026

Updated the engine pin from `6475f62f3` to `cc94ed7af`. The Minecraft target remains
26.3 (protocol 777). This incorporates upstream shader shadow/custom-uniform fixes,
controller menu navigation, rendering and gameplay changes. The Moly branding
patch was rebased onto the new engine; upstream data-pack reload/error handling
is preserved. Existing instances retain their own data folders. Test launches
use isolated homes, never player saves.
The updated engine uses its own `cache/rust-engine/cc94ed7` resource cache;
the earlier cache is retained. First launch may prepare resources again.
