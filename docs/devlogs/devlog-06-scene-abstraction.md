# Devlog #6 — Refactoring: The Scene Abstraction and Serialization

> **[PERSONALIZE]** Add a line about what the code looked like before — entities hardcoded in the constructor, magic numbers everywhere, etc. What triggered the decision to refactor?

---

For the first few months, every scene was a hardcoded list of entity creation calls inside a layer constructor. Changing anything meant recompiling. "Let me just move the gun two units to the left" required a compile cycle. Loading a different level meant changing the code.

That's obviously not sustainable, but it was fine while I was focused on getting the systems working. At some point the friction got high enough that I had to deal with it.

## The Scene class

Before this refactor, the `Registry` was the top-level object. The `Application` owned a `Registry` and layers talked directly to it. This was fine but brittle — there was no logical grouping of entities, no name for what a "scene" even was in the codebase.

I introduced a `Scene` class that wraps the `Registry` and adds:
- Entity naming (create an entity with a human-readable name for editor tooling)
- A stable handle passed around instead of the raw `Registry*`
- A clear ownership point for serialization

```cpp
// Before
auto* registry = Application::get().getRegistry();
Entity e = registry->createEntity();

// After
auto& scene = Application::get().getScene();
Entity e = scene.createEntity("Player");
```

Small change, but it matters when you're building an editor that needs to display a list of entities.

## JSON serialization

`SceneSerializer` reads and writes the full entity graph as JSON. Every built-in component has a serializer and deserializer. A saved scene looks roughly like:

```json
{
  "entities": [
    {
      "name": "Camera",
      "components": {
        "CTransform": { "position": [0, 2, 10], "rotation": [0, 0, 0], "scale": [1, 1, 1] },
        "CCamera":    { "type": "Perspective", "fovY": 0.785, "near": 0.1, "far": 100 },
        "CCameraController": { "translationSpeed": 10, "mouseSensitivity": 0.1 }
      }
    },
    {
      "name": "Gun",
      "components": {
        "CTransform": { "position": [2, -2, 2], "rotation": [0, 0, 0], "scale": [0.1, 0.1, 0.1] },
        "CPBRModel":  { "path": "assets/models/gun/Cerberus_LP.cbkm" }
      }
    }
  ]
}
```

Loading:

```cpp
Application::get().loadScene(SceneSerializer::deserialize("scene.json"));
```

One call, and all the entities and components from the file are recreated in the Registry. Because systems derive their entity membership from signatures, `rebuildSystemMembership()` runs automatically after load to re-evaluate all entities against all registered systems.

## The rebuild step

This was a subtle issue. When you load a scene, you're populating a registry that already has systems registered. The systems need to know which entities satisfy their signatures. The normal path — `addComponent` triggering `EntitySignatureChanged` — doesn't fire for entities created by the deserializer through internal paths.

The fix: after deserialization, sweep all entities and re-run `EntitySignatureChanged` for each one that has any components. One pass, and every system's entity set is correct.

```cpp
void Registry::rebuildSystemMembership() {
    for (Entity e = 0; e < MAX_ENTITIES; e++) {
        Signature sig = m_EntityManager->getSignature(e);
        if (sig.any())
            m_SystemManager->EntitySignatureChanged(e, sig);
    }
}
```

## What this unlocked

With serialization working, I could save scenes from the Sandbox, edit the JSON, reload, and see the changes without recompiling. More importantly, it laid the groundwork for an editor — something that could write entity data directly to the file and have the engine reload it.

## What's next

Raw mesh files from Blender are large, slow to parse, and full of details the engine doesn't need. I built a dedicated asset converter.

---

*[PERSONALIZE: what was the first scene you successfully saved and reloaded? Any funny deserialization bugs?]*

**Links:** [GitHub repo](https://github.com/cabranca/game-dev) | [Scene source](https://github.com/cabranca/game-dev/tree/main/Cabrankengine/src/Cabrankengine/Scene)
