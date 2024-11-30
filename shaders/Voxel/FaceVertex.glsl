in  vec4  _color;
in  vec3  _vertex;
out vec4  color;
out float fogFactor;

void main() {
    vec4 vertex = view * vec4(model(_vertex).xzy, 1.0);

    gl_Position = projection * vertex;
    fogFactor   = getFogFactor(length(vertex));
    color       = _color;
}
