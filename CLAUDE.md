# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

Premake5 (5.0.0-beta8) generates the build — there is no CMake. The `premake5` binary itself is gitignored; each dev supplies their own.

```bash
git submodule update --init --recursive              # 7 submodules, mandatory
export VULKAN_SDK=$HOME/VulkanSDK/<version>/x86_64   # required on every platform, or premake5 errors out
./premake5 gmake                                     # Linux and macOS; ./premake5 vs2022 on Windows
make config=release -j$(nproc)                       # config is debug|release, lowercase
make config=debug CBKEditor                          # single target (also Sandbox, CBKAssetConverter, UnitTests)
make help                                            # list targets and configs
```

There is no backend switch. Linux and Windows build Vulkan only (premake defines `CBK_RENDERER_VULKAN`); macOS builds Metal. The old `--renderer` option is gone — `premake5.lua:13-15` still reads `_OPTIONS["renderer"]`, but nothing declares or uses it. On Windows the SDK install must include the volk and VMA components (see `VULKAN_SDK_COMPONENTS` in `.github/workflows/ci-windows.yml`).

**OpenGL, OpenGL ES and the WebAssembly/Emscripten target were removed from `main`.** Their last working state lives on the `legacy` branch. Do not reintroduce them, and do not port changes from `legacy`.

Build output is in-source: `bin/<Config>-<system>-<arch>/<Project>/<Project>`.

Four submodules are **forks** (GLFW, ImGui, FreeType, Assimp) — do not swap them for upstream. volk, VMA, and Slang headers come from the Vulkan SDK, not from `vendor/`.

## Running

**Run binaries from their own output directory.** Every asset, config and scene path is relative to CWD, and premake copies `assets/` and `config.json` next to the binary:

```bash
cd bin/Release-linux-x86_64/Sandbox && ./Sandbox
cd bin/Release-linux-x86_64/CBKEditor && ./CBKEditor
```

The engine only loads `.cbkm` / `.cbkt`, never `.obj` / `.png`. Those are gitignored, so a fresh clone needs `CBKAssetConverter` run over the committed raw sources in `Sandbox/assets/` before anything renders. `config.json` is also gitignored and auto-generated on first run.

The Vulkan pipelines load `assets/shaders/Phong.slang` and `PBR.slang` during renderer init (`Platform/Vulkan/VulkanPipelineHelpers.h:32`), so every app needs those next to its binary.

CBKEditor specifics:
- `CBKEditor/assets/` is **gitignored**, and the postbuild step copies it only if it exists. Populate it yourself (at least `shaders/` and whatever the scene references).
- On startup `EditorLayer` loads `scenes/testScene.cbkscn` relative to CWD. `SceneSerializer::deserialize` asserts in Debug and throws from `json::parse` in Release when the file is missing, so that file has to exist.
- *Save Scene as…* and *Load Scene* shell out to `zenity`, so they only work on Linux with zenity installed.

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

## Renderer

The goal on `main` is a modern, Vulkan-first renderer. Two backends are selected **at compile time**:
- `CBK_RENDERER_VULKAN` on Linux and Windows. **This is the only working backend.**
- `CBK_RENDERER_METAL` on macOS, forced in `Core/Core.h:27`. **Metal is known-broken on `main`.** It still uses the pre-refactor `IMetalRecordable` model and will be ported later. Don't try to keep it compiling as a side effect of a Vulkan change unless asked, and don't delete it either.

Every abstract renderer type exposes a static `create()` that `#ifdef`s on those defines and returns `createRef<Concrete>(...)`. `Renderer/Shader.cpp:14-25` is the canonical example. Several other factories (`FrameBuffer`, `Texture`, `GeometryDescriptor`, `GraphicsContext`, `RenderCommand`, `RendererAPI`, `Texture2DMaterial`, and the `RendererAPI::API::OpenGL` enumerator) still have dead `CBK_RENDERER_OPENGL` branches. Those are leftovers, so don't copy them.

