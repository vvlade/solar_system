#version 450 core
in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;

// TODO: make the iss have specular lightning with material and light properties

struct Material {
    sampler2D texture_diffuse;
    sampler2D texture_specular;

    vec4 ambient;
    vec4 diffuse;
    vec4 specular;

    float shininess;
};

struct Light {
    vec3 position;

    vec4 ambient;
    vec4 diffuse;
    vec4 specular;

    float constant;
    float linear;
    float quadratic;
};

uniform Material material;
uniform Light light;
uniform vec3 ViewPos;

vec4 CalculatePointLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    // ambient
    vec4 Ambient = light.ambient * texture(material.texture_diffuse, TexCoords) * material.ambient;
    // diffuse
    vec3 lDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lDir), 0.0f);
    vec4 Diffuse = light.diffuse * diff * texture(material.texture_diffuse, TexCoords) * material.diffuse;

    // specular
    vec3 halfwayDir = normalize(lDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0f), material.shininess);
    vec4 Specular = 1.0f * spec * texture(material.texture_specular, TexCoords) * material.specular;


    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear*distance + light.quadratic*(distance*distance));

    return (Ambient + Diffuse + Specular) * attenuation;
}

void main() {
    vec3 normal = normalize(Normal);
    vec3 ViewDir = normalize(ViewPos - FragPos);
    vec4 result = CalculatePointLight(light, normal, FragPos, ViewDir);
    if(result.a < 0.5) {
        discard;
    }
    FragColor = result;
}