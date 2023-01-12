in  vec2 _vertex;
in  vec4 _color;
out vec4 color;

void main() {
    gl_Position = vec4(_vertex, 0.0, 1.0);
    color = _color;
}