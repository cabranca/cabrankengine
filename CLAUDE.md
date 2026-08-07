# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

Premake5 (5.0.0-beta8) generates the build — there is no CMake. The `premake5` binary itself is gitignored; each dev supplies their own.

```bash
git submodule update --init --recursive              # 8 submodules, mandatory
export VULKAN_SDK=$HOME/VulkanSDK/<version>/x86_64   # required, or premake5 errors out
./premake5 gmake                                     # Linux and macOS; ./premake5 vs2022 on Windows
make config=release -j$(nproc)                       # config is debug|release, lowercase
make config=debug Sandbox                            # single target
make help                                            # list targets and configs
```

`--renderer=opengl|vulkan` is a **generate-time** flag on `premake5`, not a build flag. Switching backends means re-running `premake5` *and* `make clean` — stale objects from the other backend linger in `bin-int/`. On a machine with no Vulkan SDK, use `./premake5 gmake --renderer=opengl`.

Build output is in-source: `bin/<Config>-<system>-<arch>/<Project>/<Project>`.

WASM builds go through `./build_wasm.sh`, which requires `tools/emsdk/emsdk_env.sh` (and `tools/` is gitignored).

Four submodules are **forks** (GLFW, ImGui, FreeType, Assimp) — do not swap them for upstream. volk, VMA, and Slang headers come from the Vulkan SDK, not from `vendor/`.

## Running

**Run binaries from their own output directory.** Every asset and config path is relative to CWD, and premake copies `assets/` and `config.json` next to the binary:

```bash
cd bin/Release-linux-x86_64/Sandbox && ./Sandbox
```

Invoking `./bin/.../Sandbox` from the repo root (as the README shows) fails to find assets.

The engine only loads `.cbkm` / `.cbkt`, never `.obj` / `.png`. Those are gitignored, so a fresh clone needs `CBKAssetConverter` run over the committed raw sources in `Sandbox/assets/` before anything renders. `config.json` is also gitignored and auto-generated on first run.

## Tests

Catch2 v2.13.10, single-header, vendored. No CTest.

```bash
make config=debug UnitTests
./bin/Debug-linux-x86_64/UnitTests/UnitTests                                  # all
./bin/Debug-linux-x86_64/UnitTests/UnitTests "Vector3 Arithmetic Operations"  # one TEST_CASE
./bin/Debug-linux-x86_64/UnitTests/UnitTests "Vector3*" -c "Binary Addition"  # one SECTION
./bin/Debug-linux-x86_64/UnitTests/UnitTests --list-tests
```

New test files must contain `Test` in the filename — `.runsettings` filters discovery on that regex.

## Renderer backends

Four backends, selected **at compile time** via `CBK_RENDERER_VULKAN` / `CBK_RENDERER_OPENGL` / `CBK_RENDERER_METAL`. Defaults: Linux → Vulkan, Windows → OpenGL, macOS → Metal (forced in `Core/Core.h:27`, ignores `--renderer`), WASM → OpenGL ES.

Every abstract renderer type exposes a static `create()` that `#ifdef`s on those defines and returns `createRef<Concrete>(...)`. `Renderer/Shader.cpp:18-42` is the canonical example — follow it for any new resource type.

Shaders compile at **runtime**; there is no offline shader step. `ShaderLibrary::load()` takes an **extension-less** base path and the active backend appends `.slang` (Vulkan), `.glsl` / `.glsl.es` (OpenGL / WebGL2), or `.metal`. Adding a shader therefore means authoring up to four variants. Every `.metal` file must expose `vertex_main` and `fragment_main`.

Vulkan uses dynamic rendering — no `VkRenderPass` or framebuffer objects. Descriptor set indices live in exactly one place: `Platform/Vulkan/VulkanDescriptorBinding.h`.

On Vulkan and Metal the backend `RendererAPI` owns only the frame lifecycle and `dynamic_cast`s materials to `IVulkanRecordable` / `IMetalRecordable` to delegate per-draw recording. `RenderCommand::endFrame()` must be called exactly once per frame after *all* rendering including ImGui — a no-op on OpenGL, mandatory on Metal.

## Conventions

- `#pragma once`, never include guards.
- Use `cbk::Ref<T>` / `createRef` and `cbk::Scope<T>` / `createScope` (`Core/Core.h:56-70`), not `std::make_shared` / `std::make_unique`.
- Logging: `CBK_CORE_*` (engine), `CBK_APP_*` (game), `CBK_AC_*` (converter), fmt-style. Asserts: `CBK_CORE_ASSERT(cond, msg)` and friends are **debug-only** and compile to nothing in Release — never put side effects inside one. Wrap Vulkan calls in `VK_CHECK`.
- Naming: `m_PascalCase` members, `s_PascalCase` statics, `k_PascalCase` constants, `camelCase` methods — but **public POD/struct members are PascalCase** (`CTransform::Position`). ECS components take a `C` prefix, systems a `System` suffix, archetype builders an `Arch` suffix.
- `Application::Run()` and `Application::OnEvent()` are legacy PascalCase. Leave them alone; they are the public API.
- Every `.cpp` under `Cabrankengine/` begins with `#include <pch.h>` as its literal first line (Windows force-includes it, Linux/macOS do not).
- Includes: `<...>` for cross-module paths rooted at `src`, `"..."` for same-directory siblings. `SortIncludes: false` — the grouping is hand-maintained, so do not reorder.
- Exceptions are effectively unused (one `try/catch` repo-wide, around JSON parsing); handle errors by log-and-return, `std::optional`, or abort. RTTI **is** required — `typeid` keys the ECS component storage and `dynamic_cast` drives material recording, so never build with `-fno-rtti`.
- Systems never reference each other; they communicate only through shared components.
- Prefer the archetype builders in `Scene/Archetypes/` over manual `createEntity` + `addComponent` chains.

## Formatting

`.clang-format` is `BasedOnStyle: LLVM` with these deviations: 140-column limit, tabs for indentation at width 4, `int* ptr` (left-aligned pointers), namespace contents indented, indented case labels, `{ 1.f, 2.f }` braced lists with inner spaces, and `SortIncludes: false`. Run `clang-format -i` on every file you touch.

## Git

`main` is the default branch and small fixes go straight to it; anything substantial gets a feature branch. Commit subjects are imperative, capitalized, no trailing period, no Conventional-Commits prefix (`Fix Metal pipeline`, `Add nodiscard`) — no body, no trailers. Suffix `(WIP)` for intentionally incomplete work.

## Docs

`docs/` is partly aspirational and contradicts the code in several places (entity limits, `Application::run()` vs `Run()`, a `Cabrankengine/src/Cabrankengine/Math/` directory that does not exist — math lives in `Common/src/Common/Math/` — and `CPhongModel`/`CPBRModel` vs the single real `CModel`). **Trust the code over the docs**, and fix the docs when you notice drift.
