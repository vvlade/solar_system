#version 450 core

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
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
uniform samplerCube depthMap;

uniform float far_plane;

vec3 gridSamplingDisk[20] = vec3[] (
   vec3(1, 1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1, 1,  1),
   vec3(1, 1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
   vec3(1, 1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1, 1,  0),
   vec3(1, 0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1, 0, -1),
   vec3(0, 1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0, 1, -1)
);

float ShadowCalculation(vec3 fragPos) {
    vec3 fragToLight = fragPos - light.position;
    float currentDepth = length(fragToLight);

    float shadow = 0.0;
    float bias = 0.15;
    int samples = 20;
    float viewDistance = length(ViewPos - FragPos);
    float diskRadius = (1.0 + (viewDistance / far_plane)) / 25.0;
    for(int i = 0; i < samples; ++i) {
        float closestDepth = texture(depthMap, fragToLight + gridSamplingDisk[i] * diskRadius).r;
        closestDepth *= far_plane;
        if(currentDepth - bias > closestDepth) {
            shadow += 1.0;
        }
    }
    shadow /= float(samples);

    return shadow;
}


vec3 CalculatePointLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    // ambient
    vec3 Ambient = light.ambient * vec3(texture(material.texture_diffuse1, TexCoords));
    // diffuse
    vec3 lDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lDir), 0.0f);
    vec3 Diffuse = light.diffuse * diff * vec3(texture(material.texture_diffuse1, TexCoords));
    // specular
    vec3 halfwayDir = normalize(lDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0f), material.shininess);
    vec3 Specular = light.specular * spec * vec3(texture(material.texture_specular1, TexCoords));

    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear*distance + light.quadratic*(distance*distance));

    float shadow = ShadowCalculation(FragPos);

    return (Ambient + (1.0 - shadow) * (Diffuse + Specular));
}

void main() {
    vec3 normal = normalize(Normal);
    vec3 ViewDir = normalize(ViewPos - FragPos);
    vec3 result = CalculatePointLight(light, normal, FragPos, ViewDir);
    FragColor = vec4(result, 1.0f);
}