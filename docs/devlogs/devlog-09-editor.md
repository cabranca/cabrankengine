# Devlog #9 — Building a Web Editor for the Engine

> **[PERSONALIZE]** Add a line about what prompted the editor idea. Was it frustration with hardcoding scenes? A conversation with Fran? What's the vision for what the editor should eventually do?

---

Every serious game engine has an editor. Unity has the Unity Editor. Unreal has Unreal Editor. Mine had... a text editor and a recompile cycle.

The idea for a web editor came from a practical observation: if the engine can compile to WebAssembly and expose its internals via JavaScript bindings, you don't need a native UI toolkit. You can build the editor as a web app, run the engine in the same tab, and communicate between them via the Embind bridge. The editor runs anywhere a browser runs. No installation.

## The architecture

The editor is a separate project (`Cabrankeditor/`) with its own build. The engine runs as a WASM module in the browser, loaded by the editor's JavaScript. The editor's UI is HTML/CSS/JS — no framework required for the initial version.

```
Browser tab
  ├── Cabrankeditor (HTML/JS/CSS)    ← editor UI: entity list, inspector panel, viewport
  └── Cabrankengine.wasm             ← the actual engine, rendering into a <canvas>
       └── CBKBindings               ← Embind bridge: JS calls into C++
```

The viewport is a `<canvas>` element. The engine renders into it via WebGL. The editor UI lives in DOM elements beside it. When you select an entity in the editor, a JavaScript call goes through Embind to the C++ side, which reads the component data and returns it as a JSON string. The editor parses that and renders an inspector panel.

## What's implemented so far

- Engine loads and renders into the canvas — the RenderLayer runs in a `requestAnimationFrame` loop
- Entity list panel populated from the scene (names come from the `Scene::createEntity(name)` API added earlier)
- Basic inspector that reads `CTransform` fields and displays them
- Scene load/save via the JS bindings layer

> **[ADD SCREENSHOT or GIF: DebugImGui.gif from docs/media/ works here, or a screenshot of the web editor UI if you have one]**

## The RenderLayer refactor for editor mode

The built-in `RenderLayer` assumes it's the top of the stack and controls frame submission. For the editor, the editor's JavaScript drives the frame loop instead. I added an **editor mode** flag to `RenderLayer` that changes the submission path:

- **Game mode** — `RenderLayer` calls `RenderCommand::clear()` and submits everything to the default framebuffer
- **Editor mode** — `RenderLayer` renders to an offscreen framebuffer; the editor JS reads the texture and displays it in the canvas

This keeps the game build path unchanged while giving the editor control over the frame.

## The bindings interface

The key bindings exposed via Embind:

```cpp
EMSCRIPTEN_BINDINGS(cabrankengine) {
    // Load a scene from JSON string
    emscripten::function("loadSceneFromJSON", &loadSceneFromJSON);

    // Get all entity names in the current scene
    emscripten::function("getEntityNames", &getEntityNames);

    // Get a component as a JSON string for the inspector
    emscripten::function("getComponent", &getComponentJSON);

    // Set a component from a JSON string (editor writes back)
    emscripten::function("setComponent", &setComponentJSON);
}
```

The JSON round-trip through the bindings is not the most efficient design, but it's the simplest: the editor doesn't need to know about C++ types, and the engine doesn't need to know about DOM structures. Both sides speak JSON.

## What's next

The editor is early. The pieces are in place — WASM build, bindings, entity list, basic inspector — but there's a lot of polish left:

- Transform gizmo in the viewport (translate/rotate/scale handles)
- Full component inspector with typed editors (vector fields, color pickers, asset pickers)
- Drag-and-drop asset loading
- Undo/redo
- Multi-scene management

The longer-term vision is an editor you can share with a link — load a scene URL, inspect it, make changes, export the modified JSON. No install required.

---

*[PERSONALIZE: describe where the editor is today. What's the most interesting technical challenge remaining? What are you working on right now?]*

**Links:** [GitHub repo](https://github.com/cabranca/game-dev) | [Cabrankeditor source](https://github.com/cabranca/game-dev/tree/main/Cabrankeditor) | [CBKBindings](https://github.com/cabranca/game-dev/tree/main/CBKBindings)
