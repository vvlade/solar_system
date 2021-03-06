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

struct PointLight {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};


struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};


uniform Material material;
uniform PointLight light;
uniform SpotLight spotLight;
uniform vec3 ViewPos;

uniform samplerCube depthMap;

vec3 CalculatePointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalculateSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
float ShadowCalculation(vec3 fragPos);

void main() {
    vec3 normal = normalize(Normal);
    vec3 ViewDir = normalize(ViewPos - FragPos);
    vec3 result = CalculateSpotLight(spotLight, normal, FragPos, ViewDir);
    result += CalculatePointLight(light, normal, FragPos, ViewDir);

    FragColor = vec4(result, 1.0f);
}

vec3 CalculatePointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
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


//     float shadow = ShadowCalculation(FragPos);

    return (Ambient + Diffuse + Specular) * attenuation;

//     return (Ambient + (1.0 - shadow) * (Diffuse + Specular)) * attenuation;
}


vec3 CalculateSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    // ambient
    vec3 Ambient = light.ambient * vec3(texture(material.texture_diffuse1, TexCoords)) * material.ambient;

    // diffuse
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 Diffuse = light.diffuse * diff * vec3(texture(material.texture_diffuse1, TexCoords)) * material.diffuse;


    // specular
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
    vec3 Specular = light.specular * spec * vec3(texture(material.texture_specular1, TexCoords)) * material.specular;


    // attenuation
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = (light.cutOff - light.outerCutOff);
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    Diffuse *= intensity;
    Specular *= intensity;

    // attenuation
    float distance = length(light.position - FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    Ambient *= attenuation;
    Diffuse *= attenuation;
    Specular *= attenuation;

    return (Ambient + Diffuse + Specular);
}

float ShadowCalculation(vec3 fragPos) {
    // get vector between fragment position and light position
    vec3 fragToLight = FragPos - light.position;
    // ise the fragment to light vector to sample from the depth map
    float closestDepth = texture(depthMap, fragToLight).r;
    // it is currently in linear range between [0,1], let's re-transform it back to original depth value
    closestDepth *= 1000.0f;
    // now get current linear depth as the length between the fragment and light position
    float currentDepth = length(fragToLight);
    // test for shadows
    float bias = 0.05; // we use a much larger bias since depth is now in [near_plane, far_plane] range
    float shadow = currentDepth -  bias > closestDepth ? 1.0 : 0.0;

    return shadow;
}