# Devlog #2 — 2D Batch Rendering with OpenGL

> **[PERSONALIZE]** Add a line about the first thing you rendered. Was it a colored triangle? A quad? How did it feel seeing pixels controlled by your own engine for the first time?

---

Once the ECS was working, I had a clean way to represent game objects. What I didn't have was any way to see them. Time to write a renderer.

The naive approach is simple: for each sprite entity, bind a texture, bind a shader, issue a draw call, unbind. This works. It also murders performance the moment you have more than a handful of objects, because GPU draw calls have overhead — state validation, driver work, command buffer submissions. A Survivors-style game with 500 enemies means 500 draw calls per frame. That's not acceptable.

The standard solution is batching.

## How batch rendering works

Instead of one draw call per sprite, you accumulate all sprite geometry into a single vertex buffer on the CPU, then flush the entire buffer in one draw call. A quad is two triangles, six indices, four vertices. If you support up to 1000 quads per batch, you pre-allocate a vertex buffer for 4000 vertices and an index buffer for 6000 indices, then fill them incrementally each frame.

```cpp
// Each vertex carries position, color, UV, texture slot, and tiling factor
struct QuadVertex {
    Vector3 Position;
    Vector4 Color;
    Vector2 TexCoord;
    float   TexIndex;
    float   TilingFactor;
};
```

The trick with textures is the `TexIndex` field. Modern OpenGL lets you bind multiple textures simultaneously (`GL_TEXTURE0` through `GL_TEXTURE31` on most hardware). Instead of rebinding for each sprite, you bind up to 32 different textures at the start of the batch and store the slot index per vertex. The fragment shader samples from `texture(u_Textures[int(v_TexIndex)], v_TexCoord)`.

When the batch is full — either the vertex buffer is at capacity or you've run out of texture slots — you flush (draw call + reset), then continue.

## The flush

```
beginScene(camera)   ← upload view-projection matrix
  submit(sprite1)    ← write 4 vertices into the staging buffer
  submit(sprite2)
  ...
  if buffer full → flush
endScene()           ← final flush
```

`flush` does the minimum: upload the dirty portion of the vertex buffer with `glBufferSubData`, then `glDrawElements`. One draw call. No texture rebinding mid-batch.

## What this looked like in practice

Before batching, rendering 200 sprites sat around 60fps on my machine. After batching, 2000 sprites at 60fps. The difference wasn't even particularly clever code — it was just stop doing 2000 redundant things per frame.

> **[ADD SCREENSHOT/GIF: 2DBatchRender.gif — the existing one in docs/media/ is perfect here]**

## One gotcha: the index buffer is static

The vertex buffer changes every frame (sprites move). The index buffer never does — a quad is always `{0, 1, 2, 2, 3, 0}` offset by 4 * quadi. So the index buffer gets filled once at startup and stays. Only the vertex buffer gets sub-uploaded each frame.

## What's next

2D was working. The natural next step was 3D — and 3D rendering introduces materials, lighting, and a lot more complexity.

---

*[PERSONALIZE: what was the first game-like thing you rendered with the batch renderer? What did you notice about performance?]*

**Links:** [GitHub repo](https://github.com/cabranca/game-dev) | [Renderer source](https://github.com/cabranca/game-dev/tree/main/Cabrankengine/src/Cabrankengine/Renderer)
