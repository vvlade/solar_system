#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

// TODO: make the iss have specular lightning with material and light properties


uniform sampler2D tex0;

void main()
{
    vec4 texColor = texture(tex0, TexCoords);
    if(texColor.a < 0.1) {
        discard;
    }
    FragColor = texColor;
}