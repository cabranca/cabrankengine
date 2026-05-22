// Batched text shader (TextRenderer) — Vulkan/Slang variant.
//
// Bindings:
//   set 0, binding 0  ConstantBuffer<SceneData>  shared scene UBO — only viewProjection used
//   set 1, binding 0  Sampler2D[32]              per-glyph greyscale textures
//
// Glyph textures are R8 — the red channel is treated as alpha and multiplied
// by the vertex color so the mesh color controls the text color.

struct SceneData {
    float4x4 viewProjection;
    // Full scene UBO contains additional fields; only viewProjection is read here.
};

[[vk::binding(0, 0)]] ConstantBuffer<SceneData> scene;
[[vk::binding(0, 1)]] Sampler2D textures[32];

struct VSInput {
    float3 position : POSITION;
    float4 color    : COLOR;
    float2 texCoord : TEXCOORD0;
    float  texIndex : TEXCOORD1;
};

struct VSOutput {
    float4 svPosition : SV_POSITION;
    float4 color;
    float2 texCoord;
    float  texIndex;
};

[shader("vertex")]
VSOutput main(VSInput input) {
    VSOutput o;
    o.svPosition   = mul(scene.viewProjection, float4(input.position, 1.0));
    o.svPosition.y = -o.svPosition.y;
    o.svPosition.z = (o.svPosition.z + o.svPosition.w) * 0.5;
    o.color        = input.color;
    o.texCoord     = input.texCoord;
    o.texIndex     = input.texIndex;
    return o;
}

[shader("fragment")]
float4 main(VSOutput input) : SV_TARGET {
    uint  idx   = (uint)input.texIndex;
    float alpha = textures[NonUniformResourceIndex(idx)].Sample(input.texCoord).r;
    return float4(input.color.rgb, input.color.a * alpha);
}
