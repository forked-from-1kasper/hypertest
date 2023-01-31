in  vec3  _vertex;
in  vec4  _color;
in  vec2  _texCoord;
in  float _mixFactor;

out vec4  color;
out vec2  texCoord;
out float mixFactor;

void main() {
    gl_Position = vec4(_vertex, 1.0);

    color        = _color;
    texCoord     = _texCoord;
    mixFactor    = _mixFactor;
}