#type vertex
#version 460 core

struct DirLight {
    vec3 direction;
    vec3 radiance;
};

layout(std140, binding = 0) uniform SceneData {
    mat4 u_ViewProjection;
    DirLight u_DirLight;
    vec3 u_CameraPosition; // Agregamos la cámara acá
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

    // This should be done in CPU as the inverse is expensive
    Normal = mat3(transpose(inverse(u_Model))) * normal;

    TexCoords = texCoords;

    gl_Position = u_ViewProjection * vec4(FragPos, 1.0);
}

#type fragment
#version 460 core
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
    vec3 direction;
    vec3 radiance;
};

struct PointLight {
    vec3 position;
    vec3 radiance;

    float constant;
    float linear;
    float quadratic;
};

layout(std140, binding = 0) uniform SceneData {
    mat4 u_ViewProjection;
    DirLight u_DirLight;
    vec3 u_CameraPosition;
};

uniform Material material;

layout(std430, binding = 0) readonly buffer LightBuffer {
    int u_PointLightCount;
    int _pad[3];
    PointLight pointLights[];
} u_Lights;


vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

void main()
{
    vec3 norm    = normalize(Normal);
    vec3 viewDir = normalize(u_CameraPosition - FragPos);

    // Ambient applied once — small constant so shading gradients are visible
    vec3 ambient = 0.05 * vec3(texture(material.diffuse, TexCoords));

    vec3 result = ambient;
    result += CalcDirLight(u_DirLight, norm, viewDir);
    for (int i = 0; i < u_Lights.u_PointLightCount; i++)
        result += CalcPointLight(u_Lights.pointLights[i], norm, FragPos, viewDir);

    // Tone mapping + gamma correction (matches PBR output)
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

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir    = normalize(light.position.xyz - fragPos);
    float diff       = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir  = reflect(-lightDir, normal);
    float spec       = pow(max(dot(viewDir, reflectDir), 0.0), max(material.shininess, 1.0));

    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance +
                                light.quadratic * (distance * distance));

    vec3 diffuse  = light.radiance * diff * vec3(texture(material.diffuse,  TexCoords));
    vec3 specular = light.radiance * spec * vec3(texture(material.specular, TexCoords));
    diffuse  *= attenuation;
    specular *= attenuation;
    return diffuse + specular;
}
