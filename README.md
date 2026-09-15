# Cabrankengine

A C++23 game engine and editor built from scratch as a learning project and portfolio piece. The `main` branch is being rebuilt around a modern **Vulkan 1.4** renderer, and **CBKEditor**, an ImGui-based scene editor, is growing alongside it. The engine covers an ECS core, Phong and PBR material pipelines, scene serialization, and a custom binary asset format.

> The OpenGL 4.5 / OpenGL ES backends and the WebAssembly target were removed from `main`. Their last working state, including the 2D batch renderer shown below, lives on the [`legacy`](https://github.com/cabranca/cabrankengine/tree/legacy) branch.

---

## Status

| Area | State on `main` |
|------|-----------------|
| Vulkan renderer (Linux, Windows) | Working, being modernized |
| Metal renderer (macOS) | Broken since the renderer refactor, port pending |
| CBKEditor | Dockable Outliner / Viewport / Details panels; new, save, save as and load scene. A transform gizmo is next |
| 2D batch renderer, text rendering | Disabled on `main` after the pipeline refactor |
| OpenGL, WebAssembly | Removed (see `legacy`) |

---

## Showcase

*Captured on the `legacy` branch (OpenGL backend).*

### Phong and PBR lighting

![Phong and PBR lighting](docs/media/phong_and_pbr.gif)

### PBR material detail

![PBR material detail](docs/media/pbr.gif)

### Vampire Survivors style prototype

![Survivors-like prototype](docs/media/Survivors-like.gif)

### 2D Batch Rendering

![2D Batch Rendering](docs/media/2DBatchRender.gif)

### ImGui Debug Overlay

![ImGui Debug Overlay](docs/media/DebugImGui.gif)

---

## Features

- **Entity Component System**: registry-based ECS with typed component arrays, signature-filtered systems, and up to 20 000 concurrent entities
- **Vulkan 1.4 renderer**: dynamic rendering (no render passes), synchronization2, VMA-backed memory, and Slang shaders compiled to SPIR-V at runtime
- **Pipeline-per-material-kind**: a shared scene UBO and point-light SSBO serve every pipeline, each material instance gets its own descriptor set, and per-draw data goes through push constants
- **Render-to-texture**: the scene can render offscreen and be shown inside the editor viewport, or go straight to the swapchain for standalone apps
- **3D materials**: Phong and PBR (metal/roughness workflow)
- **Lighting**: a directional light plus up to 10 point lights with attenuation
- **Camera system**: perspective and orthographic projections, plus a first-person controller
- **CBKEditor**: docked Outliner, Viewport and Details panels, a default layout with reset, and scene new / save / save as / load
- **Scene serialization**: scenes save and load as JSON (`.cbkscn`)
- **Custom asset pipeline**: `CBKAssetConverter` converts `.obj/.fbx/.gltf` to `.cbkm` and images to LZ4-compressed `.cbkt`, including PBR metal/roughness packing
- **Collision shapes**: AABB, sphere, OBB, capsule, plane, cylinder (2D/3D)
- **Unit tests**: ECS, math, and the collision solver, with Catch2

---

## Quick Start

```cpp
// MyApp.cpp
#include <Cabrankengine.h>
#include <Cabrankengine/Core/EntryPoint.h>

using namespace cbk;
using namespace cbk::ecs;
using namespace cbk::math;
using namespace cbk::scene;
using namespace cbk::scene::arch;

class MyLayer : public Layer {
public:
    MyLayer() : Layer("MyLayer") {
        // Camera with perspective projection + first-person controller
        CameraControllerArch camera(ProjectionType::Perspective);

        // 3D model with PBR materials
        PBRModelArch gun{ "assets/models/gun/Cerberus_LP.cbkm" };
        gun.transform().Position = { 2.f, 0.f, -5.f };
        gun.transform().Scale    = Vector3(0.05f);

        // Directional light
        DirectionalLightArch sun{};
        sun.light().Direction = { 1.f, -1.f, -1.f };
        sun.light().Radiance  = { 2.f, 2.f, 2.f };

        // Or load a saved scene. The swap happens at the start of the next frame.
        // Application::get().queueSceneLoad(SceneSerializer::deserialize("scenes/myScene.cbkscn"));
    }

    void onUpdate(Timestep dt) override {}
    void onImGuiRender() override {}
};

class MyApp : public Application {
public:
    // false renders straight to the window; CBKEditor passes true to render into a texture.
    MyApp() : Application(false) { pushLayer(createScope<MyLayer>()); }
};

Application* cbk::createApplication() { return new MyApp(); }
```

For a step-by-step walkthrough see [docs/getting-started.md](docs/getting-started.md).

---

## Build

### Requirements

- C++23 compiler (GCC 13+ / Clang 17+ / MSVC 19.38+)
- [Premake5](https://premake.github.io/) 5.0.0-beta8
- **Vulkan SDK 1.4** on every platform (it supplies volk, VMA and Slang), and a GPU driver with Vulkan 1.4 support
- GNU Make (Linux) or Visual Studio 2022 (Windows)
- `zenity` on Linux, for the editor's file dialogs
- macOS builds use Metal through metal-cpp, which is *currently broken on `main`*

### Vulkan SDK

On **Linux**, install the LunarG SDK with the `--set-dep-ld` flag (required by the latest installer to match the engine's linker config), then export `VULKAN_SDK`:

```bash
./vulkansdk-linux-x86_64-*.run --set-dep-ld
export VULKAN_SDK=$HOME/VulkanSDK/<version>/x86_64
```

On **Windows**, include the volk and VMA components when installing the SDK. Premake reads `VULKAN_SDK` and fails without it.

### Steps

```bash
# Clone with submodules (vendor dependencies are submodules)
git clone --recurse-submodules https://github.com/cabranca/cabrankengine.git
cd cabrankengine

# Generate build files
premake5 gmake       # Linux / macOS
premake5 vs2022      # Windows

# Build everything, or a single target
make config=release -j$(nproc)
make config=release CBKEditor
```

### Assets

The engine only loads the converted `.cbkm` / `.cbkt` formats, which are gitignored. After a fresh clone, run the converter over the raw sources in `Sandbox/assets/`:

```bash
./bin/Release-linux-x86_64/CBKAssetConverter/CBKAssetConverter <path/to/model.obj> [--max-tex <N>]
```

See [docs/asset-pipeline.md](docs/asset-pipeline.md) for details.

### Running

Run each binary **from its own output directory**, because asset, config and scene paths are relative to the working directory:

```bash
cd bin/Release-linux-x86_64/Sandbox   && ./Sandbox
cd bin/Release-linux-x86_64/CBKEditor && ./CBKEditor
```

`CBKEditor/assets/` is gitignored. Populate it (shaders plus converted models) before building the editor, and put a scene at `scenes/testScene.cbkscn` next to the binary, because the editor opens it on startup.

### Tests

```bash
make config=debug UnitTests
./bin/Debug-linux-x86_64/UnitTests/UnitTests
```

### Editor setup (clangd)

If clangd floods `Cabrankengine/` with `'algorithm' file not found` or unresolved-symbol
errors, it has autodetected the wrong GCC. Pass `--query-driver=/usr/bin/g++*` to the clangd
binary (`clangd.arguments` in VS Code, `cmd` in nvim-lspconfig). Full explanation in
[docs/getting-started.md](docs/getting-started.md#editor-setup-clangd).

---

## Documentation

| Doc | Description |
|-----|-------------|
| [Getting Started](docs/getting-started.md) | Prerequisites, build steps, first entity walkthrough |
| [Architecture](docs/architecture.md) | Module layout, ECS design, rendering pipeline, system execution order |
| [API Reference](docs/api-reference.md) | Registry, components, archetype builders |
| [Asset Pipeline](docs/asset-pipeline.md) | CBKAssetConverter: converting models and textures |
| [ImGui Widgets](docs/imgui-widgets.md) | Cheat sheet for the widgets in the vendored ImGui docking fork |

---

## Dependencies

| Library | Purpose |
|---------|---------|
| [GLFW](https://www.glfw.org/) | Window and input |
| [volk](https://github.com/zeux/volk) | Vulkan meta-loader (from the Vulkan SDK) |
| [VMA](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) | Vulkan memory allocation (from the Vulkan SDK) |
| [Slang](https://github.com/shader-slang/slang) | Runtime shader compilation to SPIR-V (from the Vulkan SDK) |
| [metal-cpp](https://developer.apple.com/metal/cpp/) | Metal C++ bindings (macOS) |
| [ImGui](https://github.com/ocornut/imgui) | Editor and debug UI (docking branch) |
| [spdlog](https://github.com/gabime/spdlog) | Logging |
| [nlohmann/json](https://github.com/nlohmann/json) | Scene and config serialization |
| [FreeType](https://freetype.org/) | Font rendering |
| [LZ4](https://github.com/lz4/lz4) | Texture compression in `.cbkt` |
| [Catch2](https://github.com/catchorg/Catch2) | Unit testing |
| [Assimp](https://assimp.org/) | Model import (asset converter only) |
| [stb_image](https://github.com/nothings/stb) | Image import (asset converter only) |

---

## Roadmap

Done:

- [x] Vulkan renderer backend
- [x] Pipeline-per-material-kind refactor (Phong, PBR)
- [x] Render-to-texture scene output
- [x] Editor: dockable panels and scene new / save / load

In progress / next:

- [ ] Editor: transform gizmo
- [ ] Fix and finish Metal integration (macOS)

Later:

- [ ] SIMD math library
- [ ] Audio backend
- [ ] Scripting layer

---

## License

Not yet defined. Until then, the project is for personal learning and portfolio purposes only.

---

## Authors

- **Joaquin Cabrera** (cabranca): creator and main developer
- **Francisco Pintar** (Franpintar): contributor