Shaders compile at **runtime**; there is no offline shader step. `VulkanShader` compiles Slang to SPIR-V 1.4. `ShaderLibrary::load()` takes an **extension-less** base path and the backend appends `.slang` (Vulkan) or `.metal` (Metal, which must expose `vertex_main` and `fragment_main`). The `.glsl` files still in `Sandbox/assets/shaders/` are dead.

Vulkan requires API **1.4** plus dynamic rendering, synchronization2 and buffer device address (`Platform/Vulkan/VulkanDeviceContext.cpp`). It uses no `VkRenderPass` or framebuffer objects.

### Materials and pipelines (Vulkan)

- Each `common::MaterialKind` (`Common/BinaryFormats.h:53`) maps to one pipeline wrapper (`VulkanPhongGraphicsPipeline`, `VulkanPBRGraphicsPipeline`), owned by `VulkanRendererAPI`. A wrapper holds a `VulkanGraphicsPipeline` member (composition, not inheritance) and supplies the set-1 binding count, push-constant struct and shader name. `Unlit` has no pipeline yet.
- `Renderer` buckets draws by `Material::getKind()`. `VulkanRendererAPI::recordMaterial` switches on the kind and `static_cast`s to the concrete material, with no `dynamic_cast` or recordable interface.
- Descriptor sets: **set 0** holds the scene UBO (camera + directional light), **set 1** holds per-material-instance textures allocated from the pipeline's pool, and **set 2** holds the point-light SSBO (max 10). Sets 0 and 2 are static and shared by every pipeline. Per-draw data (transform, material scalars) goes through push constants. The authoritative description is in `Platform/Vulkan/VulkanGraphicsPipeline.h`.
- `UBOData` is `static_assert`ed against the std140 layout of `SceneData` in `Phong.slang`, so change both together.
- To add a material kind: add a `MaterialKind` value (keeping `k_MaterialKindCount` in sync) and its JSON name in `Scene/ComponentSerialization.h`, a pipeline wrapper, an `allocate<Kind>DescriptorSet()`, a case in `recordMaterial`, and the `.slang` shader. The `add-render-resource` skill has the full walkthrough.

Currently disabled on `main`: `Renderer2D`, `TextRenderer` and `DefaultLibrary` (their `init`/`shutdown` calls are commented out in `Renderer/Renderer.cpp`), and `Texture2DMaterial::create()` / `TextMaterial::create()` return `nullptr`.

### Frame and lifetime rules

- Frame order in `Application::run()`: `beginFrame` → layers' `onUpdate` → `endScenePass` → ImGui → `endFrame`. `RenderCommand::endFrame()` runs exactly once per frame, after *all* rendering including ImGui.
- `RendererSpec::RenderSceneToTexture` (set from `Application(bool editorMode)`) renders the scene offscreen and exposes it through `RenderCommand::getFinalFrame()` for the editor viewport. With `false` (Sandbox), the scene resolves straight into the swapchain.
- Scene swaps are deferred: `Application::queueSceneLoad()` stores the scene, and `loadScene()` swaps it at the top of the next frame after `waitIdle()`. Never replace the scene mid-frame.
- `~Application` tears down explicitly in this order: layer stack → scene → `Renderer::shutdown()`. Anything that owns VMA memory must be released before `Renderer::shutdown()`, and statics that outlive it will assert on exit.

## Editor (`CBKEditor/`)

