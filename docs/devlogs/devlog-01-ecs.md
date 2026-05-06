# Devlog #1 — Building an ECS from Scratch

> **[PERSONALIZE]** Add 1–2 sentences about where you were when you started this project. What pushed you to build a game engine instead of just using one?

---

At some point I stopped finding game jams satisfying. I wasn't learning anything new — I was just duct-taping Unity components together and shipping a prototype I had no idea how to maintain. So I decided to build an engine. Not to use it for shipping games, but to understand every piece: how rendering works, how game objects are managed, how systems talk to each other.

The first real decision I had to make was: how do you represent a game object?

## The obvious answer and why it fails

The instinctive answer is inheritance. You write a `GameObject` base class, then `Player : public GameObject`, `Enemy : public GameObject`, and so on. This works for five objects. It falls apart when a `Player` needs physics but a `Ghost` doesn't, and a `Ghost` needs rendering but a `StaticWall` doesn't, and now you have a diamond inheritance problem and a `RigidBodyRenderedCollidableEntity` class nobody wants to maintain.

The other failure mode is the mega-object: a `GameObject` with every possible field — health, velocity, sprite, shader, collider — most of them irrelevant for any given instance. Memory layout suffers. Cache performance suffers.

## Entity-Component-System

ECS flips the model. An **entity** is just a number — a `uint32_t` handle with no data of its own. Data lives in **components**, which are plain structs. **Systems** operate on entities that have a specific set of components.

```
Entity:    1234
Components: CTransform, CSprite, CVelocity

System VelocitySystem processes all entities with {CTransform, CVelocity}
System SpriteSystem   processes all entities with {CTransform, CSprite}
```

Entity 1234 is processed by both. You don't subclass anything. You compose.

## The implementation

I split the implementation into three managers and a Registry that coordinates them.

**EntityManager** — issues entity IDs from a queue, tracks which IDs are alive, and stores each entity's component *signature*: a `std::bitset<64>` where bit N is set if the entity has component type N.

**ComponentManager** — stores each component type in a `ComponentArray<T>`, a dense array with two maps: `entity → dense index` and `dense index → entity`. This means iterating all `CTransform` components is a tight loop over contiguous memory, regardless of which entity IDs are alive.

**SystemManager** — holds all registered systems. When a component is added or removed, it checks whether the entity's new signature satisfies each system's required signature. If yes, the entity is added to that system's set. If no longer satisfied, it's removed.

The **Registry** is the only public-facing class. Games never touch the three managers directly.

```cpp
Entity player = registry.createEntity();
registry.addComponent<CTransform>(player, CTransform{});
registry.addComponent<CSprite>(player, CSprite{ .Path = "hero.cbkt" });
```

## What surprised me

The hardest part wasn't the data structures — it was the ordering problem. Systems process entities in whatever order they live in an `unordered_set`. I expected that to be fine. It isn't, because frame N's rendering reads positions that frame N's physics needs to have already updated. The ECS doesn't enforce system execution order; the engine's frame loop does. Getting that right took more iteration than the ECS itself.

The other thing that surprised me: `std::bitset` comparison is fast enough that you can afford to run `EntitySignatureChanged` on every add/remove without batching. The membership check is essentially free compared to the actual system work.

## What's next

A data model without rendering isn't very interesting. The next step was getting something on screen — and that meant building a renderer.

---

*[PERSONALIZE: add a specific moment or bug that stood out during this implementation.]*

**Links:** [GitHub repo](https://github.com/cabranca/game-dev) | [ECS source](https://github.com/cabranca/game-dev/tree/main/Cabrankengine/src/Cabrankengine/ECS)
