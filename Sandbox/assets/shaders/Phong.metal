#include <metal_stdlib>
using namespace metal;

// Metal Phong shader. Argument-table layout (see MetalBinding.h):
//   vertex   buffer(0)   [[stage_in]] vertex data (common::Vertex)
//   vertex   buffer(30)  SceneData  (view-projection)
//   vertex   buffer(29)  float4x4   model
//   fragment buffer(30)  SceneData  (camera position + directional light)
//   fragment buffer(29)  float      shininess
//   fragment texture(0)  diffuse    texture(1) specular
//
// SceneData mirrors AltSceneData in Renderer.cpp (112 bytes); float3 members are
// 16-byte aligned to match the engine's explicit padding.

struct VertexIn {
    float3 position  [[attribute(0)]];
    float3 normal    [[attribute(1)]];
    float2 texCoords [[attribute(2)]];
    float3 tangent   [[attribute(3)]];
};

struct DirLight {
    float3 direction;
    float3 radiance;
};

struct SceneData {
    float4x4 viewProjection;
    DirLight dirLight;
    float3   cameraPosition;
};

struct VertexOut {
    float4 position [[position]];
    float3 worldPos;
    float3 worldNormal;
    float2 texCoords;
};

vertex VertexOut vertex_main(VertexIn in [[stage_in]],
                             constant SceneData& scene [[buffer(30)]],
                             constant float4x4& model  [[buffer(29)]]) {
    VertexOut o;
    float4 worldPos = model * float4(in.position, 1.0);
    o.worldPos    = worldPos.xyz;
    // Assumes no non-uniform scale (otherwise the inverse-transpose is required).
    o.worldNormal = normalize((model * float4(in.normal, 0.0)).xyz);
    o.texCoords   = in.texCoords;
    o.position    = scene.viewProjection * worldPos;

    // The engine's projection is OpenGL-convention (clip z in [-w, w]); Metal clips
    // to [0, w]. Remap so the near half of the frustum isn't clipped. No Y-flip is
    // needed: Metal NDC +Y is up, like OpenGL (unlike Vulkan).
    o.position.z = (o.position.z + o.position.w) * 0.5;
    return o;
}

fragment float4 fragment_main(VertexOut in [[stage_in]],
                              constant SceneData& scene  [[buffer(30)]],
                              constant float& shininess  [[buffer(29)]],
                              texture2d<float> diffuseMap  [[texture(0)]],
                              texture2d<float> specularMap [[texture(1)]]) {
    constexpr sampler texSampler(mag_filter::linear, min_filter::linear, mip_filter::linear, address::repeat);

    float3 diffuseSample  = diffuseMap.sample(texSampler, in.texCoords).rgb;
    float3 specularSample = specularMap.sample(texSampler, in.texCoords).rgb;

    float3 N = normalize(in.worldNormal);
    // dirLight.direction is the direction light travels; lighting wants the vector
    // from the surface toward the light, so negate.
    float3 L = normalize(-scene.dirLight.direction);
    float3 V = normalize(scene.cameraPosition - in.worldPos);
    float3 H = normalize(L + V);

    float3 ambient  = 0.1 * diffuseSample;
    float3 diffuse  = max(dot(N, L), 0.0) * diffuseSample * scene.dirLight.radiance;
    float3 specular = pow(max(dot(N, H), 0.0), shininess) * specularSample * scene.dirLight.radiance;

    return float4(ambient + diffuse + specular, 1.0);
}
