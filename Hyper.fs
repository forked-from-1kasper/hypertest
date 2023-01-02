#version 120

varying vec2 texCoord;

uniform sampler2D textureSheet;

void main()
{
    gl_FragColor = texture2D(textureSheet, texCoord);
}