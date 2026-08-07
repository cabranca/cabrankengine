# Architecture

Cabrankengine is a C++23 game engine built around an Entity-Component-System (ECS) core, a layered application loop, and an abstract renderer with Vulkan, OpenGL 4.5, and Metal backends selected at compile time. It also compiles to WebAssembly via Emscripten to run in the browser.

---

## Module Layout

```
Cabrankengine/src/Cabrankengine/
├── Core/           Application loop, LayerStack, Input, Window, Audio
├── ECS/            Registry, EntityManager, ComponentManager, SystemManager
├── Renderer/       RendererAPI abstraction, Renderer2D, Materials (Phong/PBR)
├── Scene/          Camera, Model loading, SceneSerializer, Transform
│   └── Archetypes/ Entity builder templates (Sprite, PhongModel, PBRModel, Text, Camera, Lights)
├── Events/         Event types and dispatcher
├── ImGui/          ImGui layer
├── Debug/          Profiling instrumentation
└── Config/         Compile-time configuration

Common/src/Common/          shared by the engine AND CBKAssetConverter
├── Math/           Vector2/3/4, Mat4, Quaternion, MatrixFactory (column-major)
├── Logger.h        spdlog wrapper (CBK_CORE_*, CBK_APP_*, CBK_AC_* macros)
├── Assertion.h     CBK_CORE_ASSERT / CBK_APP_ASSERT / CBK_AC_ASSERT
└── BinaryFormats.h Shared structs for .cbkm / .cbkt binary files

Platform/           OS-specific and backend-specific implementations
├── Linux/          LinuxWindow, LinuxInput
├── Windows/        WindowsWindow, WindowsInput
├── MacOS/          MacOSWindow
├── Vulkan/         VulkanRendererAPI, VulkanShader, Vulkan*Material, ...
├── OpenGL/         OpenGLRendererAPI, OpenGLShader, OpenGLTexture, ...
└── Metal/          MetalRendererAPI, MetalShader, ...
```

---

## Application & Layer Stack

The entry point is `cbk::createApplication()`, defined by the game. Calling `Application::run()` starts the frame loop.

```
Application::run()
  └─> LayerStack::onUpdate(dt)
      ├─> Layer 0 ::onUpdate()      ← game logic, entity mutations
      ├─> Layer 1 ::onUpdate()
      └─> RenderLayer (built-in)
          ├─> CameraSystem::update()
          ├─> SpriteRenderSystem::update()
          ├─> PhongRenderSystem::update()
          ├─> PBRRenderSystem::update()
          └─> TextRenderSystem::update()

  └─> LayerStack::onImGuiRender()   ← overlays, debug panels

  └─> Window::onUpdate()            ← swap buffers, poll events
```

**Layers** are the primary extension point. Push them in your `Application` constructor via `pushLayer()`. Overlays (pushed via `pushOverlay()`) sit above all layers and receive events first.

Each layer has four callbacks:

| Callback | Purpose |
|----------|---------|
| `onAttach()` | One-time setup — create entities, load assets |
| `onDetach()` | Cleanup |
| `onUpdate(Timestep)` | Per-frame game logic |
| `onImGuiRender()` | ImGui panels |
| `onEvent(Event&)` | Input and window events |

---

## ECS Core

The ECS is the primary data model. All game objects are entities; all state lives in components; all logic runs in systems.

### Concepts

```
Entity    uint32_t handle, max 20 000 concurrent (k_MaxEntities in ECS/Common.h),
          IDs recycled via queue
Signature std::bitset<64> — one bit per registered component type
          (k_MaxComponents = 64)
```

### Three Managers

```
EntityManager     creates/destroys entity IDs, tracks each entity's Signature
ComponentManager  stores components in dense typed arrays (ComponentArray<T>)
                  maps entity ↔ dense index for cache-friendly iteration
SystemManager     holds registered systems; updates each system's entity set
                  when an entity's Signature changes
```

The **Registry** owns all three and is the only public API consumers touch:

```
Registry
  createEntity()           → Entity
  destroyEntity(Entity)
  addComponent<T>(Entity, T)
  removeComponent<T>(Entity)
  getComponent<T>(Entity)  → std::optional<T*>
  registerSystem<T>()      → std::shared_ptr<T>
  getSystem<T>()           → std::shared_ptr<T>
  setSystemSignature<T>(Signature)
  rebuildSystemMembership()
```

### System Membership

When a component is added or removed, `SystemManager::entitySignatureChanged` runs automatically. If the entity's new signature satisfies a system's required signature, the entity is added to that system's entity set. It is removed when it no longer satisfies the signature.

### Design Pattern: Systems Share Data Through Components

Systems do not reference each other. When two systems need to share state, they both declare the relevant component in their signature. The canonical example is the camera:

```
CameraControllerSystem  signature: {CTransform, CCameraController}
  Reads input → writes new Position/Rotation into CTransform

CameraSystem            signature: {CTransform, CCamera}
  Reads CTransform → builds the view-projection matrix
```

An entity that has all three components is a player-controlled camera. An entity with only `CTransform + CCamera` is a static or scripted camera the controller never touches. No explicit link between systems — `CTransform` is the contract.

---

## Scene

