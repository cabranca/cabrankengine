#type vertex
#version 300 es
precision highp float;

struct DirLight {
    highp vec3 direction;
    highp vec3 radiance;
};

layout(std140) uniform SceneData {
    highp mat4 u_ViewProjection;
    DirLight u_DirLight;
    highp vec3 u_CameraPosition;
};

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoords;

uniform mat4 u_Model;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

void main()
{
    FragPos = vec3(u_Model * vec4(pos, 1.0));

    Normal = mat3(transpose(inverse(u_Model))) * normal;

    TexCoords = texCoords;

    gl_Position = u_ViewProjection * vec4(FragPos, 1.0);
}

#type fragment
#version 300 es
precision mediump float;

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float     shininess;
};

struct DirLight {
    highp vec3 direction;
    highp vec3 radiance;
};

layout(std140) uniform SceneData {
    highp mat4 u_ViewProjection;
    DirLight u_DirLight;
    highp vec3 u_CameraPosition;
};

uniform Material material;

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);

void main()
{
    vec3 norm    = normalize(Normal);
    vec3 viewDir = normalize(u_CameraPosition - FragPos);

    vec3 ambient = 0.05 * vec3(texture(material.diffuse, TexCoords));

    // Point lights are not supported on WebGL 2 (SSBOs unavailable). Directional only.
    vec3 result = ambient + CalcDirLight(u_DirLight, norm, viewDir);

    result = result / (result + vec3(1.0));
    result = pow(result, vec3(1.0 / 2.2));

    FragColor = vec4(result, 1.0);
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir    = normalize(-light.direction);
    float diff       = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir  = reflect(-lightDir, normal);
    float spec       = pow(max(dot(viewDir, reflectDir), 0.0), max(material.shininess, 1.0));

    vec3 diffuse  = light.radiance * diff * vec3(texture(material.diffuse,  TexCoords));
    vec3 specular = light.radiance * spec * vec3(texture(material.specular, TexCoords));
    return diffuse + specular;
}
