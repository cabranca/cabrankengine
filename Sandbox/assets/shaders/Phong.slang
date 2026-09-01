// Cross-backend Phong shader for Cabrankengine (Vulkan/Slang variant).
//
// Bindings:
//   set 0, binding 0  ConstantBuffer<SceneData>  view-projection, dir light, camera pos
//   set 1, binding 0  Sampler2D                  diffuse map
//   set 1, binding 1  Sampler2D                  specular map
//
// Push constants (one block, two stages):
//   vertex:    float4x4 model           bytes  0..63
//   fragment:  float    shininess       byte  64

struct DirLight {
    float3 direction;
    float  _pad0;
    float3 radiance;
    float  _pad1;
};

struct SceneData {
    float4x4 viewProjection;
    float3             cameraPosition;
    float              _pad2;
    DirLight           dirLight;
};

[[vk::binding(0, 0)]] ConstantBuffer<SceneData> scene;

[[vk::binding(0, 1)]] Sampler2D diffuseMap;
[[vk::binding(1, 1)]] Sampler2D specularMap;

// Slang doesn't accept GLSL-style buffer blocks with unbounded arrays, so we
// load the header and each light by byte offset.
[[vk::binding(0, 2)]] ByteAddressBuffer u_Lights;

static const uint k_PointLightStride = 48; // sizeof(PointLight) in std430

struct PushConstants {
    float4x4 model;
    float              shininess;
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
    float4 worldPos = mul(pc.model, float4(input.position, 1.0));
    o.worldPos     = worldPos.xyz;

    // TODO: SEE IF IT'S WORTH HANDLING NON UNIFORM SCALE (maybe check if uniform?)
    // Assumes pc.model has no non-uniform scale. For non-uniform scale the proper
    // normal transform is the inverse-transpose of model's upper-left 3x3.
    o.worldNormal  = normalize(mul(pc.model, float4(input.normal, 0.0)).xyz); // So the transpose is lacking here, right?
    o.texCoords    = input.texCoords;
    o.svPosition   = mul(scene.viewProjection, worldPos);
    
    // Vulkan NDC has Y pointing down (OpenGL has Y up). The engine's projection
    // matrix is built for OpenGL conventions, so flip Y in clip space here.
    o.svPosition.y = -o.svPosition.y;
    
    // Remap clip-space z from OpenGL's [-w, +w] to Vulkan's [0, +w] so that the
    // near half of the view frustum isn't clipped away.
    o.svPosition.z = (o.svPosition.z + o.svPosition.w) * 0.5;
    return o;
}

[shader("fragment")]
float4 main(VSOutput input) : SV_TARGET {
    float3 diffuseSample  = diffuseMap.Sample(input.texCoords).rgb;
    float3 specularSample = specularMap.Sample(input.texCoords).rgb;

    float3 N = normalize(input.worldNormal);
    // dirLight.direction is the direction light travels; lighting wants the
    // vector FROM the surface TO the light, so negate.
    float3 L = normalize(-scene.dirLight.direction);
    float3 V = normalize(scene.cameraPosition - input.worldPos);
    float3 H = normalize(L + V);

    float3 ambient  = 0.1 * diffuseSample;
    float3 diffuse  = max(dot(N, L), 0.0) * diffuseSample * scene.dirLight.radiance;
    float3 specular = pow(max(dot(N, H), 0.0), pc.shininess) * specularSample * scene.dirLight.radiance;

    return float4(ambient + diffuse + specular, 1.0);
}
