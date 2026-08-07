---
name: convert-assets
description: Regenerate the gitignored .cbkm/.cbkt binary assets by running CBKAssetConverter over the raw sources in Sandbox/assets. Use after a fresh clone, when the Sandbox fails to load a model or texture, or when a raw .obj/.png is added or changed. Optionally takes a path to convert a single file.
---

# Converting assets

The engine loads **only** its own binary formats — `.cbkm` (models, magic `0x43424B4D`) and `.cbkt` (textures, magic `0x43424B54`, lz4-compressed pixels). It never reads `.obj` or `.png` at runtime. Both formats are gitignored, so a fresh clone has committed raw sources and no loadable assets, and the Sandbox fails at load time with a `CBK_CORE_ERROR` rather than anything more obvious.

`$ARGUMENTS`, if given, is a single file to convert instead of the whole tree.

## Build the converter

```bash
make config=release CBKAssetConverter -j$(nproc)
```

It is a separate Premake project depending on Assimp and Common — it does not need the engine or a GPU, so this works on any machine regardless of renderer backend.

## Usage

The CLI takes **one file per invocation** (`CBKAssetConverter/src/Main.cpp:11-43`):

```
CBKAssetConverter <file> [--max-tex <N>]
```

- Models: `.obj`, `.fbx`, `.gltf`, `.dae` → `.cbkm`
- Textures: `.png`, `.jpg`, `.jpeg`, `.tga`, `.bmp`, `.hdr` → `.cbkt`
- `--max-tex <N>` caps texture dimensions at N (default 0 = no cap). Useful for keeping WASM builds small.

Anything else logs `Unsupported file format` and exits 0 — so check the log, not the exit code.

## Batch conversion

```bash
CONV=./bin/Release-linux-x86_64/CBKAssetConverter/CBKAssetConverter
find Sandbox/assets -type f \
  \( -iname '*.obj' -o -iname '*.fbx' -o -iname '*.gltf' -o -iname '*.dae' \
     -o -iname '*.png' -o -iname '*.jpg' -o -iname '*.jpeg' \
     -o -iname '*.tga' -o -iname '*.bmp' -o -iname '*.hdr' \) \
  -exec "$CONV" {} \;
```

Adjust the `Release-linux-x86_64` segment to match the host — the path is `bin/<Config>-<system>-<arch>/CBKAssetConverter/CBKAssetConverter`.

Converting a model usually also converts its referenced textures, so a model-only pass often suffices; run the full sweep when in doubt, since it is idempotent.

## Verifying

The outputs land next to their sources. Confirm they exist and that the Sandbox loads them:

```bash
find Sandbox/assets -name '*.cbkm' -o -name '*.cbkt' | head
cd bin/Release-linux-x86_64/Sandbox && ./Sandbox --log-level=debug
```

Note that premake copies `assets/` into the output directory as a **post-build** step. Converting assets after a build leaves the copy under `bin/` stale — rebuild (or copy the new files across) before running, or the Sandbox will keep loading the old ones.
