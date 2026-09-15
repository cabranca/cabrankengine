---
name: add-shader
description: Add, modify, or port a shader in Cabrankengine. Use whenever a new shader is needed, or an existing shader gains a uniform, texture or push-constant field — this covers the Slang file layout and entry-point names, the descriptor-set and push-constant contract with the Vulkan pipelines, the C++ structs that must mirror the shader, where shader files must live, and the optional Metal variant.
---

# Adding a shader

Shaders are **compiled at runtime**. There is no offline shader build step, no checked-in SPIR-V, and nothing in the build graph to update. `VulkanShader` compiles a `.slang` file to SPIR-V 1.4 with Slang when the pipeline that uses it is created. Shader errors therefore show up at renderer init, as `CBK_CORE_ERROR` lines starting with `VulkanShader:`.

Vulkan is the only backend that runs on `main`, so **the `.slang` file is the shader**. Metal still reads `.metal` files but is broken. OpenGL `.glsl` support is gone from `main` and exists only on the `legacy` branch.

## The load contract

`ShaderLibrary::load()` (`Renderer/Shader.h:20-44`) takes an **extension-less base path**, and the backend appends `.slang` (Vulkan) or `.metal` (Metal). In practice you don't call it directly. Pipeline wrappers load their shader through `createShaderModule("<Name>")` (`Platform/Vulkan/VulkanPipelineHelpers.h:32`), which loads `assets/shaders/<Name>` relative to the working directory and returns the Vulkan shader module.

## The file contract

One `.slang` file holds **both stages**, and both entry points are named `main`:

```slang
[shader("vertex")]
VSOutput main(VSInput input) { ... }

[shader("fragment")]
float4 main(VSOutput input) : SV_TARGET { ... }
```

- `VulkanShader` links every entry point the module defines into one SPIR-V module (`VulkanShader.cpp:61-67`).
- `VulkanGraphicsPipeline::createPipeline` requests entry point `"main"` for both the vertex and the fragment stage (`VulkanGraphicsPipeline.cpp:118-124`). Any other name compiles fine and then fails at pipeline creation.
- Every pipeline shares the same vertex input layout, because `createPipeline` is common code and `PipelineDescriptor` has no vertex-layout field. Reuse `VSInput` from `Phong.slang` unchanged.

## The binding contract

All pipelines share one layout. Declare bindings with `[[vk::binding(binding, set)]]`, and list them in a header comment the way `Phong.slang:4-6` does.

| Slot | Slang declaration | Stages | C++ mirror that must match |
|---|---|---|---|
| set 0, binding 0 | `ConstantBuffer<SceneData>`: camera + directional light, std140 | vertex, fragment | `UBOData` in `VulkanGraphicsPipeline.h`, `static_assert`ed to 112 bytes |
| set 1, bindings 0..N-1 | one `Sampler2D` per material texture | fragment | `k_MaterialBindingCount` in the pipeline wrapper, plus the write order in `Vulkan<Kind>Material::updateDescriptorSet()` |
| set 2, binding 0 | `ByteAddressBuffer` of point lights | fragment | `GPUPointLightsBufferHeader` + `GPUPointLight` in `VulkanGraphicsPipeline.h` |
| push constants | `[[vk::push_constant]] ConstantBuffer<PushConstants>`: transform + material scalars | vertex, fragment | `PushData` in the pipeline wrapper, `static_assert`ed on size |

What that implies:

- **Set 1 is samplers only.** `createSetLayoutBindings` hard-codes fragment-stage combined image samplers. A vertex-stage texture or a per-material uniform buffer means changing that helper first.
- **Sets 0 and 2 are shared by every pipeline.** Changing `SceneData` or the light structs means updating every `.slang` file and the C++ mirror together.
- **Keep padding explicit on both sides.** The C++ mirrors pad `vec3` fields out to 4-component alignment (`Pad0`, `Padding`). The `static_assert`s catch size drift, but not reordered fields.

## Steps

1. **Write `Sandbox/assets/shaders/<Name>.slang`**, starting from `Phong.slang`, the smaller of the two live shaders. Keep the set 0 and set 2 declarations and `VSInput` identical; change set 1, the push constants and the shading.
2. **Update the C++ mirrors** from the table for anything you changed.
3. **Hook it up.** A new material kind needs a pipeline wrapper that calls `createShaderModule("<Name>")`; follow the `add-render-resource` skill. Changing an existing shader needs nothing else.
4. **Copy it into `CBKEditor/assets/shaders/`.** That directory is gitignored and nothing generates it, so otherwise the editor keeps loading its old copy. Premake copies `assets/` next to each binary as a post-build step, so either rebuild or copy the file straight into `bin/<Config>-<system>-<arch>/<App>/assets/shaders/`.
5. **Metal variant (optional).** A `<Name>.metal` file must expose `vertex_main` and `fragment_main` (`MetalShader.cpp:77`). Metal is broken on `main`, so skipping it is fine — but say explicitly that you skipped it.

## Existing shaders

`Sandbox/assets/shaders/` on `main`:

- **Live:** `Phong.slang` and `PBR.slang`, loaded by `VulkanPhongGraphicsPipeline` and `VulkanPBRGraphicsPipeline`.
- **Present but unused:** `Error.slang`, `Text.slang` and `Texture.slang`. Their consumers (`DefaultLibrary`, `TextRenderer`, `Renderer2D`) are disabled, and they were written for the older material design, so check them against the table above before reviving one.
- **Metal:** `Error.metal`, `PBR.metal`, `Phong.metal`, `Text.metal`, `Texture.metal` and `Triangle.metal`, all for the broken Metal backend.
- **Dead:** every `.glsl` file. They are OpenGL leftovers that nothing loads; ask before deleting them.

## Verifying

There is no shader unit test. Run an app from its output directory and watch renderer init:

```bash
make config=debug Sandbox -j$(nproc)
cd bin/Debug-linux-x86_64/Sandbox && ./Sandbox --log-level=debug
```

Compile and link failures print as `VulkanShader: ...` with Slang's diagnostics. After copying the shader into its assets, check CBKEditor as well, since it renders through the render-to-texture path. A Metal variant cannot be verified without a Mac, so report it as unverified rather than implying it works.
