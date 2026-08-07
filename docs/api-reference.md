# API Reference

This document covers the public API a game author uses: Registry, built-in components, and archetype builders.

For architecture context see [architecture.md](architecture.md).

---

## Registry

`cbk::ecs::Registry` — the central coordinator for all ECS operations.

Access it from anywhere via:

```cpp
cbk::ecs::Registry* reg = cbk::Application::get().getRegistry();
```

### Entity operations

```cpp
// Create a new entity (returns a uint32_t handle)
Entity e = reg->createEntity();

// Destroy an entity and remove all its components
reg->destroyEntity(e);

// Get the component signature bitset for an entity
Signature sig = reg->getSignature(e);
```

### Component operations

```cpp
// Register a component type (do this once, before any add/get)
reg->registerComponent<MyComponent>();

// Attach a component to an entity
reg->addComponent<CTransform>(e, CTransform{});
reg->addComponent<CSprite>(e, CSprite{ .Path = "assets/textures/hero.cbkt" });

// Remove a component
reg->removeComponent<CSprite>(e);

// Read/write a component — returns std::optional<T*>
if (auto transform = reg->getComponent<CTransform>(e)) {
    (*transform)->Position.x += 1.f;
}

// Const access
if (auto transform = reg->getComponent<CTransform>(e)) {
    float x = (*transform)->Position.x;
}

// Get the numeric type ID of a component (used to build Signatures)
uint8_t id = reg->getComponentType<CTransform>();
```

### System operations

```cpp
// Register a system and get a shared_ptr to it
auto mySystem = reg->registerSystem<MySystem>();

// Build a signature for the system (call after registering all needed components)
Signature sig;
sig.set(reg->getComponentType<CTransform>());
sig.set(reg->getComponentType<CSprite>());
reg->setSystemSignature<MySystem>(sig);

// Retrieve a registered system
auto mySystem = reg->getSystem<MySystem>();

// After loading a scene onto a registry that already had systems registered,
// call this to re-evaluate all entities against system signatures:
reg->rebuildSystemMembership();
```

### Writing a custom system

```cpp
#include <Cabrankengine/ECS/SystemManager.hpp>

class VelocitySystem : public cbk::ecs::ISystem {
public:
    void update(cbk::ecs::Registry& reg, float dt) override {
        for (auto entity : getEntities()) {
            auto transform  = reg.getComponent<CTransform>(entity);
            auto velocity   = reg.getComponent<CVelocity>(entity);
            if (transform && velocity) {
                (*transform)->Position += (*velocity)->Value * dt;
            }
        }
    }
};

// Registration (in your Layer::onAttach or constructor):
reg->registerComponent<CVelocity>();
auto sys = reg->registerSystem<VelocitySystem>();
Signature sig;
sig.set(reg->getComponentType<CTransform>());
sig.set(reg->getComponentType<CVelocity>());
reg->setSystemSignature<VelocitySystem>(sig);
```

---

## Built-in Components

All built-in components are in namespace `cbk::ecs` and defined in
[Cabrankengine/src/Cabrankengine/ECS/Components.h](../Cabrankengine/src/Cabrankengine/ECS/Components.h).

### CTransform

Position, rotation (Euler angles in degrees), and scale for any entity.

```cpp
struct CTransform {
    math::Vector3 Position;           // default {0, 0, 0}
    math::Vector3 Rotation;           // Euler angles in degrees, default {0, 0, 0}
    math::Vector3 Scale{ 1.f };       // default {1, 1, 1}
};
```

### CCamera

Perspective or orthographic camera. Used by `CameraSystem` to build the view-projection matrix.

```cpp
struct CCamera {
    ProjectionType Type      = ProjectionType::Perspective;
    bool           IsActive  = true;

    // Perspective
    float FovY       = PI / 4.f;   // vertical field of view in radians
    float AspectRatio = 16.f / 9.f;
    float Near        = 0.1f;
    float Far         = 100.f;

    // Orthographic
    float OrthoSize   = 900.f;     // half-height in world units
};
```

