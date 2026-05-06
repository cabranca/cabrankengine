# Asset Pipeline

Cabrankengine does not load raw mesh or image files at runtime. Instead, a standalone CLI tool — `CBKAssetConverter` — converts them into compact binary formats the engine reads directly. This keeps runtime loading fast and dependencies out of the engine itself.

---

## Build the converter

```bash
make config=debug CBKAssetConverter
# or
make config=release CBKAssetConverter

# Binary lands at:
./bin/Debug-linux-x86_64/CBKAssetConverter/CBKAssetConverter
```

For convenience, add it to your PATH or alias it.

---

## Usage

```
CBKAssetConverter <file>
```

The converter detects the format from the file extension and writes the output alongside the input file.

---

## Supported Conversions

### Models → `.cbkm`

| Input format | Notes |
|-------------|-------|
| `.obj` | Wavefront OBJ |
| `.fbx` | Autodesk FBX |
| `.gltf` / `.glb` | GL Transmission Format |
| `.dae` | COLLADA |

Powered by [Assimp](https://assimp.org/). Extracts meshes, vertices, indices, UV coordinates, normals, tangents, texture paths, and material properties (shininess, metalness, roughness, base color).

```bash
CBKAssetConverter assets/models/backpack/backpack.obj
# → assets/models/backpack/backpack.cbkm
```

### Textures → `.cbkt`

| Input format | Notes |
|-------------|-------|
| `.png` | |
| `.jpg` / `.jpeg` | |
| `.tga` | |
| `.bmp` | |
| `.hdr` | High dynamic range |

Pixel data is compressed and written with a header containing width, height, channel count, and compressed/uncompressed sizes.

```bash
CBKAssetConverter assets/textures/albedo.png
# → assets/textures/albedo.cbkt
```

---

## PBR Metal/Roughness Packing

PBR workflows often ship metalness and roughness as separate grayscale images. The engine expects them packed into a single texture (following the glTF convention):

```
R = 0
G = roughness
B = metalness
A = 255
```

Use `TextureConverter::packMetalRough` from C++ (or add it to your build pipeline):

```cpp
#include "TextureConverter.h"

cbk::ac::TextureConverter::packMetalRough(
    "assets/textures/metalness.png",
    "assets/textures/roughness.png",
    "assets/textures/metal_rough.cbkt"
);
```

The output `.cbkt` can then be referenced from a `.cbkm` material as `TextureType::MetalRoughness`.

---

## Binary Format Reference

These structs are shared between the converter and the engine via
[Common/src/Common/BinaryFormats.h](../Common/src/Common/BinaryFormats.h).

### `.cbkt` — texture binary (v1)

```
TextureHeader
  magic            = 0x43424B54   ("CBKT")
  version          = 1
  width, height    pixels
  channels         1–4
  compressedSize   byte count of compressed payload
  uncompressedSize byte count before compression

[compressed pixel data]
```

### `.cbkm` — model binary (v2)

```
ModelHeader
  magic       = 0x43424B4D   ("CBKM")
  version     = 2
  numMeshes
  numTextures
  numProperties

TextureEntry[numTextures]
  type        Diffuse=1, Specular=2, Normal=3, MetalRoughness=4, AO=5
  pathLength
  char[pathLength]

PropertyEntry[numProperties]
  key         Shininess=1, Metalness=2, Roughness=3
              BaseColorR=4, BaseColorG=5, BaseColorB=6
  value       float

MeshHeader + data [repeated numMeshes times]
  numVertices
  numIndices
  Vertex[numVertices]
    px, py, pz    position
    nx, ny, nz    normal
    tx, ty        UV coordinates
    tanx, tany, tanz  tangent

  uint32_t[numIndices]
```

---

## Typical Workflow

1. Export your model from Blender/Maya as `.gltf` or `.fbx`.
2. Export textures as `.png` (albedo, normal, AO) and separate metalness/roughness if using PBR.
3. Run the converter on the model — it embeds texture paths automatically.
4. If using PBR, run `packMetalRough` on the metal and roughness maps to get a single `.cbkt`.
5. Reference the `.cbkm` in a `PBRModelArch` or `PhongModelArch` in your layer.

```cpp
PBRModelArch gun{ "assets/models/gun/Cerberus_LP.cbkm" };
```

The engine loads the `.cbkm` at startup and resolves the embedded texture paths relative to the model file's directory.
