---
name: add-render-resource
description: Add a new renderer abstraction — texture kind, buffer, material, geometry descriptor — across the Vulkan, OpenGL and Metal backends of Cabrankengine. Use when a rendering feature needs a new engine-level type with per-backend implementations, or when an existing abstraction is missing one backend.
---

# Adding a renderer resource

The renderer is three layers deep, and a new resource has to be threaded through all of them:

```
Renderer2D / Renderer3D          submission API, backend-agnostic
        ↓
RendererAPI + abstract resources  Cabrankengine/src/Cabrankengine/Renderer/
        ↓
VulkanX | OpenGLX | MetalX        Cabrankengine/src/Platform/<Backend>/
```

Backend selection is **compile-time**, not virtual dispatch at the factory level. Read `Renderer/Shader.cpp:18-42` first — it is the canonical shape and every other resource copies it.

## The factory pattern

The abstract type declares a static `create()`; the `.cpp` `#ifdef`s on the `CBK_RENDERER_*` defines and returns a `createRef<Concrete>`:

```cpp
Ref<Shader> Shader::create(const std::string& filepath) {
#ifdef CBK_RENDERER_OPENGL
    return createRef<platform::opengl::OpenGLShader>(filepath);
#elif defined(CBK_RENDERER_METAL)
    return createRef<platform::metal::MetalShader>(filepath);
#elif defined(CBK_RENDERER_VULKAN)
    return createRef<platform::vk::VulkanShader>(filepath);
#else
    CBK_CORE_ASSERT(false, "No renderer API defined!");
    return nullptr;
#endif
}
```

Two things are easy to miss: the per-backend `#include`s at the top of the file are themselves `#ifdef`-guarded (`Shader.cpp:4-14`), and the `#else` branch must assert rather than silently return `nullptr`.

## Steps

1. **Declare the abstract type** in `Cabrankengine/src/Cabrankengine/Renderer/` (or `Renderer/Materials/` for a material). Pure-virtual interface, static `create()`, `[[nodiscard]]` on getters, `#pragma once`, namespace `cbk::rendering`.

2. **Implement per backend** in `Cabrankengine/src/Platform/Vulkan/`, `.../OpenGL/`, `.../Metal/`, in namespaces `cbk::platform::vk`, `cbk::platform::opengl`, `cbk::platform::metal`. Filenames mirror the abstract type with the backend as prefix (`VulkanShader`, `OpenGLShader`, `MetalShader`).

3. **Write the `create()` dispatcher** in the abstract type's `.cpp`, following the pattern above verbatim.

4. **Materials additionally implement a recordable interface.** On Vulkan and Metal the backend `RendererAPI` owns only the frame lifecycle — acquire, command-buffer begin/end, barriers, submit, present — and `dynamic_cast`s the material to `IVulkanRecordable` / `IMetalRecordable` to delegate per-draw recording (`Platform/Vulkan/VulkanRendererAPI.cpp:272`, `Platform/Metal/MetalRendererAPI.cpp:154`). This is why the engine must keep RTTI enabled. Each concrete material owns its pipeline and descriptor-set layout/pool as shared per-class state.

5. **Vulkan descriptor set indices are centralized** in `Platform/Vulkan/VulkanDescriptorBinding.h` — set 0 scene globals, set 1 material, set 2 point-light SSBO. Add to that header; never hard-code an index at the use site.

6. **No premake edits needed for new sources.** The project files glob `src/**`, so a new `.cpp`/`.h` under an existing module is picked up automatically. A new *module* would need its own `premake5.lua` and a `include` in the root one — that is a much bigger change, so confirm before doing it.

7. **macOS `.mm` files are special-cased.** Objective-C++ sources have PCH disabled (`filter("files:**.mm") enablepch("off")` in `Cabrankengine/premake5.lua`), so they must not `#include <pch.h>`.

## Conventions that apply here specifically

- `#include <pch.h>` as the literal first line of every `.cpp` under `Cabrankengine/` — except `.mm` files, per above.
- `Ref<T>`/`createRef`, `Scope<T>`/`createScope` from `Core/Core.h:56-70`. Never `std::make_shared`.
- `VK_CHECK(...)` around every Vulkan call that returns a `VkResult`. It logs `CBK_CORE_FATAL` and aborts.
- `CBK_CORE_ASSERT` is **debug-only** and compiles to nothing in Release — never put a resource-creation call or any other side effect inside one.
- Members `m_PascalCase`, statics `s_PascalCase`, constants `k_PascalCase`, methods `camelCase`.

## Verifying

There is no renderer test target — verification is building and running the Sandbox on each backend you claimed to support:

```bash
./premake5 gmake                     # Vulkan on Linux; needs VULKAN_SDK set
make config=debug -j$(nproc)
cd bin/Debug-linux-x86_64/Sandbox && ./Sandbox

rm -rf bin bin-int                   # switching backends: wipe, do not `make clean`
./premake5 gmake --renderer=opengl
make config=debug -j$(nproc)
```

`make clean` is **not** sufficient when switching backends: it leaves the previous backend's objects inside the static libs, and the link fails with undefined references to that backend's SDK (e.g. `slang_createGlobalSession2` when going Vulkan → OpenGL). Only `rm -rf bin bin-int` clears it.

Metal cannot be verified without a Mac. If you could not build it, say the Metal path is unverified rather than reporting it as done.
