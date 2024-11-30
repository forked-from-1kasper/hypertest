in  vec4  color;
in  float fogFactor;
out vec4  fragColor;

void main() {
    fragColor = mix(color, fog.color, fogFactor);
}
