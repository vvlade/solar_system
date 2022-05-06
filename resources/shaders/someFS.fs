#version 450 core

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float shininess;
};

struct Light {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

uniform Material material;
uniform Light light;
uniform vec3 ViewPos;

vec3 CalculatePointLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    // ambient
    vec3 Ambient = light.ambient * vec3(texture(material.texture_diffuse1, TexCoords)) * material.ambient;
    // diffuse
    vec3 lDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lDir), 0.0f);
    vec3 Diffuse = light.diffuse * diff * vec3(texture(material.texture_diffuse1, TexCoords)) * material.diffuse;
    // specular
    vec3 halfwayDir = normalize(lDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0f), material.shininess);
    vec3 Specular = light.specular * spec * vec3(texture(material.texture_specular1, TexCoords)) * material.specular;

    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear*distance + light.quadratic*(distance*distance));

    return (Ambient + Diffuse + Specular) * attenuation;
}

void main() {
    vec3 normal = normalize(Normal);
    vec3 ViewDir = normalize(ViewPos - FragPos);
    vec3 result = CalculatePointLight(light, normal, FragPos, ViewDir);
    FragColor = vec4(result, 1.0f);
}