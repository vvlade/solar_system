#version 450 core

struct Material {
    sampler2D texture_diffuse1;
};

in vec3 TexCoords;

out vec4 FragColor;

uniform Material material;

void main() {
    FragColor = texture(material.texture_diffuse1, TexCoords);
}