in  vec4  color;
in  vec2  texCoord;
in  float mixFactor;
out vec4  fragColor;

uniform sampler2D textureSheet;

void main() {
    fragColor = mix(texture(textureSheet, texCoord), color, mixFactor);
}