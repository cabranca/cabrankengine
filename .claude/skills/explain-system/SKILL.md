---
name: explain-system
description: Explain how a Cabrankengine subsystem actually works, read from the source rather than from docs/ — ECS, layer stack, events, renderer, materials, scene serialization, asset formats, config, profiling, WASM bindings. Use when asked how a system works, why it is built that way, what the alternatives were, or where to start on a feature that touches it.
---

# Explaining an engine subsystem

The goal is understanding, not a change. Produce an explanation the reader could use to modify the system themselves — which means covering the *why*, the tradeoffs, and how this engine's choice compares to how other engines solve the same problem.

## Ground rules

**Read the code, not `docs/`.** The docs are partly aspirational and are known to contradict the source: entity limits, `Application::run()` vs the real `Run()`, a `Cabrankengine/src/Cabrankengine/Math/` directory that does not exist (math lives in `Common/src/Common/Math/`), and `CPhongModel`/`CPBRModel` vs the single real `CModel`. Cite `file:line`. If the docs disagree with what you read, say so explicitly — that is useful signal, not a distraction.

**Do not write code unless asked.** This skill produces explanations. Illustrative excerpts *quoted from the existing source* are fine and encouraged; new implementations are not.

## Where things live

| Subsystem | Start here |
|---|---|
| Entry point / app lifecycle | `Cabrankengine/src/Cabrankengine/Core/EntryPoint.h`, `Core/Application.cpp` |
| Layer stack | `Core/Layer.h`, `Core/LayerStack.*`, `Application::Run()` |
| Events | `Events/Event.h` (dispatcher + the `EVENT_CLASS_*` macros), `Application::OnEvent` |
| ECS | `ECS/Registry.hpp`, `ECS/ComponentManager.hpp`, `ECS/SystemManager.hpp`, `ECS/Components.h`, `ECS/Common.h` |
| Renderer abstraction | `Renderer/RendererAPI.h`, `Renderer/Shader.cpp` (the factory pattern), `Platform/<Backend>/` |
| Materials | `Renderer/Materials/`, plus `Platform/Vulkan/Vulkan*Material.cpp` and `Platform/Metal/Metal*Material.cpp` |
| Scene + archetypes | `Scene/Scene.*`, `Scene/Archetypes/`, `Scene/DefaultLibrary.cpp`, `Scene/SceneSerializer.*` |
| Math | `Common/src/Common/Math/` (custom, column-major — glm is a submodule but the engine does not use it for its own types) |
| Binary asset formats | `Common/src/Common/BinaryFormats.h`, `CBKAssetConverter/src/` |
| Config | `Cabrankengine/src/Cabrankengine/Config/Config.h` |
| Profiling | `Debug/Instrumentator.h` |
| WASM bindings | `CBKBindings/` (embind, `Makefile.emscripten`) |

## What a good explanation covers

1. **The shape** — what the system is responsible for and what it deliberately is not. Draw the data flow if it has one.
2. **The mechanism** — how it actually works, walked through real code with `file:line` anchors. Name the load-bearing details: `Signature` is a `std::bitset<64>`, events propagate through the layer stack *in reverse* so overlays get first refusal, component storage is keyed by `typeid(T).name()`.
3. **The tradeoffs** — this matters most. What does the design buy, and what does it cost? Some worked examples in this codebase:
   - Dense typed component arrays keyed by `typeid` give cache-friendly iteration but pin `MAX_ENTITIES`/`MAX_COMPONENTS` at compile time and make RTTI mandatory. Archetype-based ECS (EnTT, Unity DOTS) trades lookup complexity for cheaper structural changes.
   - Compile-time backend selection via `CBK_RENDERER_*` keeps the hot path free of virtual dispatch and dead-strips unused backends, but means one binary cannot fall back at runtime and a "does it build on all three?" check needs three separate builds.
   - Self-recording materials (`IVulkanRecordable`) keep pipeline state next to the thing that owns it, at the cost of a `dynamic_cast` per draw and of `RendererAPI` no longer being the single place to reason about command recording.
   - Runtime shader compilation removes a build step and lets shaders be edited without relinking, but pushes shader errors from build time to frame one, and a missing per-backend variant fails on exactly one platform.
4. **How other engines do it** — brief, concrete, and only where it illuminates. The point is calibration, not a survey.
5. **Where to start** if the reader wants to extend it — the specific files and the first decision they will face.

## Graphics and GPU-API depth

When the subsystem touches the GPU, explain the technique itself, not only this engine's wiring: what a descriptor set *is* and why Vulkan groups bindings by update frequency; why dynamic rendering replaced render passes; what a pipeline barrier is protecting against; why sRGB handling belongs at texture-view creation. Assume fluency in C++ and none in the graphics concept.

## Verifying before you explain

Grep before asserting. Constants move, and the docs lag — `MAX_ENTITIES` is in `ECS/Common.h`, not in the README. If something cannot be determined from the source, say so instead of filling the gap with a plausible-sounding default.
