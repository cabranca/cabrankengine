# Getting Started

This guide walks you through building Cabrankengine from source and creating your first entity.

---

## Prerequisites

| Requirement | Notes |
|-------------|-------|
| C++23 compiler | GCC 13+, Clang 17+, or MSVC 19.38+ |
| [Premake5](https://premake.github.io/) | Build file generator |
| OpenGL 4.5 | Linux / Windows. Metal on macOS (minimum macOS 12.0) |
| GNU Make | Linux builds |
| Visual Studio 2022 | Windows builds |

---

## Clone

The vendor dependencies are Git submodules. Clone with `--recurse-submodules`:

```bash
git clone --recurse-submodules https://github.com/cabranca/game-dev.git
cd game-dev
```

If you already cloned without it:

```bash
git submodule update --init --recursive
```

---

## Build (Linux)

```bash
# Generate Makefiles
premake5 gmake2

# Build everything in debug mode
make

# Or build a specific project
make config=debug Sandbox
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

## Run the Sandbox

The Sandbox project is a live example application. After building:

```bash
./bin/Debug-linux-x86_64/Sandbox/Sandbox
```

Source: [Sandbox/src/SandboxApplication.cpp](../Sandbox/src/SandboxApplication.cpp)

---

## Run the Tests

```bash
make config=debug UnitTests
./bin/Debug-linux-x86_64/UnitTests/UnitTests
```

Tests cover ECS, math types, and the collision solver. See [UnitTests/src/](../UnitTests/src/).

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
using namespace cbk;
using namespace cbk::scene::arch;

MyLayer::MyLayer() : Layer("MyLayer") {
    // Perspective camera with a first-person controller
    CameraControllerArch camera(ProjectionType::Perspective);

    // 2D sprite — path to a .cbkt texture
    SpriteArch background{ "assets/textures/background.cbkt" };
    background.transform().Scale = Vector3(800.f, 600.f, 0.f);

    // 3D model with PBR materials
    PBRModelArch gun{ "assets/models/gun/Cerberus_LP.cbkm" };
    gun.transform().Position = { 2.f, -2.f, 2.f };
    gun.transform().Scale    = Vector3(0.1f);

    // Directional light
    DirectionalLightArch sun{};
    sun.light().Direction = { 0.f, -1.f, 0.f };
    sun.light().Radiance  = { 1.f, 0.9f, 0.8f };
}

void MyLayer::onUpdate(Timestep dt) {
    // Per-frame logic goes here.
    // Built-in systems (camera, rendering) run automatically in RenderLayer.
}

void MyLayer::onImGuiRender() {
    ImGui::Begin("Debug");
    ImGui::End();
}

void MyLayer::onEvent(Event& e) {}
```

### 3. Register the layer in your Application

```cpp
// MyApp.cpp
#include <Cabrankengine.h>
#include <Cabrankengine/Core/EntryPoint.h>
#include "MyLayer.h"

class MyApp : public cbk::Application {
public:
    MyApp() { pushLayer(new MyLayer()); }
};

cbk::Application* cbk::createApplication() { return new MyApp(); }
```

---

## Loading a Scene

If you have a serialized scene file:

```cpp
Application::get().loadScene(
    cbk::scene::SceneSerializer::deserialize("assets/scenes/my_scene.json")
);
```

And to save the current scene:

```cpp
cbk::scene::SceneSerializer::serialize(
    Application::get().getScene(), "assets/scenes/my_scene.json"
);
```

---

## Converting Assets

Raw mesh and image files must be converted to the engine's binary formats before use. See [asset-pipeline.md](asset-pipeline.md) for the full workflow.

Quick reference:

```bash
# Model: .obj / .fbx / .gltf / .dae  →  .cbkm
./CBKAssetConverter assets/models/my_model.obj

# Texture: .png / .jpg / .hdr / .tga  →  .cbkt
./CBKAssetConverter assets/textures/albedo.png
```

---

## Next Steps

- [Architecture](architecture.md) — how the ECS, renderer, and layer stack fit together
- [API Reference](api-reference.md) — Registry, components, and archetype builders
- [Asset Pipeline](asset-pipeline.md) — converting models and textures
