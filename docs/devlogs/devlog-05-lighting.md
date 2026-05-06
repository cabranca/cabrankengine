# Devlog #5 — Adding Lights: Directional and Point Lights

> **[PERSONALIZE]** Add a line about what the scene looked like before lights — uniform ambient gray? All white? What was the moment where the first directional light made things look real?

---

Once I had 3D models on screen with PBR materials, I realized they looked flat. Not stylistically flat — literally flat, like a textured cardboard cutout. The reason was that my lighting was just a hardcoded constant ambient term in the shader: every fragment got the same base color regardless of where the light was or where the surface was facing. Materials looked like they had detail in them, but none of it responded to the scene.

It was time to make light a first-class concept in the ECS.

## Two light types

I started with the two most fundamental light types in real-time rendering:

**Directional light** — infinite distance, uniform direction. Models the sun. Every surface in the scene receives light from the same direction with the same intensity. No position needed, no attenuation.

```cpp
struct CDirectionalLight {
    Vector3 Direction{ 0.f, -1.f, 0.f };   // normalized world-space direction
    Vector3 Radiance{ 1.f };                // RGB light color × intensity
};
```

**Point light** — emits in all directions from a position. Attenuates with distance using the classic quadratic falloff formula:

```
attenuation = 1 / (Constant + Linear * d + Quadratic * d²)
```

The three coefficients let you tune the falloff curve. High constant = soft falloff. High quadratic = sharp falloff.

```cpp
struct CPointLight {
    Vector3 Radiance{ 1.f };
    float Constant  = 1.f;
    float Linear    = 0.09f;
    float Quadratic = 0.032f;
};
```

The point light's position comes from `CTransform`. Place the entity where you want the light.

## Fitting lights into ECS

Light data lives in components like everything else. The render systems query for light entities at the start of each frame and upload their data to the shader via uniforms before drawing any mesh.

The shader computes the contribution of all directional lights, then loops over all point lights, accumulating radiance:

```glsl
// Pseudocode in the PBR fragment shader
vec3 Lo = vec3(0.0);
for each directional light:
    Lo += evaluateBRDF(surface, light);
for each point light:
    float atten = 1.0 / (c + l*d + q*d*d);
    Lo += evaluateBRDF(surface, light) * atten;
```

## Archetype builders

The archetype pattern from earlier carried forward cleanly:

```cpp
DirectionalLightArch sun{};
sun.light().Direction = { 0.f, -1.f, -0.3f };
sun.light().Radiance  = { 1.f, 0.95f, 0.85f };   // warm sunlight

PointLightArch lamp{};
lamp.transform().Position = { 3.f, 2.f, 0.f };
lamp.light().Radiance     = { 1.f, 0.4f, 0.1f };  // orange lamp
lamp.light().Linear       = 0.14f;
lamp.light().Quadratic    = 0.07f;
```

> **[ADD SCREENSHOT: a 3D scene with visible directional and/or point lighting showing shadow gradients and material response]**

## What surprised me

Tuning attenuation coefficients is more art than science. The "correct" physically-based formula is inverse square (`Constant=0, Linear=0, Quadratic=1`), but that gives a very sharp falloff that rarely looks good in practice. The defaults in the engine (`Linear=0.09, Quadratic=0.032`) approximate a 30-unit radius soft falloff, which I found works well for most indoor/outdoor scenes.

Also: getting normals correct in world space vs. tangent space vs. view space for lighting calculations is one of those things that looks subtly wrong for a long time before you identify the coordinate space mismatch.

## What's next

I had 3D objects, materials, cameras, and lights. The next step was making scenes manageable — being able to save, load, and edit them rather than hardcoding everything in the layer constructor.

---

*[PERSONALIZE: what scene were you lighting? What specific visual quality made the lights feel "right"?]*

**Links:** [GitHub repo](https://github.com/cabranca/game-dev) | [Components.h](https://github.com/cabranca/game-dev/blob/main/Cabrankengine/src/Cabrankengine/ECS/Components.h)
