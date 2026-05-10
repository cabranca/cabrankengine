#type vertex
#version 300 es
precision highp float;

layout (location = 0) in vec3 pos;
layout (location = 1) in vec4 color;
layout (location = 2) in vec2 texCoords;
layout (location = 3) in float texIndex;

uniform mat4 u_ViewProjection;

out vec4 v_Color;
out vec2 v_TexCoords;
out float v_TexIndex;

void main()
{
    v_Color = color;
    v_TexCoords = texCoords;
    v_TexIndex = texIndex;

    gl_Position = u_ViewProjection * vec4(pos, 1.0);
}

#type fragment
#version 300 es
precision mediump float;

layout(location = 0) out vec4 color;

in vec4 v_Color;
in vec2 v_TexCoords;
in float v_TexIndex;

uniform sampler2D u_Textures[32];

// ESSL 3.00 forbids dynamic indexing of sampler arrays in fragment shaders, so unroll.
// Glyph atlases are single-channel (R8) — sample only the red channel.
float sampleAlphaByIndex(int idx, vec2 uv)
{
    switch (idx) {
        case  0: return texture(u_Textures[ 0], uv).r;
        case  1: return texture(u_Textures[ 1], uv).r;
        case  2: return texture(u_Textures[ 2], uv).r;
        case  3: return texture(u_Textures[ 3], uv).r;
        case  4: return texture(u_Textures[ 4], uv).r;
        case  5: return texture(u_Textures[ 5], uv).r;
        case  6: return texture(u_Textures[ 6], uv).r;
        case  7: return texture(u_Textures[ 7], uv).r;
        case  8: return texture(u_Textures[ 8], uv).r;
        case  9: return texture(u_Textures[ 9], uv).r;
        case 10: return texture(u_Textures[10], uv).r;
        case 11: return texture(u_Textures[11], uv).r;
        case 12: return texture(u_Textures[12], uv).r;
        case 13: return texture(u_Textures[13], uv).r;
        case 14: return texture(u_Textures[14], uv).r;
        case 15: return texture(u_Textures[15], uv).r;
        case 16: return texture(u_Textures[16], uv).r;
        case 17: return texture(u_Textures[17], uv).r;
        case 18: return texture(u_Textures[18], uv).r;
        case 19: return texture(u_Textures[19], uv).r;
        case 20: return texture(u_Textures[20], uv).r;
        case 21: return texture(u_Textures[21], uv).r;
        case 22: return texture(u_Textures[22], uv).r;
        case 23: return texture(u_Textures[23], uv).r;
        case 24: return texture(u_Textures[24], uv).r;
        case 25: return texture(u_Textures[25], uv).r;
        case 26: return texture(u_Textures[26], uv).r;
        case 27: return texture(u_Textures[27], uv).r;
        case 28: return texture(u_Textures[28], uv).r;
        case 29: return texture(u_Textures[29], uv).r;
        case 30: return texture(u_Textures[30], uv).r;
        default: return texture(u_Textures[31], uv).r;
    }
}

void main()
{
    vec4 sampled = vec4(1.0, 1.0, 1.0, sampleAlphaByIndex(int(v_TexIndex), v_TexCoords));
    color = v_Color * sampled;
}
