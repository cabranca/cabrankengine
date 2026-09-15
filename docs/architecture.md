# Architecture

Cabrankengine is a C++23 game engine built around an Entity-Component-System (ECS) core, a layered application loop, and an abstract renderer whose backend is selected at compile time. **Vulkan** (Linux, Windows) is the working backend. **Metal** (macOS) exists but has been broken on `main` since the renderer refactor. **CBKEditor**, an ImGui-based editor, is built on top of the engine.

> OpenGL, OpenGL ES and the WebAssembly target were removed from `main`. They live on the `legacy` branch.

---

## Repository Layout

```
Cabrankengine/      the engine (static library)
CBKEditor/          editor application
Sandbox/            example application
CBKAssetConverter/  CLI that converts models and images to .cbkm / .cbkt
Common/             code shared by the engine and the converter
UnitTests/          Catch2 tests
```

## Module Layout

```
Cabrankengine/src/Cabrankengine/
├── Core/           Application loop, LayerStack, Input, Window, Audio (stub)
├── ECS/            Registry, EntityManager, ComponentManager, SystemManager, built-in components and systems
├── Renderer/       Renderer (per-material-kind draw queues), RenderCommand, RendererAPI, RenderLayer,
│                   EditorCamera, Shader, Texture, Materials/ (Phong, PBR, Texture2D, Text),
│                   Renderer2D and TextRenderer (disabled on main)
├── Scene/          Scene, Model, Mesh, SceneSerializer, ComponentSerialization, DefaultLibrary (disabled on main)
│   └── Archetypes/ Entity builders (PhongModel, PBRModel, Camera, CameraController, DirectionalLight, PointLight, Sprite, Text)
├── Events/         Event types and dispatcher
├── ImGui/          ImGui layer
├── Debug/          Profiling instrumentation
└── Config/         Runtime config.json loading (window title and size)

Cabrankengine/src/Platform/
├── Linux/          LinuxWindow, LinuxInput
├── Windows/        WindowsWindow, WindowsInput
├── MacOS/          MacOSWindow, MacOSInput
├── Vulkan/         VulkanRendererAPI, VulkanDeviceContext, VulkanSwapchainManager,
│                   VulkanGraphicsPipeline, Vulkan*GraphicsPipeline, Vulkan*Material, VulkanShader, ...
└── Metal/          MetalRendererAPI, MetalShader, Metal*Material, ... (broken on main)

Common/src/Common/  shared by the engine AND CBKAssetConverter
├── Math/           Vector2/3/4, Mat4, Quaternion, MatrixFactory (column-major)
├── Logger.h        spdlog wrapper (CBK_CORE_*, CBK_APP_*, CBK_AC_* macros)
├── Assertion.h     CBK_CORE_ASSERT / CBK_APP_ASSERT / CBK_AC_ASSERT
└── BinaryFormats.h Shared structs for .cbkm / .cbkt binary files, and MaterialKind

CBKEditor/src/
├── EditorApplication.cpp  EditorLayer: dockspace, menu bar, owns the panels
└── Panels/                Panel base class, OutlinerPanel, DetailsPanel, ViewportPanel
```

---

## Application & Layer Stack

The application defines `cbk::createApplication()`. `Core/EntryPoint.h` provides `main()`, which parses `--log-level`, creates the application and calls `Application::run()`.

The `Application(bool editorMode)` constructor loads `config.json`, creates the window, and initializes the renderer, with `editorMode` becoming `RendererSpec::RenderSceneToTexture`. It then pushes `RenderLayer` as the **first** layer and `ImGuiLayer` as an overlay. Layers your application pushes in its own constructor land after `RenderLayer`.

```
Application::run()   (once per frame)
  ├─> loadScene()                    only if queueSceneLoad() was called: waits for GPU idle, swaps the scene
  ├─> RenderCommand::beginFrame()
  ├─> Layer::onUpdate(dt), in push order
  │     ├─> RenderLayer                cameras, lights, model submission (see below)
  │     └─> your layers                game logic, entity mutations
  ├─> RenderCommand::endScenePass()
  ├─> ImGuiLayer::begin()
  ├─> Layer::onImGuiRender(), in push order    editor panels, debug UI
  ├─> ImGuiLayer::end()
  ├─> RenderCommand::endFrame()      submit and present, exactly once per frame
  └─> Window::onUpdate()             poll events
```

While the window is minimized, the scene-load check, `beginFrame` and `onUpdate` are skipped.

Because `RenderLayer` is pushed first, it updates **before** your layers, so entity changes made in a layer's `onUpdate` are drawn on the next frame.

**Layers** are the primary extension point. Push them in your `Application` constructor via `pushLayer()`. Overlays (pushed via `pushOverlay()`) sit above all layers and receive events first.

