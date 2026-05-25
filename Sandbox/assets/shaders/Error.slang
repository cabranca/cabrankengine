// Fallback shader: renders solid magenta so missing/broken materials are
// immediately obvious. Matches the cross-backend Slang conventions used by
// Phong.slang / PBR.slang so it can stand in for either.
//
// Bindings:
//   set 0, binding 0  ConstantBuffer<SceneData>  view-projection (only field read)
//
// Push constants:
//   vertex:    float4x4 model   bytes 0..63

struct DirLight {
    float3 direction;
    float  _pad0;
    float3 radiance;
    float  _pad1;
};

struct SceneData {
    float4x4 viewProjection;
    DirLight dirLight;
    float3   cameraPosition;
    float    _pad2;
};

[[vk::binding(0, 0)]] ConstantBuffer<SceneData> scene;

struct PushConstants {
    float4x4 model;
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
};

[shader("vertex")]
VSOutput main(VSInput input) {
    VSOutput o;
    float4 worldPos = mul(pc.model, float4(input.position, 1.0));
    o.svPosition    = mul(scene.viewProjection, worldPos);
    return o;
}

[shader("fragment")]
float4 main(VSOutput input) : SV_TARGET {
    return float4(1.0, 0.0, 1.0, 1.0);
}
