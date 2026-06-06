#type vertex
#version 460 core

struct DirLight {
    vec3 direction;
    vec3 radiance;
};

layout(std140, binding = 0) uniform SceneData {
    mat4 u_ViewProjection;
    DirLight dirLight;
    vec3 u_CameraPos;
};

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 u_Model;

void main() {
    FragPos = vec3(u_Model * vec4(pos, 1.0));
    Normal = mat3(transpose(inverse(u_Model))) * normal; // This could be done in CPU as the inverse is expensive
    TexCoords = texCoords;

    gl_Position = u_ViewProjection * vec4(FragPos, 1.0);
}

#type fragment
#version 460 core

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
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
    DirLight dirLight;
    vec3 u_CameraPos;
};

layout(std430, binding = 0) readonly buffer LightBuffer {
    int u_PointLightCount;
    int _pad[3];
    PointLight pointLights[];
} u_Lights;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

layout(location = 0) out vec4 fragColor;

uniform Material material;

/*
    lightDir: vector from light source to fragment
    lightRadiance: light source color
    normal: normalized normal vector to the fragment's surface
    viewDir: normalized vector from camera to fragment
*/
vec3 calcLightIncidence(vec3 lightDir, vec3 lightRadiance, vec3 normal, vec3 viewDir);
vec3 calcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

void main() {
    // Common variables needed
    vec3 normal = normalize(Normal);
    vec3 viewDir = normalize(FragPos - u_CameraPos); // From camera to fragment

    // Ambient component
    float ambientFactor = 0.05;
    vec3 ambient = ambientFactor * vec3(texture(material.diffuse, TexCoords));
    vec3 result = ambient;
    
    result += calcLightIncidence(dirLight.direction, dirLight.radiance, normal, viewDir);
    for (int i = 0; i < u_Lights.u_PointLightCount; i++)
        result += calcPointLight(u_Lights.pointLights[i], normal, FragPos, viewDir);
    
    // Tone mapping + gamma correction
    result = result / (result + vec3(1.0));
    result = pow(result, vec3(1.0 / 2.2));
    fragColor = vec4(result, 1.0); // No alpha
}

vec3 calcLightIncidence(vec3 lightDir, vec3 lightRadiance, vec3 normal, vec3 viewDir) {
    // Diffuse component
    float diffuseFactor = max(dot(normal, -lightDir), 0.0);
    vec3 diffuse = lightRadiance * diffuseFactor * vec3(texture(material.diffuse, TexCoords));

    // Specular component
    vec3 reflectDir = reflect(lightDir, normal);
    float specularFactor = pow(max(dot(-viewDir, reflectDir), 0.0), max(material.shininess, 1.0));
    vec3 specular = lightRadiance * specularFactor * vec3(texture(material.specular, TexCoords));

    return diffuse + specular;
}

vec3 calcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    vec3 lightDir = normalize(fragPos - light.position);
    vec3 baseLightIncidence = calcLightIncidence(lightDir, light.radiance, normal, viewDir);

    float distance = length(fragPos - light.position);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

    return baseLightIncidence * attenuation;
}
