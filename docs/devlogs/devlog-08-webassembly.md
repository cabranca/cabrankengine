# Devlog #8 — WebAssembly: Running My Engine in the Browser

> **[PERSONALIZE]** Add a line about why you wanted the engine in the browser. Was it for the editor, for demos, just curiosity? What did it feel like the first time it loaded in a tab?

---

At some point I had a working engine — ECS, renderer, materials, lights, serialization — and it ran fine on Linux. The next question was: could it run in a browser?

WebAssembly (WASM) lets you compile C++ to a binary format that runs in the JavaScript engine of any modern browser. The toolchain for this is [Emscripten](https://emscripten.org/), which replaces your system compiler, provides its own implementations of platform APIs (OpenGL via WebGL, file I/O via virtual filesystem, threading via Web Workers), and generates the HTML/JS glue code.

The end goal wasn't just to run the engine in the browser. It was to build a web-based editor — a tool where you can load a scene, inspect entities, and make changes without installing anything. The WASM target was step one.

## The compilation target

I added a separate Makefile (`CBKBindings/Makefile.emscripten`) for the WASM build. The key Emscripten flags:

```makefile
EMFLAGS = \
  -s USE_WEBGL2=1           \  # OpenGL ES 3.0 → WebGL 2
  -s ALLOW_MEMORY_GROWTH=1  \  # don't cap the heap
  -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
  --bind                       # Embind for C++ ↔ JS bindings
```

`--bind` enables [Embind](https://emscripten.org/docs/porting/connecting_cpp_and_javascript/embind.html), which lets you expose C++ classes and functions to JavaScript directly. This is how the editor will call into the engine.

## Platform guards

The engine uses GLFW for windowing on desktop. GLFW doesn't exist in the browser; Emscripten provides its own event loop and canvas. I added `#ifdef CBK_PLATFORM_WASM` guards around platform-specific code so the engine compiles cleanly in both contexts:

```cpp
#ifndef CBK_PLATFORM_WASM
#include <GLFW/glfw3.h>
// desktop window initialization
#endif
```

The renderer already used OpenGL 4.5 on desktop. WebGL 2 is roughly OpenGL ES 3.0 — a strict subset. I had to audit shader code for features not in ES 3.0 (no `gl_ClipDistance`, no `ARB_*` extensions) and strip them or provide fallbacks.

## The bindings layer: CBKBindings

`CBKBindings` is a thin C++ module that exposes engine functionality to JavaScript via Embind:

```cpp
EMSCRIPTEN_BINDINGS(cabrankengine) {
    emscripten::class_<cbk::Scene>("Scene")
        .function("getEntityNames", &cbk::Scene::getEntityNames)
        .function("getComponent",   &cbk::Scene::getComponentJS);

    emscripten::function("loadScene", &loadSceneFromJS);
}
```

On the JavaScript side, this becomes:

```js
const scene = Module.loadScene("scene.json");
const names = scene.getEntityNames();  // ["Camera", "Gun", "Sun"]
```

> **[ADD SCREENSHOT: browser tab with the engine running — even a blank canvas with the demo scene works here]**

## What broke

Several things required fixing:

1. **irrKlang audio** — the audio library I was using doesn't compile to WASM. I removed it entirely and left audio as a future problem. The CI workflows now skip it too.
2. **Assimp in WASM** — too large and unnecessary for runtime. The converter already produced `.cbkm` files, so I just don't link Assimp in the WASM build.
3. **File I/O** — `fopen` in a browser reads from a virtual filesystem that Emscripten manages. Assets need to be `--preload-file` packed into the WASM binary or fetched via `fetch()`. I used preloaded assets for the demo.
4. **WebGL texture format limits** — WebGL 2 doesn't support all the internal formats OpenGL 4.5 does. Compressed texture formats (BPTC/BC7) aren't available without extensions. I fell back to RGBA8 for the browser build.

## CI

I added a GitHub Actions workflow that builds the WASM target on every push:

```yaml
- name: Install Emscripten
  run: |
    git clone --depth 1 https://github.com/emscripten-core/emsdk.git
    cd emsdk && ./emsdk install latest && ./emsdk activate latest
- name: Build WASM
  run: source emsdk/emsdk_env.sh && make -f CBKBindings/Makefile.emscripten
```

Catching WASM regressions in CI means I don't discover them three weeks later.

## What's next

With a WASM build of the engine and a working bindings layer, the next step is the editor itself — a web app that talks to the engine via those bindings.

---

*[PERSONALIZE: what was the first thing that worked in the browser? Was there a specific bug that took you the longest to track down?]*

**Links:** [GitHub repo](https://github.com/cabranca/game-dev) | [CBKBindings source](https://github.com/cabranca/game-dev/tree/main/CBKBindings)
