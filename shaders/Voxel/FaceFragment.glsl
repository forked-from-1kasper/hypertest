in  vec2  texCoord;
in  float fogFactor;
out vec4  fragColor;

uniform sampler2D textureSheet;

void main() {
    fragColor = mix(texture(textureSheet, texCoord), fog.color, fogFactor);
}
