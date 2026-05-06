# Devlog #4 — Camera Systems and ECS Composition

> **[PERSONALIZE]** Add a line about the specific thing that was broken before — were you hardcoding the view matrix? Was the camera just a global variable? What did cleaning that up feel like?

---

For a while, the camera in Cabrankengine was an embarrassment. It was a global variable. Or close to it — a `CameraController` object sitting directly on the layer, with input handling and matrix computation mixed into `onUpdate`. It worked, but it wasn't ECS. It was just a class with a view matrix attached to the side of the engine like a post-it note.

The right solution was obvious: the camera is game data, so it should be a component. The input controller is a behavior, so it should be a system.

## Two systems, one shared component

The camera works through two systems that both touch `CTransform`:

**CameraControllerSystem** has signature `{CTransform, CCameraController}`. It reads input every frame and writes the updated position and rotation back into `CTransform`.

**CameraSystem** has signature `{CTransform, CCamera}`. It reads the position and rotation from `CTransform` and computes the view-projection matrix that the renderer will use.

Neither system knows about the other. The contract between them is `CTransform`. The camera controller writes it; the camera system reads it. If you run the controller before the camera system in the frame loop, the camera always sees the latest position.

```
Frame N:
  CameraControllerSystem  →  writes CTransform (new position from WASD + mouse)
  CameraSystem            →  reads CTransform  → builds view-projection matrix
  SpriteRenderSystem      →  renders with that matrix
```

## What this enables

An entity with `CTransform + CCamera + CCameraController` is a player-controlled camera.

An entity with only `CTransform + CCamera` is a fixed or scripted camera — maybe a cinematic that follows a path, driven by a custom system that writes into `CTransform`. The controller never touches it because it lacks `CCameraController`.

You get both behaviors from the same systems. No inheritance. No flags. No `if (isPlayerControlled)`. The signature is the switch.

## The projection types

`CCamera` supports perspective and orthographic projections via a `ProjectionType` enum:

```cpp
// Perspective — for 3D scenes
CameraControllerArch camera3D(ProjectionType::Perspective);
camera3D.camera().FovY        = PI / 4.f;   // 45 degrees
camera3D.camera().AspectRatio = 16.f / 9.f;
camera3D.camera().Near        = 0.1f;
camera3D.camera().Far         = 100.f;

// Orthographic — for 2D scenes
CameraControllerArch camera2D(ProjectionType::Orthographic);
camera2D.camera().OrthoSize = 900.f;   // half-height in world units
```

The archetype sets everything up with sane defaults. The first line creates the entity with all three components attached.

## Mouse capture

`CCameraController` handles first-person mouse look by tracking delta between frames. The `MouseCaptured` flag gates whether the engine grabs the cursor — toggled by right-click. This was important to get right for ImGui panels: you don't want the camera spinning while you're adjusting a slider.

## What's next

The camera let me actually look at the scene. The next thing I noticed was that the scene was dark. It was time to add lights.

---

*[PERSONALIZE: what was the first scene you navigated with the working camera? Any funny bugs with inverted axes or gimbal lock?]*

**Links:** [GitHub repo](https://github.com/cabranca/game-dev) | [ECS systems](https://github.com/cabranca/game-dev/tree/main/Cabrankengine/src/Cabrankengine/ECS)
