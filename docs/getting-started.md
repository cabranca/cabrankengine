# Getting Started

This guide walks you through building Cabrankengine from source, running the Sandbox and the editor, and creating your first entities.

> `main` is Vulkan-only on Linux and Windows. The OpenGL backend and the WebAssembly target live on the `legacy` branch. If your machine has no Vulkan 1.4 driver, use that branch instead.

---

## Prerequisites

| Requirement | Notes |
|-------------|-------|
| C++23 compiler | GCC 13+, Clang 17+, or MSVC 19.38+ |
| [Premake5](https://premake.github.io/) 5.0.0-beta8 | Build file generator |
| Vulkan SDK 1.4 | Required on every platform; it provides volk, VMA and Slang. The GPU driver must support Vulkan 1.4 |
| GNU Make | Linux builds |
| Visual Studio 2022 | Windows builds |
| zenity | Linux only, for the editor's *Save Scene as* / *Load Scene* dialogs |
| macOS 12.0+ | Metal backend, *currently broken on `main`* |

---

## Clone

The vendor dependencies are Git submodules. Clone with `--recurse-submodules`:

```bash
git clone --recurse-submodules https://github.com/cabranca/cabrankengine.git
cd cabrankengine
```

If you already cloned without it:

```bash
git submodule update --init --recursive
```

---

## Vulkan SDK Setup

Premake reads the SDK location from `VULKAN_SDK` and errors out if it is unset.

**Linux.** The latest LunarG SDK installer requires `--set-dep-ld` to match the engine's linker configuration:

```bash
./vulkansdk-linux-x86_64-*.run --set-dep-ld
export VULKAN_SDK=$HOME/VulkanSDK/<version>/x86_64
```

Add the `export` to your shell profile so it persists across sessions.

**Windows.** The installer only installs the core SDK by default. Select the **volk** and **VMA** components, because the engine includes their headers from the SDK.

---

## Build (Linux)

```bash
# Generate Makefiles
premake5 gmake

# Build everything in debug mode
make

# Or build a specific project
make config=debug Sandbox
make config=debug CBKEditor
make config=debug CBKAssetConverter
make config=debug UnitTests
make config=release Cabrankengine

# Binaries land in:
# bin/Debug-linux-x86_64/<ProjectName>/<ProjectName>
# bin/Release-linux-x86_64/<ProjectName>/<ProjectName>
```

---

## Build (Windows)

```bash
premake5 vs2022
# Open the generated .sln in Visual Studio 2022 and build normally.
```

---

## Convert the Assets

The engine only loads its own binary formats (`.cbkm` models, `.cbkt` textures), and those are gitignored. A fresh clone has only the raw sources, so convert them before running anything:

```bash
./bin/Debug-linux-x86_64/CBKAssetConverter/CBKAssetConverter Sandbox/assets/models/backpack/backpack.obj
./bin/Debug-linux-x86_64/CBKAssetConverter/CBKAssetConverter Sandbox/assets/models/gun/Cerberus_LP.FBX
```

Premake copies `assets/` next to each binary as a **post-build** step, so convert first and build afterwards, or rebuild after converting. See [asset-pipeline.md](asset-pipeline.md) for the full workflow.

---

## Run the Sandbox

The Sandbox project is a live example application. Asset and config paths are relative to the working directory, so run it **from its output directory**:

```bash
cd bin/Debug-linux-x86_64/Sandbox
./Sandbox
./Sandbox --log-level=debug   # more verbose logging
```

`config.json` (window title and size) is created next to the binary on first run.

Source: [Sandbox/src/SandboxApplication.cpp](../Sandbox/src/SandboxApplication.cpp)

---

## Run the Editor

```bash
cd bin/Debug-linux-x86_64/CBKEditor
./CBKEditor
```

The editor needs two things a fresh clone doesn't provide:

- **Assets.** `CBKEditor/assets/` is gitignored, and the post-build step copies it only if it exists. Create it with at least `shaders/` (the Vulkan pipelines load `Phong.slang` and `PBR.slang` at startup) plus any converted models your scenes reference. Copying from `Sandbox/assets/` is the quickest start.
- **A startup scene.** The editor opens `scenes/testScene.cbkscn` relative to the binary, so that file must exist.

Source: [CBKEditor/src/](../CBKEditor/src/)

---

## Run the Tests

```bash
make config=debug UnitTests
./bin/Debug-linux-x86_64/UnitTests/UnitTests
```

Tests cover ECS, math types, and the collision solver. See [UnitTests/src/](../UnitTests/src/).

---

## Editor Setup (clangd)

The project builds with a `compile_commands.json` (generate it with `bear -- make`
or a Premake compile-commands exporter). clangd reads that database for its flags.

### clangd picks the wrong GCC and can't find standard headers

**Symptom:** every file in `Cabrankengine/` lights up with errors like
`'algorithm' file not found` or thousands of unresolved-symbol diagnostics, even
right after regenerating `compile_commands.json`. It comes and goes across
`apt upgrade`s.

**Cause:** the compile database is fine — it names `/usr/bin/g++`. The problem is
clangd's built-in GCC-toolchain detection: it scans `/usr/lib/gcc/x86_64-linux-gnu/`
and picks the **highest version number** it finds. If a C-only `gcc-N` package (or
`libgcc-N-dev`) is installed ahead of the matching `g++-N` / `libstdc++-N-dev`,
that directory exists but has no C++ standard library, so `<algorithm>` and
friends resolve to a path that doesn't exist and the whole translation unit
collapses.

**Fix:** tell clangd to ask the real compiler for its system include paths, with
`--query-driver`. This is a clangd binary argument, not something that fits in
`.clangd`, so it goes in your editor config:

- **VS Code** — `.vscode/settings.json`:

  ```json
  "clangd.arguments": ["--query-driver=/usr/bin/g++*"]
  ```

- **Neovim** (nvim-lspconfig):

  ```lua
  require('lspconfig').clangd.setup {
      cmd = { "clangd", "--query-driver=/usr/bin/g++*" },
  }
  ```

Restart the language server and wipe `.cache/clangd/` after changing it.

**Diagnose it yourself:** `clangd --check=path/to/File.cpp` prints the exact
compiler invocation clangd built and the first errors it hit — that is how you
confirm which `include/c++/<version>` path it landed on.

Alternative (editor-agnostic, but distro-specific and needs a manual bump when you
upgrade GCC): pin the toolchain in `.clangd` with
`CompileFlags: { Add: [--gcc-install-dir=/usr/lib/gcc/x86_64-linux-gnu/15] }`.

---

## Your First Layer

All game logic lives in `Layer` subclasses pushed onto the application's `LayerStack`. Here is the minimal pattern:

### 1. Create the layer

```cpp
// MyLayer.h
#pragma once
#include <Cabrankengine.h>

class MyLayer : public cbk::Layer {
public:
    MyLayer();
    void onUpdate(cbk::Timestep dt) override;
    void onImGuiRender() override;
    void onEvent(cbk::Event& e) override;
};
```

### 2. Spawn entities in the constructor

```cpp
// MyLayer.cpp
#include "MyLayer.h"

#include <imgui.h>

using namespace cbk;
using namespace cbk::ecs;
using namespace cbk::math;
using namespace cbk::scene::arch;

MyLayer::MyLayer() : Layer("MyLayer") {
    // Perspective camera with a first-person controller
    CameraControllerArch camera(ProjectionType::Perspective);

    // 3D model with Phong materials
    PhongModelArch backpack{ "assets/models/backpack/backpack.cbkm" };
    backpack.transform().Position = { -2.f, 0.f, -5.f };

    // 3D model with PBR materials
    PBRModelArch gun{ "assets/models/gun/Cerberus_LP.cbkm" };
    gun.transform().Position = { 2.f, 0.f, -5.f };
    gun.transform().Scale    = Vector3(0.05f);

    // Directional light
    DirectionalLightArch sun{};
    sun.light().Direction = { 1.f, -1.f, -1.f };
    sun.light().Radiance  = { 2.f, 2.f, 2.f };

    // Point light, positioned through its transform
    PointLightArch lamp{};
    lamp.transform().Position = { 0.f, 0.f, 2.f };
    lamp.light().Radiance     = { 5.f, 0.f, 0.f };
}

void MyLayer::onUpdate(Timestep dt) {
    // Per-frame logic goes here. The built-in RenderLayer updates before your
    // layers, so changes made here are drawn on the next frame.
}

void MyLayer::onImGuiRender() {
    ImGui::Begin("Debug");
    ImGui::End();
}

void MyLayer::onEvent(Event& e) {}
```

`SpriteArch` and `TextArch` also exist, but 2D sprite and text rendering are disabled on `main`.

### 3. Register the layer in your Application

`Application`'s constructor takes `editorMode`. Pass `false` to render the scene straight to the window; CBKEditor passes `true` to render it into a texture shown in its Viewport panel.

`pushLayer` takes a `Scope<Layer>` (`std::unique_ptr<Layer>`). Build it with the
engine's `createScope<T>()` helper. The `LayerStack` owns the layer for its lifetime.

```cpp
// MyApp.cpp
#include <Cabrankengine.h>
#include <Cabrankengine/Core/EntryPoint.h>
#include "MyLayer.h"

class MyApp : public cbk::Application {
public:
    MyApp() : cbk::Application(false) {
        // Simple case: stack takes full ownership
        pushLayer(cbk::createScope<MyLayer>());
    }
};

cbk::Application* cbk::createApplication() { return new MyApp(); }
```

If you need to call methods on the layer after pushing it, keep a raw pointer — but never delete it yourself:

```cpp
MyApp() : cbk::Application(false) {
    auto layer = cbk::createScope<MyLayer>();
    MyLayer* raw = layer.get(); // borrow for later use
    pushLayer(std::move(layer));
    // raw remains valid while the layer is on the stack
}
```

To remove a layer before shutdown call `popLayer(raw)` — the stack destroys the `unique_ptr` at that point.

---

## Loading a Scene

Scenes are JSON files; the editor uses the `.cbkscn` extension. To replace the current scene:

```cpp
Application::get().queueSceneLoad(
    cbk::scene::SceneSerializer::deserialize("scenes/my_scene.cbkscn")
);
```

The load is deferred: the new scene takes over at the start of the next frame, after the GPU has finished the current one. That way nothing the in-flight frame is still using gets destroyed.

To save the current scene:

```cpp
cbk::scene::SceneSerializer::serialize(
    Application::get().getScene(), "scenes/my_scene.cbkscn"
);
```

---

## Converting Assets

Raw mesh and image files must be converted to the engine's binary formats before use. See [asset-pipeline.md](asset-pipeline.md) for the full workflow.

Quick reference:

```bash
# Model: .obj / .fbx / .gltf / .dae  →  .cbkm
./CBKAssetConverter assets/models/my_model.obj

# Texture: .png / .jpg / .jpeg / .tga / .bmp / .hdr  →  .cbkt
./CBKAssetConverter assets/textures/albedo.png

# Cap texture size at N pixels
./CBKAssetConverter assets/textures/albedo.png --max-tex 1024
```

---

## Next Steps

- [Architecture](architecture.md) — how the ECS, renderer, layer stack and editor fit together
- [API Reference](api-reference.md) — Registry, components, and archetype builders
- [Asset Pipeline](asset-pipeline.md) — converting models and textures
- [ImGui Widgets](imgui-widgets.md) — widget cheat sheet for writing editor panels
