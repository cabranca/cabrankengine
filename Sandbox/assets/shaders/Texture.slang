// Batched 2D quad shader (Renderer2D) — Vulkan/Slang variant.
//
// Bindings:
//   set 0, binding 0  ConstantBuffer<SceneData>  shared scene UBO — only viewProjection used
//   set 1, binding 0  Sampler2D[32]              texture atlas slots
//
// No push constants: vertices are already in world space (CPU-side transform).

#pragma pack_matrix(row_major)

struct SceneData {
    row_major float4x4 viewProjection;
    // Full scene UBO contains additional fields; only viewProjection is read here.
};

[[vk::binding(0, 0)]] ConstantBuffer<SceneData> scene;
[[vk::binding(0, 1)]] Sampler2D textures[32];

struct VSInput {
    float3 position     : POSITION;
    float4 color        : COLOR;
    float2 texCoord     : TEXCOORD0;
    float  texIndex     : TEXCOORD1;
    float  tilingFactor : TEXCOORD2;
};

struct VSOutput {
    float4 svPosition   : SV_POSITION;
    float4 color;
    float2 texCoord;
    float  texIndex;
    float  tilingFactor;
};

[shader("vertex")]
VSOutput main(VSInput input) {
    VSOutput o;
    o.svPosition    = mul(float4(input.position, 1.0), scene.viewProjection);
    o.svPosition.y  = -o.svPosition.y;
    o.svPosition.z  = (o.svPosition.z + o.svPosition.w) * 0.5;
    o.color         = input.color;
    o.texCoord      = input.texCoord;
    o.texIndex      = input.texIndex;
    o.tilingFactor  = input.tilingFactor;
    return o;
}

[shader("fragment")]
float4 main(VSOutput input) : SV_TARGET {
    // texIndex varies per-fragment across quads, so the index is non-uniform
    // across the wave. NonUniformResourceIndex satisfies the Vulkan spec requirement
    // (shaderSampledImageArrayNonUniformIndexing must be enabled on the device).
    uint idx = (uint)input.texIndex;
    return textures[NonUniformResourceIndex(idx)].Sample(input.texCoord * input.tilingFactor) * input.color;
}
