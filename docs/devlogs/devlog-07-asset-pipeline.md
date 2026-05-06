# Devlog #7 — A Custom Asset Pipeline: CBKM and CBKT

> **[PERSONALIZE]** Add a line about the performance problem that triggered this — was it slow OBJ parsing at startup? Large file sizes? What was the thing that finally made you build the converter?

---

Loading assets at runtime with Assimp is convenient during development and genuinely terrible in production. Assimp is a large, general-purpose library that handles 40+ file formats. It does a lot of work parsing every mesh: triangulation, normal generation, tangent space calculation, scene graph traversal. For a `.fbx` file with multiple meshes and embedded textures, startup could take multiple seconds.

The other problem was that my engine didn't actually need most of what Assimp produces. I needed vertices (position, normal, UV, tangent), indices, texture paths, and a few material scalars. That's it. Everything else — scene hierarchies, skeletal animation, blend shapes, cameras embedded in the file — I was throwing away anyway.

The right solution was a dedicated converter: run Assimp once at authoring time, write a compact binary file, and have the engine load that.

## CBKAssetConverter

`CBKAssetConverter` is a standalone CLI tool that takes a raw asset file and writes the engine's binary format alongside it:

```bash
CBKAssetConverter assets/models/gun/Cerberus_LP.gltf
# → assets/models/gun/Cerberus_LP.cbkm

CBKAssetConverter assets/textures/albedo.png
# → assets/textures/albedo.cbkt
```

Format dispatch is by file extension. Models go through `ModelConverter`, textures through `TextureConverter`.

## The model format: `.cbkm`

CBKM (Cabrankengine Model) is a simple sequential binary layout:

```
ModelHeader        magic "CBKM", version, mesh/texture/property counts
TextureEntry[]     type enum + path string per texture
PropertyEntry[]    material scalar (shininess, metalness, roughness, base color)
[MeshHeader + Vertex[] + Index[]] × numMeshes
```

Each vertex is 52 bytes: 3 floats position, 3 normal, 2 UV, 3 tangent. Indices are `uint32_t`. The engine maps this directly into GPU buffers with a single `memcpy`-style upload — no parsing, no conversion.

A typical 50k-triangle model went from ~400ms to load via Assimp to ~8ms to load the `.cbkm`. That's the whole point.

## The texture format: `.cbkt`

CBKT (Cabrankengine Texture) stores compressed pixel data with a small header:

```
TextureHeader    magic "CBKT", version, width, height, channels,
                 compressedSize, uncompressedSize
[compressed pixel data]
```

Compression brings the file size down significantly for large textures, with decompression fast enough that it doesn't affect startup time meaningfully.

## PBR metal/roughness packing

PBR assets from tools like Substance Painter often ship as separate metalness and roughness grayscale images. The engine (and most real-time renderers) want them packed into a single texture to save a sampler slot:

```
R = 0
G = roughness
B = metalness
A = 255
```

`TextureConverter::packMetalRough` takes two input paths and writes a single `.cbkt`:

```cpp
TextureConverter::packMetalRough(
    "assets/textures/Cerberus_M.png",
    "assets/textures/Cerberus_R.png",
    "assets/textures/Cerberus_MR.cbkt"
);
```

This matches the glTF convention for combined metal/roughness textures and works directly with the PBR shader.

## Shared format structs

Both the converter and the engine read/write using the same structs, defined in `Common/src/Common/BinaryFormats.h`. This is the only file shared between the two projects. If the format changes, both sides update from one place.

## What I learned

Writing a binary format is mostly about alignment and thinking hard about what you'll need later. The current format is version 2 of CBKM because version 1 didn't store the texture type enum, just a flat list of paths. I needed to add `TextureType` to distinguish albedo from normal from metal-roughness when loading. Adding a version number to the header from the start is not optional.

## What's next

The engine builds for OpenGL on Linux and Windows. I wanted to see it run in a browser. That meant WebAssembly.

---

*[PERSONALIZE: what was the most satisfying part of seeing the converter work? Was there a moment where startup time visibly dropped?]*

**Links:** [GitHub repo](https://github.com/cabranca/game-dev) | [CBKAssetConverter source](https://github.com/cabranca/game-dev/tree/main/CBKAssetConverter/src)
