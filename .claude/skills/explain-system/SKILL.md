---
name: explain-system
description: Explain how a Cabrankengine subsystem actually works, read from the source rather than from docs/ — ECS, layer stack, events, renderer front end, Vulkan pipelines and materials, shaders, editor panels, scene serialization, asset formats, config, profiling. Use when asked how a system works, why it is built that way, what the alternatives were, or where to start on a feature that touches it.
---

# Explaining an engine subsystem

The goal is understanding, not a change. Produce an explanation the reader could use to modify the system themselves. That means covering the *why*, the tradeoffs, and how this engine's choice compares to how other engines solve the same problem.

## Ground rules

**Read the code, not `docs/`.** The docs lag behind the source, especially right after a refactor. Cite `file:line`. If the docs disagree with what you read, say so explicitly — that is useful signal, not a distraction.

**Do not write code unless asked.** This skill produces explanations. Illustrative excerpts *quoted from the existing source* are fine and encouraged; new implementations are not.

**History is on `legacy`.** That branch is the OpenGL/WASM-era engine, including the earlier self-recording material design. `git show legacy:<path>` helps when explaining *why* something changed, but label anything from it as history, not current behavior.

## Where things live

| Subsystem | Start here |
|---|---|
| Entry point / app lifecycle | `Cabrankengine/src/Cabrankengine/Core/EntryPoint.h`, `Core/Application.cpp` |
| Layer stack | `Core/Layer.h`, `Core/LayerStack.*`, `Application::run()` |
| Events | `Events/Event.h` (dispatcher + the `EVENT_CLASS_*` macros), `Application::onEvent` |
| ECS | `ECS/Registry.hpp`, `ECS/ComponentManager.hpp`, `ECS/SystemManager.hpp`, `ECS/Components.h`, `ECS/BuiltInSystems.*`, `ECS/Common.h` |
| Renderer front end | `Renderer/RenderLayer.cpp` (what runs each frame), `Renderer/Renderer.cpp` (per-kind draw queues), `Renderer/RenderCommand.cpp`, `Renderer/RendererAPI.h` |
| Vulkan backend | `Platform/Vulkan/VulkanRendererAPI.cpp` (frame lifecycle, `recordMaterial`), `VulkanDeviceContext.cpp`, `VulkanSwapchainManager.cpp`, `VulkanGraphicsPipeline.{h,cpp}` (descriptor-set contract) |
| Materials | `Renderer/Materials/`, `Platform/Vulkan/Vulkan*Material.cpp`, `Platform/Vulkan/Vulkan*GraphicsPipeline.cpp` |
| Shaders | `Renderer/Shader.cpp`, `Platform/Vulkan/VulkanShader.cpp`, `Platform/Vulkan/VulkanPipelineHelpers.h`, `Sandbox/assets/shaders/*.slang` |
| Editor | `CBKEditor/src/EditorApplication.cpp`, `CBKEditor/src/Panels/`, `Renderer/EditorCamera.*`, `docs/imgui-widgets.md` |
| Scene + archetypes | `Scene/Scene.*`, `Scene/Archetypes/`, `Scene/SceneSerializer.*`, `Scene/ComponentSerialization.h` |
| Math | `Common/src/Common/Math/` (custom, column-major; no glm) |
| Binary asset formats | `Common/src/Common/BinaryFormats.h`, `CBKAssetConverter/src/` |
| Config | `Cabrankengine/src/Cabrankengine/Config/Config.h` |
| Profiling | `Debug/Instrumentator.h` |
| Metal backend (broken on `main`) | `Platform/Metal/`. Explain it as it stands, and say it predates the Vulkan pipeline refactor |

## What a good explanation covers

1. **The shape** — what the system is responsible for and what it deliberately is not. Draw the data flow if it has one.
2. **The mechanism** — how it actually works, walked through real code with `file:line` anchors. Name the load-bearing details, for example:
   - `Signature` is a `std::bitset<64>` (`k_MaxComponents`).
   - Events propagate through the layer stack *in reverse*, so overlays get first refusal.
   - Component storage is keyed by `typeid(T).name()`.
   - `RenderLayer` is pushed first, so it updates before user layers.
   - Scene swaps wait for GPU idle at the top of the next frame.
3. **The tradeoffs** — this matters most. What does the design buy, and what does it cost? Some worked examples in this codebase:
   - **Dense typed component arrays keyed by `typeid`** give cache-friendly iteration but pin `k_MaxEntities`/`k_MaxComponents` at compile time and make RTTI mandatory. Archetype-based ECS (EnTT, Unity DOTS) trades lookup complexity for cheaper structural changes.
   - **Compile-time backend selection** via `CBK_RENDERER_*` keeps the hot path free of backend branching and strips unused backends. The cost: one binary cannot fall back at runtime, and "does it build everywhere?" needs a separate build per backend.
   - **Kind-switched pipelines.** `Renderer` buckets draws by `MaterialKind`, which is a counting sort: the key domain is fixed and no comparator is needed. `VulkanRendererAPI::recordMaterial` then switches on the kind. This keeps each pipeline's draws contiguous and needs no `dynamic_cast`, but the set of kinds is closed, so adding one touches the enum, the renderer API, a pipeline wrapper and a material. The previous design, still used by Metal and visible on `legacy` for Vulkan, let materials record themselves through `IVulkanRecordable`. That was open to extension, but cost a `dynamic_cast` per draw and scattered command recording across material classes.
   - **Descriptor sets grouped by update frequency** (set 0 scene per frame, set 1 per material instance, set 2 lights per frame, push constants per draw) make per-draw binding cheap, at the cost of a fixed layout contract every shader must follow.
   - **Deferred scene swap** (`queueSceneLoad` → `loadScene` after `waitIdle`) avoids destroying GPU resources the in-flight frame still references, at the cost of a frame of latency and a full GPU stall on load.
   - **Runtime shader compilation** removes a build step and lets shaders be edited without relinking, but it moves shader errors from build time to renderer init.
   - **Immediate-mode editor UI** (ImGui docking) re-submits every panel each frame, so state lives in the panels and the scene rather than in retained widget objects. It is fast to build, but layout persistence goes through `imgui.ini`, and anything stateful (selection, drag state, gizmo interaction) must be tracked by hand.
4. **How other engines do it** — brief, concrete, and only where it illuminates. The point is calibration, not a survey.
5. **Where to start** if the reader wants to extend it — the specific files and the first decision they will face.

## Graphics and GPU-API depth

When the subsystem touches the GPU, explain the technique itself, not only this engine's wiring. Examples: what a descriptor set *is* and why Vulkan groups bindings by update frequency; why dynamic rendering replaced render passes; what a pipeline barrier is protecting against; what frames-in-flight buy and what they force (per-frame copies of UBOs and descriptor sets); why MSAA needs a resolve attachment; why sRGB handling belongs at texture-view creation. Assume fluency in C++ and none in the graphics concept.

The same standard applies to editor and tooling code. For picking, gizmos, docking or undo, explain the interaction model and the math, not just the ImGui calls.

## Verifying before you explain

Grep before asserting. Constants move, and the docs lag — `k_MaxEntities` is in `ECS/Common.h`, not in the README. If something cannot be determined from the source, say so instead of filling the gap with a plausible-sounding default.
