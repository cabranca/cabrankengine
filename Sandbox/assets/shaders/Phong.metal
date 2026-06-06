#include <metal_stdlib>
using namespace metal;

// Metal Phong shader. Argument-table layout (see MetalBinding.h):
//   vertex   buffer(0)   [[stage_in]] vertex data (common::Vertex)
//   vertex   buffer(30)  SceneData  (view-projection)
//   vertex   buffer(29)  float4x4   model
//   fragment buffer(30)  SceneData  (camera position + directional light)
//   fragment buffer(29)  float      shininess
//   fragment buffer(28)  LightBuffer (count header + PointLight[64])
//   fragment texture(0)  diffuse    texture(1) specular
//
// SceneData mirrors AltSceneData in Renderer.cpp (112 bytes); LightBuffer mirrors
// the std430 SSBO. float3 members are 16-byte aligned to match the engine's
// explicit padding.

struct VertexIn {
    float3 position  [[attribute(0)]];
    float3 normal    [[attribute(1)]];
    float2 texCoords [[attribute(2)]];
    // tangent (attribute 3) is in the shared vertex descriptor but unused by Phong.
};

struct DirLight {
    float3 direction;
    float3 radiance;
};

// 'constant' is a reserved address-space keyword in MSL, so the attenuation terms
// use att* names (matching PBR.metal). Mirrors PointLightGPU in Renderer.cpp (48 B).
struct PointLight {
    float3 position;
    float3 radiance;

    float attConstant;
    float attLinear;
    float attQuadratic;
    float _pad;
};

struct SceneData {
    float4x4 viewProjection;
    DirLight dirLight;
    float3   cameraPosition;
};

// MSL is C++14-based and has no flexible array members, so the array is fixed at
// the engine's k_MaxPointLights cap. Mirrors LightBufferHeader + PointLightGPU[].
struct LightBuffer {
    int count;
    int pad[3];
    PointLight lights[64];
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
    // MSL has no inverse(); assume uniform scale so the upper 3x3 of model transforms
    // normals correctly. For non-uniform scale, pass a CPU-computed normal matrix.
    o.worldNormal = normalize((model * float4(in.normal, 0.0)).xyz);
    o.texCoords   = in.texCoords;
    o.position    = scene.viewProjection * worldPos;

    // The engine's projection is OpenGL-convention (clip z in [-w, w]); Metal clips
    // to [0, w]. Remap so the near half of the frustum isn't clipped. No Y-flip is
    // needed: Metal NDC +Y is up, like OpenGL (unlike Vulkan).
    o.position.z = (o.position.z + o.position.w) * 0.5;
    return o;
}

/*
    lightDir: vector from light source to fragment
    lightRadiance: light source color
    normal: normalized normal vector to the fragment's surface
    viewDir: normalized vector from camera to fragment
*/
// Unlike GLSL, MSL has no global samplers/textures/uniforms — they only exist as
// function arguments. The helpers therefore take the already-sampled material
// colors (and shininess) so they don't need access to the textures themselves.
float3 calcLightIncidence(float3 lightDir, float3 lightRadiance, float3 normal, float3 viewDir,
                          float3 diffuseSample, float3 specularSample, float shininess);
float3 calcPointLight(PointLight light, float3 normal, float3 fragPos, float3 viewDir,
                      float3 diffuseSample, float3 specularSample, float shininess);

fragment float4 fragment_main(VertexOut in [[stage_in]],
                              constant SceneData& scene         [[buffer(30)]],
                              constant float& shininess         [[buffer(29)]],
                              constant LightBuffer& pointLights [[buffer(28)]],
                              texture2d<float> diffuseMap       [[texture(0)]],
                              texture2d<float> specularMap      [[texture(1)]]) {
    constexpr sampler texSampler(mag_filter::linear, min_filter::linear, mip_filter::linear, address::repeat);

    float3 N = normalize(in.worldNormal);
    float3 V = normalize(in.worldPos - scene.cameraPosition); // From camera to fragment

    // Sample the material maps once; the lighting helpers reuse these.
    float3 diffuseSample  = diffuseMap.sample(texSampler, in.texCoords).rgb;
    float3 specularSample = specularMap.sample(texSampler, in.texCoords).rgb;

    // Ambient component
    float ambientFactor = 0.05;
    float3 result = ambientFactor * diffuseSample;

    result += calcLightIncidence(scene.dirLight.direction, scene.dirLight.radiance, N, V,
                                 diffuseSample, specularSample, shininess);
    for (int i = 0; i < pointLights.count; i++)
        result += calcPointLight(pointLights.lights[i], N, in.worldPos, V,
                                 diffuseSample, specularSample, shininess);

    // Tone mapping + gamma correction
    result = result / (result + float3(1.0));
    result = pow(result, float3(1.0 / 2.2));
    return float4(result, 1.0);
}

float3 calcLightIncidence(float3 lightDir, float3 lightRadiance, float3 normal, float3 viewDir,
                          float3 diffuseSample, float3 specularSample, float shininess) {
    // Normalize: callers may pass an unnormalized direction (e.g. the directional
    // light's raw direction). A non-unit lightDir leaves reflectDir non-unit, so the
    // specular dot can exceed 1 and pow() blows the highlight out to white.
    lightDir = normalize(lightDir);

    // Diffuse component
    float diffuseFactor = max(dot(normal, -lightDir), 0.0);
    float3 diffuse = lightRadiance * diffuseFactor * diffuseSample;

    // Specular component — only on the lit hemisphere, so it can't leak onto faces
    // turned away from the light.
    float3 specular = float3(0.0);
    if (diffuseFactor > 0.0) {
        float3 reflectDir = reflect(lightDir, normal);
        float specularFactor = pow(max(dot(-viewDir, reflectDir), 0.0), max(shininess, 1.0));
        specular = lightRadiance * specularFactor * specularSample;
    }

    return diffuse + specular;
}

float3 calcPointLight(PointLight light, float3 normal, float3 fragPos, float3 viewDir,
                      float3 diffuseSample, float3 specularSample, float shininess) {
    float3 lightDir = normalize(fragPos - light.position);
    float3 baseLightIncidence = calcLightIncidence(lightDir, light.radiance, normal, viewDir,
                                                   diffuseSample, specularSample, shininess);

    float distance = length(fragPos - light.position);
    float attenuation = 1.0 / (light.attConstant + light.attLinear * distance + light.attQuadratic * distance * distance);

    return baseLightIncidence * attenuation;
}
