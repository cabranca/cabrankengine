// Cross-backend PBR shader for Cabrankengine (Vulkan/Slang variant).
//
// Placeholder lighting model — currently directional-light diffuse + ambient
// + AO modulation. Full Cook-Torrance evaluation (Fresnel/GGX/Smith) is TODO.
//
// Bindings:
//   set 0, binding 0  ConstantBuffer<SceneData>  view-projection, dir light, camera pos
//   set 1, binding 0  Sampler2D                  albedo map
//   set 1, binding 1  Sampler2D                  normal map
//   set 1, binding 2  Sampler2D                  metalRoughness map (G=rough, B=metal)
//   set 1, binding 3  Sampler2D                  ambient occlusion map (R)
//
// Push constants:
//   vertex:    float4x4 model           bytes  0..63
//   fragment:  float3   albedoColor     bytes 64..75
//              float    metalness       byte  76
//              float    roughness       byte  80

// Engine matrices are stored row-major with row-vector convention (v * M).
// Slang defaults to column-major memory layout, which would transpose every
// matrix on read. Force row-major here.
#pragma pack_matrix(row_major)

struct DirLight {
    float3 direction;
    float  _pad0;
    float3 radiance;
    float  _pad1;
};

struct SceneData {
    row_major float4x4 viewProjection;
    DirLight           dirLight;
    float3             cameraPosition;
    float              _pad2;
};

[[vk::binding(0, 0)]] ConstantBuffer<SceneData> scene;

[[vk::binding(0, 1)]] Sampler2D albedoMap;
[[vk::binding(1, 1)]] Sampler2D normalMap;
[[vk::binding(2, 1)]] Sampler2D metalRoughMap;
[[vk::binding(3, 1)]] Sampler2D aoMap;

struct PushConstants {
    row_major float4x4 model;
    float3             albedoColor;
    float              metalness;
    float              roughness;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> pc;

struct VSInput {
    float3 position;
    float3 normal;
    float2 texCoords;
    float3 tangent;
};

struct VSOutput {
    float4 svPosition : SV_POSITION;
    float3 worldPos;
    float3 worldNormal;
    float2 texCoords;
};

[shader("vertex")]
VSOutput main(VSInput input) {
    VSOutput o;
    float4 worldPos = mul(float4(input.position, 1.0), pc.model);
    o.worldPos     = worldPos.xyz;
    // Assumes pc.model has no non-uniform scale. For non-uniform scale the proper
    // normal transform is the inverse-transpose of model's upper-left 3x3.
    o.worldNormal  = normalize(mul(float4(input.normal, 0.0), pc.model).xyz);
    o.texCoords    = input.texCoords;
    o.svPosition   = mul(worldPos, scene.viewProjection);
    // Vulkan NDC has Y down (OpenGL has Y up). Flip in clip space.
    o.svPosition.y = -o.svPosition.y;
    // OpenGL clip-space z is [-w, +w]; Vulkan is [0, +w]. Remap.
    o.svPosition.z = (o.svPosition.z + o.svPosition.w) * 0.5;
    return o;
}

[shader("fragment")]
float4 main(VSOutput input) : SV_TARGET {
    float3 albedo = albedoMap.Sample(input.texCoords).rgb * pc.albedoColor;
    float  ao     = aoMap.Sample(input.texCoords).r;

    float3 N = normalize(input.worldNormal);
    // dirLight.direction is the direction light travels; we want surface-to-light.
    float3 L = normalize(-scene.dirLight.direction);

    float3 ambient = 0.1 * albedo * ao;
    float3 diffuse = max(dot(N, L), 0.0) * albedo * scene.dirLight.radiance;

    return float4(ambient + diffuse, 1.0);
}
