#include <metal_stdlib>
using namespace metal;

// Fallback shader (DefaultLibrary loads "Error"). No Metal material builds a
// pipeline from it; this exists so the library compiles and DefaultLibrary::init
// doesn't fail. Renders flat magenta if ever used.
struct VertexIn {
    float3 position [[attribute(0)]];
};

struct VertexOut {
    float4 position [[position]];
};

vertex VertexOut vertex_main(VertexIn in [[stage_in]]) {
    VertexOut out;
    out.position = float4(in.position, 1.0);
    return out;
}

fragment float4 fragment_main() {
    return float4(1.0, 0.0, 1.0, 1.0);
}
