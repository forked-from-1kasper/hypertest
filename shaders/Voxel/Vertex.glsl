in  vec2  _texCoord;
in  vec2  _gyrovector;
in  float _height;
out vec2  texCoord;
out float fogFactor;

uniform Moebius origin;
uniform Moebius relative;

uniform mat4 projection;
uniform mat4 view;

uniform Fog fog;

float getFogFactor(in float d) {
    if (fog.enabled)
        return clamp(1.0 - (fog.far - d) / (fog.far - fog.near), 0.0, 1.0);
    else
        return 0.0;
}

void main() {
    vec2 gyrovector;

    apply(relative, _gyrovector, gyrovector);
    apply(origin, gyrovector, gyrovector);

    applyModel(gyrovector);

    vec4 vertex = view * vec4(gyrovector.x, _height, gyrovector.y, 1.0);

    gl_Position = projection * vertex;
    fogFactor   = getFogFactor(length(vertex));
    texCoord    = _texCoord;
}