Each layer has five callbacks:

| Callback | Purpose |
|----------|---------|
| `onAttach()` | One-time setup: create entities, load assets |
| `onDetach()` | Cleanup |
| `onUpdate(Timestep)` | Per-frame game logic |
| `onImGuiRender()` | ImGui panels |
| `onEvent(Event&)` | Input and window events |

**Shutdown order** is explicit in `~Application`: it clears the layer stack, then the scene, then calls `Renderer::shutdown()`, which destroys the VMA allocator and the device. Anything that owns GPU memory must be released before that last step.

---

## ECS Core

The ECS is the primary data model. All game objects are entities; all state lives in components; all logic runs in systems.

### Concepts

```
Entity    uint32_t handle, max 20 000 concurrent (k_MaxEntities in ECS/Common.h),
          IDs recycled via queue
Signature std::bitset<64>, one bit per registered component type
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

Component and system storage is keyed by `typeid(T).name()`, so the engine requires RTTI.

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

An entity that has all three components is a player-controlled camera. An entity with only `CTransform + CCamera` is a static or scripted camera the controller never touches. There's no explicit link between the systems; `CTransform` is the contract.

---

## Scene

`scene::Scene` wraps a `Registry` together with entity names and `SceneMetadata` (name, background color, ambient color). Access it via:

```cpp
cbk::scene::Scene& scene = Application::get().getScene();
cbk::ecs::Registry* reg  = Application::get().getRegistry();
```

`SceneSerializer` reads and writes scenes as JSON (the editor uses the `.cbkscn` extension), round-tripping entity names and all standard components.

Replacing the scene is **deferred**:

```cpp
Application::get().queueSceneLoad(SceneSerializer::deserialize("scenes/level.cbkscn"));
```

The swap happens at the start of the next frame, after the GPU is idle, so nothing the in-flight frame still references is destroyed mid-recording. The built-in systems are then reset against the new registry.

---

## Archetypes

`Scene/Archetypes/Archetypes.h` provides convenience builders that create an entity and attach standard components in one call. Prefer these over manual `createEntity` + `addComponent` sequences.

```cpp
PhongModelArch backpack{ "assets/models/backpack/backpack.cbkm" };
backpack.transform().Position = { -2.f, 0.f, -5.f };

PBRModelArch gun{ "assets/models/gun/Cerberus_LP.cbkm" };
gun.transform().Scale = Vector3(0.05f);

CameraControllerArch camera(ProjectionType::Perspective);

DirectionalLightArch sun{};
sun.light().Direction = { 1.f, -1.f, -1.f };
```

`SpriteArch` and `TextArch` still exist, but their render paths are disabled on `main`.

---

## Renderer

```
RenderLayer          gathers camera and lights, runs ModelRenderSystem
     │
Renderer             beginScene / submit / endScene; buckets draws by MaterialKind
     │
RenderCommand        static facade over the active backend
     │
RendererAPI          abstract interface: frame lifecycle, drawIndexed, getFinalFrame
     │
VulkanRendererAPI    (Linux, Windows)    |    MetalRendererAPI (macOS, broken on main)
```

**Backend selection** happens at compile time. Premake defines `CBK_RENDERER_VULKAN` on Linux and Windows, and `Core/Core.h` forces `CBK_RENDERER_METAL` on macOS. Every abstract resource type has a static `create()` that `#ifdef`s on those defines.

**Submission.** `ModelRenderSystem` calls `Renderer::submit(material, geometry, transform)` for every mesh. `Renderer` keeps one queue per `common::MaterialKind`, which makes submission a counting sort: the key domain is fixed, so there's no comparator, and draws of the same kind stay in submission order. `endScene()` drains the queues kind by kind into `RenderCommand::drawIndexed`.

**Render-to-texture.** With `RendererSpec::RenderSceneToTexture` set (CBKEditor), the scene resolves into an offscreen attachment, and `RenderCommand::getFinalFrame()` returns an ImGui texture handle for it, which the Viewport panel draws. Without it (Sandbox), the scene resolves straight into the swapchain image and ImGui composites on top.

**Shaders** compile at runtime. `VulkanShader` compiles a single `.slang` file (both stages) to SPIR-V 1.4 with Slang when the pipeline is created.

### Vulkan backend

