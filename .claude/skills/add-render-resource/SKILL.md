---
name: add-render-resource
description: Add a new renderer abstraction to Cabrankengine — texture kind, buffer, geometry descriptor, or a new material kind with its own Vulkan pipeline. Use when a rendering feature needs a new engine-level type with a backend implementation, or when a new MaterialKind needs a pipeline, descriptor set and draw path.
---

# Adding a renderer resource

`main` has one working backend: **Vulkan** (Linux, Windows). **Metal** (macOS) still has branches in the factories, but it has been broken since the pipeline refactor. **OpenGL is gone** from `main` and survives only on the `legacy` branch. A new resource is threaded through these layers:

```
RenderLayer / ECS systems         gather scene data, call Renderer::submit
        ↓
Renderer + RenderCommand          Cabrankengine/src/Cabrankengine/Renderer/, backend-agnostic
        ↓
RendererAPI + abstract resources  Cabrankengine/src/Cabrankengine/Renderer/
        ↓
VulkanX   (MetalX)                Cabrankengine/src/Platform/<Backend>/
```

Backend selection is **compile-time**, not virtual dispatch at the factory level. Read `Renderer/Shader.cpp` first, since it is the cleanest factory.

## The factory pattern

The abstract type declares a static `create()`; the `.cpp` `#ifdef`s on the `CBK_RENDERER_*` defines and returns a `createRef<Concrete>`:

```cpp
Ref<Shader> Shader::create(const std::string& filepath) {
#ifdef CBK_RENDERER_METAL
    return createRef<platform::metal::MetalShader>(filepath);
#elif defined(CBK_RENDERER_VULKAN)
    return createRef<platform::vk::VulkanShader>(filepath);
#else
    CBK_CORE_ASSERT(false, "No renderer API defined!");
    return nullptr;
#endif
}
```

- The per-backend `#include`s at the top of the file are `#ifdef`-guarded too (`Shader.cpp:4-10`).
- The `#else` branch must assert rather than silently return `nullptr`.
- `FrameBuffer.cpp`, `Texture.cpp`, `GeometryDescriptor.cpp`, `GraphicsContext.cpp`, `RenderCommand.cpp`, `RendererAPI.cpp` and `Texture2DMaterial.cpp` still carry dead `CBK_RENDERER_OPENGL` branches. Don't copy them, and never add a new OpenGL branch.

## Non-material resources (textures, buffers, geometry)

1. **Declare the abstract type** in `Cabrankengine/src/Cabrankengine/Renderer/`. Use a pure-virtual interface, a static `create()`, `[[nodiscard]]` on getters, `#pragma once`, and namespace `cbk::rendering`.
2. **Implement it for Vulkan** in `Cabrankengine/src/Platform/Vulkan/VulkanX.{h,cpp}`, namespace `cbk::platform::vk`. Get the device and allocator through `VulkanRendererAPI::getContext()`.
3. **Write the `create()` dispatcher** following the pattern above. A Metal implementation is optional while Metal is broken. If you skip it, say so explicitly rather than implying Metal support.
4. **Respect GPU lifetime.** `~Application` clears the layer stack, then the scene, then calls `Renderer::shutdown()`, which destroys the VMA allocator and the device. Anything holding VMA memory must be gone before that. A `static Ref<...>` that outlives it trips VMA's "allocations were not freed" assert on exit.

## A new material kind

Vulkan draws are dispatched by `MaterialKind`, not through virtual calls or `dynamic_cast`:

- `Renderer::submit` buckets draws by `Material::getKind()` into per-kind queues (`Renderer/Renderer.cpp`).
- `VulkanRendererAPI::recordMaterial` switches on the kind and `static_cast`s to the concrete Vulkan material.
- The case then calls that kind's pipeline wrapper's typed `bind()`.

Phong is the smallest complete example. Follow its files in this order:

