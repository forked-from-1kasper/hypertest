in  vec4  _color;
in  vec3  _vertex;
out vec4  color;
out float fogFactor;

void main() {
    vec4 vertex = view * model(_vertex, gl_InstanceID);

    gl_Position = projection * vertex;
    fogFactor   = getFogFactor(length(vertex.xyz / vertex.w));
    color       = _color;
}