The `Scene` class wraps a `Registry` and is the stable handle used across the application. Access it via:

```cpp
cbk::Scene& scene = Application::get().getScene();
cbk::ecs::Registry* reg = Application::get().getRegistry();
```

`SceneSerializer` reads/writes scenes as JSON, round-tripping entity names and all standard components.

---

## Archetypes

`Scene/Archetypes/Archetypes.h` provides convenience builders that create an entity and attach standard components in one call. Prefer these over manual `createEntity` + `addComponent` sequences.

```cpp
SpriteArch box{ "assets/textures/crate.cbkt" };
box.transform().Scale = Vector3(100.f, 100.f, 0.f);
box.sprite().Tint     = { 1.f, 0.f, 0.f, 1.f };

PBRModelArch gun{ "assets/models/gun.cbkm" };
gun.transform().Position = { 0.f, 0.f, -5.f };

CameraControllerArch camera(ProjectionType::Perspective);

DirectionalLightArch sun{};
sun.light().Direction = { 0.f, -1.f, 0.f };
```

---

## Renderer

The renderer is split into three layers:

```
Renderer2D / Renderer3D   high-level submission API (batch quads, submit meshes)
     │
RendererAPI               abstract interface (draw calls, clear, state)
     │
VulkanRendererAPI         or OpenGLRendererAPI or MetalRendererAPI — selected at compile time
```

**Renderer2D** batches all sprite quads in a single vertex buffer and flushes once per frame (or when the batch is full). Submit calls are cheap.

**Renderer3D** dispatches to `PhongRenderSystem` or `PBRRenderSystem` depending on the component present on the entity.

The backend is selected via Premake. On Linux the default is **Vulkan** (pass `--renderer=opengl` for the OpenGL 4.5 fallback); on Windows the default is **OpenGL** (pass `--renderer=vulkan` to opt in). Metal targets macOS. See [getting-started.md](getting-started.md) for the build flags.

### Vulkan backend: self-recording materials

The Vulkan backend uses dynamic rendering (no `VkRenderPass`/framebuffer objects). Each concrete material owns its pipeline, descriptor-set layout and pool as shared per-class state, and implements `IVulkanRecordable::record(cb, transform)` to bind everything it needs for a draw: its pipeline, the scene-globals descriptor set (set 0), its own material set (set 1), the point-light SSBO (set 2, lit materials only) and any push constants. `VulkanRendererAPI` only owns the frame lifecycle (acquire, command-buffer begin/end, barriers, submit, present) and delegates per-draw recording to the material. The descriptor-set index convention lives in one place — `Platform/Vulkan/VulkanDescriptorBinding.h` — so it is not duplicated across materials.

---

## Renderer System Execution Order

Within `RenderLayer::onUpdate`, built-in systems run in this fixed order:

1. `CameraSystem` — computes the view-projection matrix from `CTransform + CCamera`
2. `CameraControllerSystem` — applies input to `CTransform`
3. `SpriteRenderSystem` — batches and flushes all 2D sprites
4. `PhongRenderSystem` — draws Phong-shaded 3D models
5. `PBRRenderSystem` — draws PBR-shaded 3D models
6. `TextRenderSystem` — draws text

Custom systems added by game layers run in their layer's `onUpdate`, which runs **before** `RenderLayer`. Write component state there; read it in the built-in render systems.

---

## Events

Events flow from the Window through the Application to the LayerStack, propagating top-to-bottom (overlays first). A layer calls `e.setHandled()` to stop propagation — `m_Handled` is private, and `handled()` reads it. Returning `true` from a dispatched callback sets it for you.

Event categories: Application (WindowResize, WindowClose), Key, Mouse, MouseButton.

Dispatch pattern:

```cpp
void MyLayer::onEvent(Event& e) {
    EventDispatcher dispatcher(e);
    dispatcher.dispatch<KeyPressedEvent>([this](KeyPressedEvent& e) {
        // handle
        return true; // mark as handled
    });
}
```

---

## Naming Conventions

| Pattern | Example |
|---------|---------|
| Namespace | `cbk::`, `cbk::ecs`, `cbk::rendering`, `cbk::scene`, `cbk::math` |
| Component | `C` prefix — `CTransform`, `CSprite`, `CCamera` |
| System | `System` suffix — `CameraSystem`, `SpriteRenderSystem` |
| Private member | `m_` prefix — `m_Registry`, `m_LayerStack` |
| Static member | `s_` prefix — `s_Instance`, `s_Shaders` |
| Constant | `k_` prefix — `k_MaxFramesInFlight` |
| Public POD member | PascalCase, no prefix — `CTransform::Position` |
| Method | `camelCase` — `createEntity`, `pushLayer` |
| Smart pointer aliases | `Ref<T>` = `std::shared_ptr<T>`, `Scope<T>` = `std::unique_ptr<T>` — use `createRef` / `createScope` |
| Logging | `CBK_CORE_*` (engine), `CBK_APP_*` (game), `CBK_AC_*` (converter) — e.g. `CBK_CORE_INFO(...)`, `CBK_APP_WARN(...)` |
| Assertion | `CBK_CORE_ASSERT(cond, msg)` (debug-only, triggers breakpoint) |
