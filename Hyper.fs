#version 330 core
out vec4 fragColor;

in vec2 texCoord;

uniform sampler2D textureSheet;

void main()
{
    fragColor = texture(textureSheet, texCoord);
}