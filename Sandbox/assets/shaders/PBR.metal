#include <metal_stdlib>
using namespace metal;

// Metal Cook-Torrance PBR shader: directional light + N point lights + ambient/AO.
// Argument-table layout (see MetalBinding.h):
//   vertex   buffer(0)   [[stage_in]] vertex data (common::Vertex)
//   vertex   buffer(30)  SceneData  (view-projection)
//   vertex   buffer(29)  float4x4   model
//   fragment buffer(30)  SceneData  (camera position + directional light)
//   fragment buffer(29)  PushData   (albedo color, metalness, roughness)
//   fragment buffer(28)  LightBuffer (count header + PointLight[64])
//   fragment texture(0)  albedo  (1) normal  (2) metalRough (G=rough,B=metal)  (3) AO
//
// SceneData / LightBuffer mirror AltSceneData and the std430 SSBO in Renderer.cpp.

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

// Matches MetalPBRMaterial::PushData (packed_float3 + two floats = 20 bytes).
struct PushData {
    packed_float3 albedoColor;
    float metalness;
    float roughness;
};

// Mirrors PointLightGPU in Renderer.cpp (std430, 48 bytes). float3 members occupy
// 16-byte slots, matching the engine's xyz+pad packing.
struct PointLight {
    float3 position;
    float3 radiance;
    float  attConstant;
    float  attLinear;
    float  attQuadratic;
    float  _pad;
};

// Mirrors LightBufferHeader (int count + int[3]) followed by the light array.
struct LightBuffer {
    int count;
    int _p0;
    int _p1;
    int _p2;
    PointLight lights[64];
};

// Metal forbids matrix types as [[stage_in]] varyings, so the TBN basis is passed
// as its three column vectors and reassembled into a float3x3 in the fragment stage.
struct VertexOut {
    float4 position [[position]];
    float3 worldPos;
    float3 worldNormal;
    float2 texCoords;
    float3 tbnT;
    float3 tbnB;
    float3 tbnN;
};

vertex VertexOut vertex_main(VertexIn in [[stage_in]],
                             constant SceneData& scene [[buffer(30)]],
                             constant float4x4& model  [[buffer(29)]]) {
    VertexOut o;
    float4 worldPos = model * float4(in.position, 1.0);
    o.worldPos    = worldPos.xyz;
    o.worldNormal = normalize((model * float4(in.normal, 0.0)).xyz);
    o.texCoords   = in.texCoords;
    o.position    = scene.viewProjection * worldPos;
    // GL->Metal clip-space z remap (see Phong.metal). No Y-flip on Metal.
    o.position.z  = (o.position.z + o.position.w) * 0.5;

    // TBN from the tangent attribute (Gram-Schmidt orthogonalised).
    float3 T = normalize((model * float4(in.tangent, 0.0)).xyz);
    float3 N = normalize(o.worldNormal);
    T = normalize(T - dot(T, N) * N);
    float3 B = cross(N, T);
    o.tbnT = T;
    o.tbnB = B;
    o.tbnN = N;
    return o;
}

constant float PI = 3.14159265359;

static float DistributionGGX(float3 N, float3 H, float roughness) {
    float a      = roughness * roughness;
    float a2     = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom  = (NdotH2 * (a2 - 1.0) + 1.0);
    denom        = PI * denom * denom;
    return a2 / max(denom, 0.0000001);
}

static float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

static float GeometrySmith(float3 N, float3 V, float3 L, float roughness) {
    float ggx2 = GeometrySchlickGGX(max(dot(N, V), 0.0), roughness);
    float ggx1 = GeometrySchlickGGX(max(dot(N, L), 0.0), roughness);
    return ggx1 * ggx2;
}

static float3 fresnelSchlick(float cosTheta, float3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Accumulate one light's Cook-Torrance contribution.
static float3 shade(float3 N, float3 V, float3 L, float3 radiance, float3 albedo,
                    float3 F0, float metallic, float roughness) {
    float3 H = normalize(V + L);
    float  NDF = DistributionGGX(N, H, roughness);
    float  G   = GeometrySmith(N, V, L, roughness);
    float3 F   = fresnelSchlick(max(dot(H, V), 0.0), F0);

    float3 numerator   = NDF * G * F;
    float  denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    float3 specular    = numerator / denominator;

    float3 kD = (float3(1.0) - F) * (1.0 - metallic);
    float  NdotL = max(dot(N, L), 0.0);
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

fragment float4 fragment_main(VertexOut in [[stage_in]],
                              constant SceneData& scene       [[buffer(30)]],
                              constant PushData& pc           [[buffer(29)]],
                              constant LightBuffer& lights    [[buffer(28)]],
                              texture2d<float> albedoMap      [[texture(0)]],
                              texture2d<float> normalMap      [[texture(1)]],
                              texture2d<float> metalRoughMap  [[texture(2)]],
                              texture2d<float> aoMap          [[texture(3)]]) {
    constexpr sampler texSampler(mag_filter::linear, min_filter::linear, mip_filter::linear, address::repeat);

    // Material inputs (sRGB albedo -> linear).
    float3 albedo = pow(albedoMap.sample(texSampler, in.texCoords).rgb, float3(2.2)) * float3(pc.albedoColor);

    // GLTF packing: G = roughness, B = metalness.
    float4 mrSample  = metalRoughMap.sample(texSampler, in.texCoords);
    float  metallic  = mrSample.b * pc.metalness;
    float  roughness = mrSample.g * pc.roughness;
    float  ao        = aoMap.sample(texSampler, in.texCoords).r;

    float3 tangentNormal = normalMap.sample(texSampler, in.texCoords).xyz * 2.0 - 1.0;
    float3x3 tbn = float3x3(in.tbnT, in.tbnB, in.tbnN);
    float3 N = normalize(tbn * tangentNormal);
    float3 V = normalize(scene.cameraPosition - in.worldPos);

    float3 F0 = mix(float3(0.04), albedo, metallic);
    float3 Lo = float3(0.0);

    // Directional light.
    Lo += shade(N, V, normalize(-scene.dirLight.direction), scene.dirLight.radiance, albedo, F0, metallic, roughness);

    // Point lights.
    for (int i = 0; i < lights.count; ++i) {
        PointLight light = lights.lights[i];
        float3 toLight   = light.position - in.worldPos;
        float  distance  = length(toLight);
        float  attenuation = 1.0 / (light.attConstant + light.attLinear * distance + light.attQuadratic * distance * distance);
        float3 radiance  = light.radiance * attenuation;
        Lo += shade(N, V, normalize(toLight), radiance, albedo, F0, metallic, roughness);
    }

    // Ambient placeholder (until IBL), Reinhard tone mapping + gamma.
    float3 ambient = float3(0.03) * albedo * ao;
    float3 color   = ambient + Lo;
    color = color / (color + float3(1.0));
    color = pow(color, float3(1.0 / 2.2));

    return float4(color, 1.0);
}