- `src/EditorApplication.cpp`: `EditorLayer` hosts a full-window dockspace plus a menu bar (*Scene*: New / Save / Save as / Load; *Window*: Reset Layout). The default layout (Outliner left, Details right, Viewport centre) is built with `DockBuilder` whenever there's no `imgui.ini`.
- Panels derive from `src/Panels/Panel.h` and open their window through `begin()` / `end()`, not `ImGui::Begin` / `End`. That's how a panel dropped on empty space snaps back into the dockspace. Each panel implements `onImGuiRender()` and `reset()`, and `reset()` runs on every scene change so panels drop stale entity handles.
- Outliner → Details selection flows through a callback (`OutlinerPanel::setSelectedEntityCallBack`), not shared state.
- `ViewportPanel` draws `getFinalFrame()` letterboxed to 16:9. Its focus, hover and size are exposed but not yet wired to camera input or render-target resizing.
- `Renderer/EditorCamera` and `RenderLayer::setEditorMode()` exist, but nothing calls `setEditorMode()` yet.
- ImGui is the vendored docking-branch fork, and `docs/imgui-widgets.md` is a widget cheat sheet for it.
- Next planned feature: a transform gizmo.

## Conventions

- `#pragma once`, never include guards.
- Use `cbk::Ref<T>` / `createRef` and `cbk::Scope<T>` / `createScope` (`Core/Core.h:61-73`), not `std::make_shared` / `std::make_unique`.
- Logging: `CBK_CORE_*` (engine), `CBK_APP_*` (game), `CBK_AC_*` (converter), fmt-style. Asserts: `CBK_CORE_ASSERT(cond, msg)` and friends are **debug-only** and compile to nothing in Release — never put side effects inside one. Wrap Vulkan calls in `VK_CHECK`.
- Naming: `m_PascalCase` members, `s_PascalCase` statics, `k_PascalCase` constants, `camelCase` methods — but **public POD/struct members are PascalCase** (`CTransform::Position`). ECS components take a `C` prefix, systems a `System` suffix, archetype builders an `Arch` suffix.
- Naming is enforced by `readability-identifier-naming` in `.clang-tidy`. Two deliberate exceptions carry `NOLINT` with a reason: nlohmann's `to_json`/`from_json` ADL hooks in `Scene/ComponentSerialization.h` (renaming them compiles and silently breaks serialization), and the named direction constants `Vector3::Up`, `Zero`, etc.
- Public struct members have no single rule: ECS components use PascalCase, while on-disk layout structs use camelCase so field names match the `.cbkm`/`.cbkt` layout. `PublicMemberCase` is intentionally unset — do not add it.
- Every `.cpp` under `Cabrankengine/` begins with `#include <pch.h>` as its literal first line (Windows force-includes it, Linux/macOS do not).
- Includes: `<...>` for cross-module paths rooted at `src`, `"..."` for same-directory siblings. `SortIncludes: false` — the grouping is hand-maintained, so do not reorder.
- Exceptions are effectively unused (one `try/catch` repo-wide, around JSON parsing in `Config/Config.h`); handle errors by log-and-return, `std::optional`, or abort. RTTI **is** required — `typeid` keys the ECS component and system storage (`ECS/ComponentManager.hpp`, `ECS/SystemManager.hpp`), so never build with `-fno-rtti`.
- Systems never reference each other; they communicate only through shared components.
- Prefer the archetype builders in `Scene/Archetypes/` over manual `createEntity` + `addComponent` chains.

## Formatting

`.clang-format` is `BasedOnStyle: LLVM` with these deviations: 140-column limit, tabs for indentation at width 4, `int* ptr` (left-aligned pointers), namespace contents indented, indented case labels, `{ 1.f, 2.f }` braced lists with inner spaces, and `SortIncludes: false`. Run `clang-format -i` on every file you touch.

## Git

`main` is the default branch and small fixes go straight to it; anything substantial gets a feature branch. `legacy` is a frozen snapshot of the OpenGL/WASM era — don't commit to it or merge from it. Commit subjects are imperative, capitalized, no trailing period, no Conventional-Commits prefix (`Fix Metal pipeline`, `Add nodiscard`) — no body, no trailers. Suffix `(WIP)` for intentionally incomplete work.

## Docs

`docs/` and `.claude/skills/` were brought in line with the Vulkan-only `main` in September 2026, but they lag behind refactors. **Trust the code over the docs**, and fix the docs and skills when you notice drift.