1. **Kind.** Add an enumerator to `common::MaterialKind` (`Common/src/Common/BinaryFormats.h:53`) and bump `k_MaterialKindCount` (line 61), because it sizes the renderer's per-kind queues. Then add the kind's JSON name to the `NLOHMANN_JSON_SERIALIZE_ENUM` in `Scene/ComponentSerialization.h:116`, which is how `CModel::Kind` is written to scene files. nlohmann maps any value missing from that list to the first entry, so a forgotten kind silently saves as `"pbr"`.
2. **Abstract material.** Create `Renderer/Materials/<Kind>Material.{h,cpp}` deriving from `Material`. Override `getKind()`, accept `.cbkm` data through `applyTexture` / `applyProperty`, and add a static `create()` (see `PhongMaterial.{h,cpp}`).
3. **Pipeline wrapper.** Create `Platform/Vulkan/Vulkan<Kind>GraphicsPipeline.{h,cpp}` modeled on `VulkanPhongGraphicsPipeline`. It holds a `VulkanGraphicsPipeline` member and supplies only what differs per kind:
   - `k_MaterialBindingCount`: the number of set-1 textures, created by `createSetLayoutBindings` in `VulkanPipelineHelpers.h` as fragment-stage combined image samplers.
   - `k_MaxInstances`: the descriptor pool capacity, i.e. how many material instances of this kind can exist at once.
   - `PushData`: mirrors the shader's push-constant block, with a `static_assert` on its size.
   - `createShaderModule("<Kind>")`: loads `assets/shaders/<Kind>.slang`.
   - A typed `bind(cb, frameIndex, materialSet, ...)` that packs `PushData` and forwards to the inner pipeline.

   Phong and PBR currently duplicate this code, and a TODO notes the missing base class. Ask before refactoring it as a side effect.
4. **Renderer API wiring** in `VulkanRendererAPI.{h,cpp}`:
   - Add an `m_<Kind>GraphicsPipeline` member plus an `s_<Kind>GraphicsPipeline` static pointer, the same member/pointer split the existing two use.
   - `init()` it after `VulkanGraphicsPipeline::initSceneResources()`, and shut it down before `shutdownSceneResources()`.
   - Add a static `allocate<Kind>DescriptorSet()`.
   - Add a `case` in `recordMaterial`.
5. **Concrete material.** Create `Platform/Vulkan/Vulkan<Kind>Material.{h,cpp}`. Its constructor allocates the instance's own set 1 through `VulkanRendererAPI::allocate<Kind>DescriptorSet()`. Its `updateDescriptorSet()` writes the texture image infos once, after every slot is populated (see `VulkanPhongMaterial.cpp`).
6. **Shader.** Write `<Kind>.slang` against the set 0 / 1 / 2 and push-constant contract, following the `add-shader` skill.
7. **User entry point.** Add an archetype in `Scene/Archetypes/<Kind>ModelArch.{h,cpp}` that calls `<Kind>Material::create()` and tags `CModel::Kind` (see `PhongModelArch.cpp`).

Metal still uses the older self-recording design, where `MetalRendererAPI.cpp:168` `dynamic_cast`s materials to `IMetalRecordable`. Don't mirror that on Vulkan, and don't port Metal as part of this change unless asked.

## Conventions that apply here specifically

- `#include <pch.h>` as the literal first line of every `.cpp` under `Cabrankengine/`. The exception is `.mm` files: premake disables PCH for them (`filter("files:**.mm") enablepch("off")`), so they must not include it.
- `Ref<T>`/`createRef`, `Scope<T>`/`createScope` from `Core/Core.h:61-73`. Never `std::make_shared`.
- `VK_CHECK(...)` around every Vulkan call that returns a `VkResult`. It logs `CBK_CORE_FATAL` and aborts.
- `CBK_CORE_ASSERT` is **debug-only** and compiles to nothing in Release. Never put a resource-creation call or any other side effect inside one.
- Members `m_PascalCase`, statics `s_PascalCase`, constants `k_PascalCase`, methods `camelCase`.
- **No premake edits for new sources.** The project files glob `src/Cabrankengine/**` and `src/Platform/<Backend>/**`. A new *module* would need its own `premake5.lua` and an `include` in the root one, which is a much bigger change, so confirm before doing it.

## Verifying

There is no renderer test target. Verification means building and running the apps that exercise the renderer:

```bash
./premake5 gmake                      # needs VULKAN_SDK set
make config=debug -j$(nproc)
cd bin/Debug-linux-x86_64/Sandbox   && ./Sandbox --log-level=debug
cd bin/Debug-linux-x86_64/CBKEditor && ./CBKEditor --log-level=debug
```

The two apps cover different paths. Sandbox resolves straight into the swapchain, and CBKEditor renders into a texture (`RendererSpec::RenderSceneToTexture`). Run both for anything that touches frame recording, attachments or formats.

Metal cannot be verified here and is broken on `main` regardless. Say the Metal path is unverified rather than reporting it as done.