### CCameraController

First-person camera input. Used by `CameraControllerSystem` to write into `CTransform`.

```cpp
struct CCameraController {
    float TranslationSpeed = 10.f;
    float MouseSensitivity = 0.1f;
    float Yaw   = 0.f;   // degrees
    float Pitch = 0.f;   // degrees
    bool  MouseCaptured = false;
    // LastMouseX / LastMouseY are internal state; don't set manually
};
```

### CSprite

2D sprite. Loaded texture is cached in `Texture`; set `Path` and it loads on first use.

```cpp
struct CSprite {
    std::string              Path;
    Ref<rendering::Texture2D> Texture;       // populated automatically
    math::Vector4            Tint{ 1.f };    // RGBA, default white
    float                    TilingFactor{ 1.f };
};
```

### CModel

A 3D mesh. Phong and PBR are not separate component types — one `CModel` carries a
`MaterialKind` tag that tells the loader which material factory to invoke.

```cpp
struct CModel {
    std::string          Path;                              // path to a .cbkm file
    common::MaterialKind Kind = common::MaterialKind::PBR;   // PBR or Phong
    Ref<scene::Model>    Res;
};
```

`MaterialKind` is defined in `Common/src/Common/BinaryFormats.h` and is serialized to
JSON alongside the path.

### CText

Rendered text string.

```cpp
struct CText {
    std::string   Text;
    float         FontScale = 0.05f;
    math::Vector4 Color{ 1.f };   // RGBA
};
```

### CDirectionalLight

Infinite directional light (sun-like). Used by Phong and PBR render systems.

```cpp
struct CDirectionalLight {
    math::Vector3 Direction{ 0.f, -1.f, 0.f };   // world-space direction
    math::Vector3 Radiance{ 1.f };                 // RGB intensity
};
```

### CPointLight

Point light with distance attenuation. Place the entity at the desired world position via `CTransform`.

```cpp
struct CPointLight {
    math::Vector3 Radiance{ 1.f };

    float Constant  = 1.f;
    float Linear    = 0.09f;
    float Quadratic = 0.032f;
};
```

### CCollisionFilter

Bitmask-based collision filtering.

```cpp
struct CCollisionFilter {
    uint32_t CategoryBits = 0x0001;   // what this entity is
    uint32_t MaskBits     = 0xFFFFFFFF; // what it collides with
};
```

### Collider shapes

All collider templates accept `Vector2` (2D) or `Vector3` (3D) as the type parameter.

```cpp
// Axis-aligned bounding box — centered, stores half-extents
template<typename VecType>
struct CAABBCollider {
    VecType HalfExtents{ 0.5f };
};

// Sphere / circle
template<typename VecType>
struct CSphereCollider {
    float Radius = 0.5f;
};

// Oriented bounding box
template<typename VecType>
struct COBBCollider {
    VecType HalfExtents{ 0.5f };
    // 2D: float Orientation (radians); 3D: Quaternion
};

// Plane / line
template<typename VecType>
struct CPlaneCollider {
    VecType Normal{};
    float   Distance = 0.f;
};

// Capsule
template<typename VecType>
struct CCapsuleCollider {
    VecType Direction{};    // normalized spine axis
    float   HalfHeight = 0.5f;
    float   Radius     = 0.25f;
};

// Cylinder (3D only, Y-aligned)
struct CCylinderCollider {
    float HalfHeight = 0.5f;
    float Radius     = 0.5f;
};
```

---

## Archetypes

Archetypes are convenience builders defined in
[Cabrankengine/src/Cabrankengine/Scene/Archetypes/Archetypes.h](../Cabrankengine/src/Cabrankengine/Scene/Archetypes/Archetypes.h).
They create an entity, attach standard components, and expose typed accessors.

Bring them into scope with:

```cpp
using namespace cbk::scene::arch;
```

### CameraControllerArch

