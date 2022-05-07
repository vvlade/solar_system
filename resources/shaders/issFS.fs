#version 450 core
in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;


struct Material {
    sampler2D texture_diffuse;
    sampler2D texture_specular;

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

vec4 CalculatePointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec4 CalculateSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);


void main() {
    vec3 normal = normalize(Normal);
    vec3 ViewDir = normalize(ViewPos - FragPos);
    float a;
    vec4 result = CalculatePointLight(light, normal, FragPos, ViewDir);
    result += CalculateSpotLight(spotLight, normal, FragPos, ViewDir);
    if(result.a < 0.5) {
        discard;
    }

    FragColor = result;
}

vec4 CalculatePointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    vec4 color = texture(material.texture_diffuse, TexCoords);
    float a = color.a;
    vec3 color_diff = vec3(color);

    vec3 color_spec = vec3(texture(material.texture_specular, TexCoords));

    // ambient
    vec3 Ambient = light.ambient * color_diff * material.ambient;
    // diffuse
    vec3 lDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lDir), 0.0f);
    vec3 Diffuse = light.diffuse * diff * color_diff * material.diffuse;

    // specular
    vec3 halfwayDir = normalize(lDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0f), material.shininess);
    vec3 Specular = light.specular * spec * color_spec * material.specular;

    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear*distance + light.quadratic*(distance*distance));

    return vec4((Ambient + Diffuse + Specular) * attenuation, a);
}


vec4 CalculateSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    vec4 color = texture(material.texture_diffuse, TexCoords);
    float a = color.a;
    vec3 color_diff = vec3(color);

    vec3 color_spec = vec3(texture(material.texture_specular, TexCoords));

    // ambient
    vec3 Ambient = light.ambient * color_diff * material.ambient;

    // diffuse
    vec3 lDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lDir), 0.0f);
    vec3 Diffuse = light.diffuse * diff * color_diff * material.diffuse;

    // specular
    vec3 halfwayDir = normalize(lDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0f), material.shininess);
    vec3 Specular = light.specular * spec * color_spec * material.specular;


    // spotlight strength
    float theta = dot(lDir, normalize(-light.direction));
    float epsilon = (light.cutOff - light.outerCutOff);
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    Diffuse  *= intensity;
    Specular *= intensity;

    // attenuation
    float distance = length(light.position - FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    Ambient *= attenuation;
    Diffuse *= attenuation;
    Specular *= attenuation;

    return vec4((Ambient + Diffuse + Specular), a);
}