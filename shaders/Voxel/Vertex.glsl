in  vec2  _texCoord;
in  vec3  _vertex;

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
    vec2 v = applyModel(apply(origin, apply(relative, _vertex.xy)));
    vec4 vec = view * vec4(v.x, _vertex.z, v.y, 1.0);

    gl_Position = projection * vec;
    fogFactor   = getFogFactor(length(vec));
    texCoord    = _texCoord;
}