A camera entity with `CTransform + CCamera + CCameraController`. Pass the projection type.

```cpp
CameraControllerArch camera(ProjectionType::Perspective);
CameraControllerArch ortho(ProjectionType::Orthographic);

camera.transform().Position = { 0.f, 2.f, 10.f };
camera.camera().FovY        = cbk::math::PI / 3.f;
```

### SpriteArch

A 2D sprite entity with `CTransform + CSprite`.

```cpp
SpriteArch bg{ "assets/textures/sky.cbkt" };
bg.transform().Scale    = Vector3(800.f, 600.f, 0.f);
bg.sprite().Tint        = { 0.8f, 0.9f, 1.f, 1.f };
bg.sprite().TilingFactor = 2.f;
```

### PhongModelArch

A 3D entity with `CTransform + CModel`, where `CModel::Kind` is set to `MaterialKind::Phong`.

```cpp
PhongModelArch backpack{ "assets/models/backpack/backpack.cbkm" };
backpack.transform().Position = { 0.f, 0.f, -3.f };
```

### PBRModelArch

A 3D entity with `CTransform + CModel`, where `CModel::Kind` is set to `MaterialKind::PBR`.

```cpp
PBRModelArch gun{ "assets/models/gun/Cerberus_LP.cbkm" };
gun.transform().Scale = Vector3(0.05f);
```

### TextArch

A text entity with `CTransform + CText`.

```cpp
TextArch label{};
label.transform().Position = { 0.f, 5.f, 0.f };
label.text().Text           = "Score: 0";
label.text().Color          = { 1.f, 1.f, 0.f, 1.f };
```

### DirectionalLightArch

```cpp
DirectionalLightArch sun{};
sun.light().Direction = { 0.f, -1.f, -0.3f };
sun.light().Radiance  = { 1.f, 0.95f, 0.9f };
```

### PointLightArch

The light position is set through its `CTransform`.

```cpp
PointLightArch lamp{};
lamp.transform().Position = { 3.f, 2.f, 0.f };
lamp.light().Radiance     = { 1.f, 0.6f, 0.2f };  // warm orange
lamp.light().Linear       = 0.14f;
lamp.light().Quadratic    = 0.07f;
```

---

## Math Types

All math types are in `cbk::math`. The main types:

| Type | Description |
|------|-------------|
| `Vector2` | 2D float vector |
| `Vector3` | 3D float vector |
| `Vector4` | 4D float vector / RGBA color |
| `Mat4` | Column-major 4×4 matrix |
| `Quaternion` | Unit quaternion for 3D rotations |
| `MatrixFactory` | Static helpers: `perspective`, `orthographic`, `lookAt`, `translate`, `rotate`, `scale` |

---

## Logging

```cpp
// Engine-side (prefix CBK_CORE_*)
CBK_CORE_INFO("Renderer initialized: {}", rendererName);
CBK_CORE_WARN("Asset not found: {}", path);
CBK_CORE_ERROR("Fatal: {}", message);

// Game-side (prefix CBK_APP_*)
CBK_APP_INFO("Player health: {}", health);
CBK_APP_WARN("Out of ammo");

// CBKAssetConverter-side (prefix CBK_AC_*)
CBK_AC_ERROR("Unsupported file format: {}", ext);
```

Each family has `TRACE`, `DEBUG`, `INFO`, `WARN`, `ERROR`, and `FATAL` levels.

Logs go to stdout and a file. Backed by [spdlog](https://github.com/gabime/spdlog).

---

## Assertions

Debug-only; triggers a platform breakpoint in debug builds, no-ops in release. Because
they compile to nothing in release, never put a side effect inside one.

```cpp
CBK_CORE_ASSERT(ptr != nullptr, "Expected valid pointer");   // engine
CBK_APP_ASSERT(index < size, "Index out of range");          // game
CBK_AC_ASSERT(header.magic == k_ModelMagic, "Bad .cbkm");    // asset converter
```