- Requires Vulkan **1.4** with dynamic rendering, synchronization2 and buffer device address. It uses no `VkRenderPass` or framebuffer objects.
- `VulkanRendererAPI` owns the device context, the swapchain manager, the per-frame command buffers and fences (`k_MaxFramesInFlight` = 2), and one pipeline per material kind.
- **One pipeline per material kind.** `VulkanPhongGraphicsPipeline` and `VulkanPBRGraphicsPipeline` each wrap a `VulkanGraphicsPipeline` and supply only what differs between kinds: the number of texture bindings in set 1, the push-constant struct, and the shader name.
- **Draw recording.** `VulkanRendererAPI::recordMaterial` switches on `Material::getKind()`, `static_cast`s to the concrete Vulkan material, lets it write its descriptor set if it hasn't yet, and binds that kind's pipeline, descriptor sets and push constants. `bindAndDraw` then issues the draw. The kind check already answered the type question, so there's no `dynamic_cast`.

Descriptor sets are grouped by how often they change:

| Set | Contents | Owned by | Written |
|-----|----------|----------|---------|
| 0 | Scene UBO: view-projection, camera position, directional light | shared static in `VulkanGraphicsPipeline` | once per frame |
| 1 | Material textures (combined image samplers) | each material instance, allocated from its kind's pipeline pool | once, when all textures are present |
| 2 | Point-light SSBO (max 10 lights) | shared static in `VulkanGraphicsPipeline` | once per frame |
| push constants | Model transform and material scalars | — | every draw |

`Platform/Vulkan/VulkanGraphicsPipeline.h` is the authoritative description. Its `UBOData` is `static_assert`ed against the std140 layout in `Phong.slang`.

---

## Renderer System Execution Order

Within `RenderLayer::onUpdate`:

1. **Camera.** In editor mode, `EditorCamera::update()`. Otherwise `CameraControllerSystem` (input → `CTransform`), then `CameraSystem` (`CTransform` → view-projection). `RenderLayer::setEditorMode()` selects the mode, but nothing calls it yet.
2. **Lights.** The first `CDirectionalLight` entity and every `CPointLight` entity are collected into a `LightEnvironment`.
3. **Models.** `Renderer::beginScene(SceneData)`, then `ModelRenderSystem` submits every `CModel`, then `Renderer::endScene()` records the draws.

`SpriteRenderSystem` and `TextRenderSystem` are still registered, but their calls are commented out on `main`.

---

## Editor

CBKEditor is an `Application` constructed with `editorMode = true`, plus a single `EditorLayer`.

- **Dockspace.** `EditorLayer` submits a full-window host window with a dockspace and a menu bar (*Scene*: New / Save / Save as / Load; *Window*: Reset Layout). With no `imgui.ini`, or after a reset, it builds the default layout with `DockBuilder`: Outliner on the left, Details on the right, Viewport in the centre.
- **Panels** derive from `Panels/Panel.h`. They open their window with `Panel::begin()` / `end()` rather than `ImGui::Begin` / `End`, which lets the base class pull a panel dropped on empty space back into the dockspace after a few settled frames. Each panel implements `onImGuiRender()` and `reset()`. `reset()` runs whenever the scene is replaced, so panels drop entity handles from the old registry.
- **Selection** flows from the Outliner to the Details panel through a callback registered with `OutlinerPanel::setSelectedEntityCallBack`, not through shared global state.
- **Viewport** draws `RenderCommand::getFinalFrame()` letterboxed to 16:9. It records its focus, hover state and size for future camera-input routing and render-target resizing.
- **File dialogs** for Save as / Load shell out to `zenity`, so they work on Linux only.

---

## Events

Events flow from the Window through the Application to the LayerStack, propagating top-to-bottom (overlays first). A layer calls `e.setHandled()` to stop propagation. `m_Handled` is private and `handled()` reads it. Returning `true` from a dispatched callback sets it for you.

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
| Namespace | `cbk::`, `cbk::ecs`, `cbk::rendering`, `cbk::scene`, `cbk::math`, `cbk::editor` |
| Component | `C` prefix: `CTransform`, `CModel`, `CCamera` |
| System | `System` suffix: `CameraSystem`, `ModelRenderSystem` |
| Private member | `m_` prefix: `m_Registry`, `m_LayerStack` |
| Static member | `s_` prefix: `s_Instance`, `s_Shaders` |
| Constant | `k_` prefix: `k_MaxFramesInFlight` |
| Public POD member | PascalCase, no prefix: `CTransform::Position` |
| Method | `camelCase`: `createEntity`, `pushLayer` |
| Smart pointer aliases | `Ref<T>` = `std::shared_ptr<T>`, `Scope<T>` = `std::unique_ptr<T>`; use `createRef` / `createScope` |
| Logging | `CBK_CORE_*` (engine), `CBK_APP_*` (game), `CBK_AC_*` (converter), e.g. `CBK_CORE_INFO(...)`, `CBK_APP_WARN(...)` |
| Assertion | `CBK_CORE_ASSERT(cond, msg)` (debug-only, triggers breakpoint) |
