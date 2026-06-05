#include <metal_stdlib>
using namespace metal;

// Metal text/glyph shader. Mirrors Texture.metal but the fragment treats the
// sampled red channel as glyph coverage modulating the vertex-color alpha.
//
// Must match TextVertex in TextRenderer.cpp:
//   { Float3 pos, Float4 color, Float2 texCoord, Float texIndex }
struct VertexIn {
    float3 position [[attribute(0)]];
    float4 color    [[attribute(1)]];
    float2 texCoord [[attribute(2)]];
    float  texIndex [[attribute(3)]];
};

struct VertexOut {
    float4 position [[position]];
    float4 color;
    float2 texCoord;
    float  texIndex;
};

// View-projection at buffer(30) — matches MetalTextMaterial (buffer_index::k_Scene).
struct Uniforms {
    float4x4 viewProjection;
};

vertex VertexOut vertex_main(VertexIn in [[stage_in]],
                             constant Uniforms& uniforms [[buffer(30)]]) {
    VertexOut out;
    out.position = uniforms.viewProjection * float4(in.position, 1.0);
    out.color    = in.color;
    out.texCoord = in.texCoord;
    out.texIndex = in.texIndex;
    return out;
}

fragment float4 fragment_main(VertexOut in [[stage_in]],
                              array<texture2d<float>, 32> textures [[texture(0)]]) {
    constexpr sampler texSampler(mag_filter::linear, min_filter::linear, address::clamp_to_edge);
    int idx = int(in.texIndex);
    float coverage = textures[idx].sample(texSampler, in.texCoord).r;
    return float4(in.color.rgb, in.color.a * coverage);
}
