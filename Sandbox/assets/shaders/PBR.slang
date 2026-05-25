// Cross-backend PBR shader for Cabrankengine (Vulkan/Slang variant).
//
// Cook-Torrance PBR: directional light + N point lights + ambient/AO. Matches
// PBR.glsl byte-for-byte on the SSBO layout so Renderer::uploadLightEnvironment
// can feed either backend.
//
// Bindings:
//   set 0, binding 0  ConstantBuffer<SceneData>  view-projection, dir light, camera pos
//   set 1, binding 0  Sampler2D                  albedo map
//   set 1, binding 1  Sampler2D                  normal map
//   set 1, binding 2  Sampler2D                  metalRoughness map (G=rough, B=metal)
//   set 1, binding 3  Sampler2D                  ambient occlusion map (R)
//   set 2, binding 0  StorageBuffer<LightBuffer> point-light count + array (std430)
//
// Push constants:
//   vertex:    float4x4 model           bytes  0..63
//   fragment:  float3   albedoColor     bytes 64..75
//              float    metalness       byte  76
//              float    roughness       byte  80

struct DirLight {
    float3 direction;
    float  _pad0;
    float3 radiance;
    float  _pad1;
};

struct SceneData {
    float4x4 viewProjection;
    DirLight           dirLight;
    float3             cameraPosition;
    float              _pad2;
};

[[vk::binding(0, 0)]] ConstantBuffer<SceneData> scene;

[[vk::binding(0, 1)]] Sampler2D albedoMap;
[[vk::binding(1, 1)]] Sampler2D normalMap;
[[vk::binding(2, 1)]] Sampler2D metalRoughMap;
[[vk::binding(3, 1)]] Sampler2D aoMap;

// Mirrors PointLightGPU in Renderer.cpp (std430, 48 bytes per element).
struct PointLight {
    float3 position;
    float  _pad0;
    float3 radiance;
    float  _pad1;
    float  constant;
    float  linear;
    float  quadratic;
    float  _pad2;
};

// Raw byte view of the SSBO produced by Renderer::uploadLightEnvironment:
// 16-byte LightBufferHeader (int count + int3 pad) followed by PointLight[].
// Slang doesn't accept GLSL-style buffer blocks with unbounded arrays, so we
// load the header and each light by byte offset.
[[vk::binding(0, 2)]] ByteAddressBuffer u_Lights;

static const uint k_PointLightStride = 48; // sizeof(PointLight) in std430

struct PushConstants {
    float4x4 model;
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
	float3x3 tbn;
};

[shader("vertex")]
VSOutput main(VSInput input) {
	VSOutput o;
	float4 worldPos = mul(pc.model, float4(input.position, 1.0));
    o.worldPos     = worldPos.xyz;

	// TODO: SEE IF IT'S WORTH HANDLING NON UNIFORM SCALE (maybe check if uniform?)
	// Assumes pc.model has no non-uniform scale. For non-uniform scale the proper
	// normal transform is the inverse-transpose of model's upper-left 3x3.
	o.worldNormal = normalize(mul(pc.model, float4(input.normal, 0.0)).xyz);
	o.texCoords = input.texCoords;
	o.svPosition = mul(scene.viewProjection, worldPos);
    
    // Are these modifications due to my perspective or view matrices?
    // Vulkan NDC has Y down (OpenGL has Y up). Flip in clip space.
    o.svPosition.y = -o.svPosition.y;
    
    // OpenGL clip-space z is [-w, +w]; Vulkan is [0, +w]. Remap.
    o.svPosition.z = (o.svPosition.z + o.svPosition.w) * 0.5;

	// Construct TBN matrix from tangent attribute
	float3 T = normalize(mul(pc.model, float4(input.tangent, 0.0)).xyz);
	float3 N = normalize(o.worldNormal);

	// Gram-Schmidt to orthogonalize T from N
	T = normalize(T - dot(T, N) * N);
	float3 B = cross(N, T);
	o.tbn = float3x3(T, B, N);

    return o;
}



static const float PI = 3.14159265359;

// Trowbridge-Reitz GGX normal distribution.
float DistributionGGX(float3 N, float3 H, float roughness) {
    float a      = roughness * roughness;
    float a2     = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom       = PI * denom * denom;

    return num / max(denom, 0.0000001);
}

// Smith's Schlick-GGX geometry term (single direction).
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// Fresnel-Schlick approximation.
float3 fresnelSchlick(float cosTheta, float3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Sample tangent-space normal map and transform into world space via TBN.
float3 getNormalFromMap(VSOutput input) {
    float3 tangentNormal = normalMap.Sample(input.texCoords).xyz * 2.0 - 1.0;
    return normalize(mul(tangentNormal, input.tbn));
}

[shader("fragment")]
float4 main(VSOutput input) : SV_TARGET {
    // 1. Material inputs (sRGB albedo -> linear).
    float3 albedo = pow(albedoMap.Sample(input.texCoords).rgb, float3(2.2)) * pc.albedoColor;

    // GLTF packing: G = roughness, B = metalness.
    float4 mrSample  = metalRoughMap.Sample(input.texCoords);
    float  metallic  = mrSample.b * pc.metalness;
    float  roughness = mrSample.g * pc.roughness;
    float  ao        = aoMap.Sample(input.texCoords).r;

    // 2. Principal vectors.
    float3 N = getNormalFromMap(input);
    float3 V = normalize(scene.cameraPosition - input.worldPos);

    // F0: 0.04 for dielectrics, albedo for metals.
    float3 F0 = float3(0.04);
    F0        = lerp(F0, albedo, metallic);

    float3 Lo = float3(0.0);

    // A. Directional light (Cook-Torrance).
    {
        float3 L        = normalize(-scene.dirLight.direction);
        float3 H        = normalize(V + L);
        float3 radiance = scene.dirLight.radiance;

        float  NDF = DistributionGGX(N, H, roughness);
        float  G   = GeometrySmith(N, V, L, roughness);
        float3 F   = fresnelSchlick(max(dot(H, V), 0.0), F0);

        float3 numerator   = NDF * G * F;
        float  denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        float3 specular    = numerator / denominator;

        float3 kS = F;
        float3 kD = float3(1.0) - kS;
        kD       *= 1.0 - metallic;

        float NdotL = max(dot(N, L), 0.0);
        Lo         += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    // B. Point lights (SSBO).
    int pointLightCount = u_Lights.Load<int>(0);
    for (int i = 0; i < pointLightCount; ++i) {
        PointLight light = u_Lights.Load<PointLight>(16 + i * k_PointLightStride);
        float3     L     = normalize(light.position - input.worldPos);
        float3     H     = normalize(V + L);

        float  distance    = length(light.position - input.worldPos);
        float  attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
        float3 radiance    = light.radiance * attenuation;

        float  NDF = DistributionGGX(N, H, roughness);
        float  G   = GeometrySmith(N, V, L, roughness);
        float3 F   = fresnelSchlick(max(dot(H, V), 0.0), F0);

        float3 numerator   = NDF * G * F;
        float  denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        float3 specular    = numerator / denominator;

        float3 kS = F;
        float3 kD = float3(1.0) - kS;
        kD       *= 1.0 - metallic;

        float NdotL = max(dot(N, L), 0.0);
        Lo         += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    // Ambient placeholder (until IBL).
    float3 ambient = float3(0.03) * albedo * ao;
    float3 color   = ambient + Lo;

    // Reinhard tone mapping + gamma correction.
    color = color / (color + float3(1.0));
    color = pow(color, float3(1.0 / 2.2));

    return float4(color, 1.0);
}
