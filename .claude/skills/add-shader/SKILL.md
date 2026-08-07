---
name: add-shader
description: Add, modify, or port a shader in Cabrankengine. Use whenever a new shader is needed, an existing shader gains a uniform or binding, or a shader must be brought up on another backend — this covers the four per-backend source variants, the extension-less load path, and the Metal entry-point contract.
---

# Adding a shader

Shaders in this engine are **compiled at runtime**. There is no offline shader build step, no generated SPIR-V checked in, and nothing in the build graph to update. What exists instead is a per-backend filename convention that is easy to get half-right.

## The load contract

`ShaderLibrary::load()` (declared in `Cabrankengine/src/Cabrankengine/Renderer/Shader.h:47-72`) takes an **extension-less base path**:

```cpp
ShaderLibrary::load("assets/shaders/Phong");
```

`Shader::create()` (`Renderer/Shader.cpp:18-29`) dispatches on the compile-time `CBK_RENDERER_*` define, and the concrete shader appends the extension it needs:

| Backend | Extension appended | Appended in |
|---|---|---|
| Vulkan | `.slang` | `Platform/Vulkan/VulkanShader.cpp` |
| OpenGL 4.5 | `.glsl` | `Platform/OpenGL/OpenGLShader.cpp` |
| OpenGL ES / WebGL2 | `.glsl.es` (under `CBK_OPENGL_ES`) | `Platform/OpenGL/OpenGLShader.cpp` |
| Metal | `.metal` | `Platform/Metal/MetalShader.cpp` |

A missing variant is therefore **not a compile error** — it is a runtime load failure on exactly one backend, which is why gaps go unnoticed. Confirm the current extension logic in those three files before relying on the table; it is the kind of thing that drifts.

## Steps

1. **Decide the target backends.** Full coverage means four files in `Sandbox/assets/shaders/`: `<Name>.slang`, `<Name>.glsl`, `<Name>.glsl.es`, `<Name>.metal`. Partial coverage is a deliberate choice, not an oversight — say which backends are being skipped and why.

2. **Write each variant** against the same uniform/binding layout. They are separate sources, so nothing keeps them in sync but discipline: a binding added to one must be added to all.

3. **Metal entry points are fixed.** Every `.metal` file must expose functions named `vertex_main` and `fragment_main` — `Platform/Metal/MetalShader.cpp` looks them up by name. Any other name loads and then fails at pipeline creation.

4. **Vulkan descriptor sets are centralized.** If the shader introduces a new resource binding, the set index convention lives in exactly one place: `Platform/Vulkan/VulkanDescriptorBinding.h` (set 0 = scene globals, set 1 = material, set 2 = point-light SSBO). Update it there rather than hard-coding a number in the shader or the material.

5. **GLSL ES is the most constrained target.** WebGL2 has no compute, tighter precision rules, and a smaller uniform budget. Expect to simplify rather than transliterate the desktop GLSL, and mark `precision` qualifiers explicitly.

6. **Register the shader** where its peers are registered — `Cabrankengine/src/Cabrankengine/Scene/DefaultLibrary.cpp` for engine defaults, or the material's own constructor for material-owned shaders (see `Renderer/Materials/PhongMaterial.cpp` and its `Platform/Vulkan/VulkanPhongMaterial.cpp` / `Platform/Metal/MetalPhongMaterial.cpp` counterparts).

## Existing coverage

Check the real state before assuming, since it moves:

```bash
ls -1 Sandbox/assets/shaders/ | sed 's/\.\(slang\|metal\|glsl\.es\|glsl\)$//' | sort -u | while read -r n; do
  printf '%-14s' "$n"
  for e in slang glsl glsl.es metal; do
    [ -f "Sandbox/assets/shaders/$n.$e" ] && printf ' %-8s' "$e" || printf ' %-8s' "-"
  done; echo
done
```

Known gaps at the time of writing: `PBR` has no `.glsl.es` (so PBR does not run on WASM/WebGL2), `LightSource` is OpenGL-only, `ModelTest` is `.glsl`-only, and `Triangle.metal` has no counterparts. Some of these are dead debug artifacts rather than real gaps — ask before "fixing" one.

## Verifying

There is no shader unit test. Verification means running the Sandbox on the affected backend:

```bash
./premake5 gmake                    # or --renderer=opengl
make config=debug Sandbox -j$(nproc)
cd bin/Debug-linux-x86_64/Sandbox && ./Sandbox
```

Shader compile errors surface through `CBK_CORE_ERROR` at load time, so run with `--log-level=debug` if the failure is quiet. Testing the Metal variant requires a Mac; if one is not available, say the variant is unverified rather than implying it works.
