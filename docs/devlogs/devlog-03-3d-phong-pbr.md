# Devlog #3 — 3D Rendering: Phong and PBR Materials

> **[PERSONALIZE]** Add a line about what 3D asset you first loaded — what model was it, where did you get it? What was broken about how it looked before you fixed the shaders?

---

Going from 2D to 3D is not just "add a Z coordinate." The entire lighting model changes. In 2D, a sprite is just a textured quad. In 3D, surfaces have normals, materials respond differently to light based on angle and distance, and you have to make a fundamental choice about what kind of rendering you want.

I ended up implementing two material systems: Phong for a simpler classic model, and PBR for physically-based rendering.

## Phong shading

Phong is the classic real-time lighting model taught in every graphics course. Each fragment color is the sum of three terms:

- **Ambient** — a flat base color so nothing is pitch black
- **Diffuse** — Lambert's law: `max(dot(N, L), 0) * lightColor * materialDiffuse`
- **Specular** — the bright highlight: `pow(max(dot(R, V), 0), shininess) * lightColor * materialSpecular`

where `N` is the surface normal, `L` is the direction to the light, `R` is the reflected light direction, and `V` is the view direction.

It's not physically correct but it's cheap and looks fine for many use cases. I used it for opaque 3D models where I didn't need realistic material response.

## PBR — Physically Based Rendering

PBR replaces the empirical Phong model with one grounded in the physics of light-matter interaction. The key insight is the reflectance equation:

```
Lo(v) = ∫ fr(l, v) * Li(l) * (n·l) dl
```

In practice you approximate this with a BRDF (Bidirectional Reflectance Distribution Function) that has two terms:

- **Diffuse term** — Lambertian, adjusted for energy conservation with the Fresnel factor
- **Specular term** — Cook-Torrance: uses a Normal Distribution Function (GGX/Trowbridge-Reitz), a Geometry function (Smith/Schlick-GGX), and the Fresnel equation (Schlick approximation)

Materials are described by two scalars — **roughness** (0 = mirror, 1 = fully diffuse) and **metalness** (0 = dielectric, 1 = conductor) — plus an albedo (base color). These map to real-world intuitions: rough metal looks like brushed steel, smooth metal looks like chrome, and non-metals behave like diffuse fabric or plastic.

The inputs are textures:

```
Albedo map       — base color
Normal map       — surface detail (tangent space)
Metal/rough map  — R=0, G=roughness, B=metalness (glTF packing)
AO map           — ambient occlusion for contact shadows
```

> **[ADD SCREENSHOT: a PBR-shaded model — the gun or backpack, showing material response to the directional light]**

## Two separate component types

I kept Phong and PBR as separate components (`CPhongModel`, `CPBRModel`) with separate render systems. This makes the code clearer — a PBR system only processes PBR entities, no runtime branching — and lets the ECS signature system route entities to the right system automatically.

```cpp
// Phong entity
PhongModelArch backpack{ "assets/models/backpack/backpack.cbkm" };

// PBR entity
PBRModelArch gun{ "assets/models/gun/Cerberus_LP.cbkm" };
```

## One thing that took longer than expected

Tangent space normal mapping. Getting the TBN matrix (Tangent-Bitangent-Normal) right — especially when Assimp generates tangents with a different handedness convention than your shader expects — costs a few hours of "why does the normal map look inside-out on half the faces."

The fix: make sure Assimp's `aiProcess_CalcTangentSpace` flag is on, and flip the green channel of the normal map if your source art uses DirectX convention instead of OpenGL convention.

## What's next

Models needed cameras to look good. Next I built the camera system — which turned out to be a good test of the ECS composition pattern.

---

*[PERSONALIZE: describe the moment when PBR started looking visually correct. What asset were you looking at?]*

**Links:** [GitHub repo](https://github.com/cabranca/game-dev) | [Materials source](https://github.com/cabranca/game-dev/tree/main/Cabrankengine/src/Cabrankengine/Renderer/Materials)
