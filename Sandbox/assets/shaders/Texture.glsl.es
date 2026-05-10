#type vertex
#version 300 es
precision highp float;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec4 col;
layout(location = 2) in vec2 tex;
layout(location = 3) in float texIndex;
layout(location = 4) in float tilingFactor;

uniform mat4 u_ViewProjection;

out vec2 v_TexCoord;
out vec4 v_Color;
out float v_TexIndex;
out float v_TilingFactor;

void main()
{
	v_TexCoord = tex;
	v_Color = col;
	v_TexIndex = texIndex;
	v_TilingFactor = tilingFactor;
	gl_Position = u_ViewProjection * vec4(pos, 1.0);
}

#type fragment
#version 300 es
precision mediump float;

layout(location = 0) out vec4 color;

in vec4 v_Color;
in vec2 v_TexCoord;
in float v_TexIndex;
in float v_TilingFactor;

uniform sampler2D u_Textures[32];

// ESSL 3.00 forbids dynamic indexing of sampler arrays in fragment shaders, so unroll
// the lookup. Index range mirrors Renderer2DData::MaxTextureSlots.
vec4 sampleByIndex(int idx, vec2 uv)
{
	switch (idx) {
		case  0: return texture(u_Textures[ 0], uv);
		case  1: return texture(u_Textures[ 1], uv);
		case  2: return texture(u_Textures[ 2], uv);
		case  3: return texture(u_Textures[ 3], uv);
		case  4: return texture(u_Textures[ 4], uv);
		case  5: return texture(u_Textures[ 5], uv);
		case  6: return texture(u_Textures[ 6], uv);
		case  7: return texture(u_Textures[ 7], uv);
		case  8: return texture(u_Textures[ 8], uv);
		case  9: return texture(u_Textures[ 9], uv);
		case 10: return texture(u_Textures[10], uv);
		case 11: return texture(u_Textures[11], uv);
		case 12: return texture(u_Textures[12], uv);
		case 13: return texture(u_Textures[13], uv);
		case 14: return texture(u_Textures[14], uv);
		case 15: return texture(u_Textures[15], uv);
		case 16: return texture(u_Textures[16], uv);
		case 17: return texture(u_Textures[17], uv);
		case 18: return texture(u_Textures[18], uv);
		case 19: return texture(u_Textures[19], uv);
		case 20: return texture(u_Textures[20], uv);
		case 21: return texture(u_Textures[21], uv);
		case 22: return texture(u_Textures[22], uv);
		case 23: return texture(u_Textures[23], uv);
		case 24: return texture(u_Textures[24], uv);
		case 25: return texture(u_Textures[25], uv);
		case 26: return texture(u_Textures[26], uv);
		case 27: return texture(u_Textures[27], uv);
		case 28: return texture(u_Textures[28], uv);
		case 29: return texture(u_Textures[29], uv);
		case 30: return texture(u_Textures[30], uv);
		default: return texture(u_Textures[31], uv);
	}
}

void main()
{
	color = sampleByIndex(int(v_TexIndex), v_TexCoord * v_TilingFactor) * v_Color;
}
