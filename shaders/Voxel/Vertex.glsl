in  vec2  _texCoord;
in  vec3  _vertex;
out vec2  texCoord;
out float fogFactor;

uniform Moebius origin;
uniform Moebius relative;

uniform mat4 projection;
uniform mat4 view;

uniform Fog fog;

float getFogFactor(float d) {
    if (fog.enabled)
        return clamp(1.0 - (fog.far - d) / (fog.far - fog.near), 0.0, 1.0);
    else
        return 0.0;
}

vec3 model(vec3 v) { return vec3(applyModel(apply(origin, apply(relative, v.xy))), v.z); }

void main() {
    vec4 vertex = view * vec4(model(_vertex).xzy, 1.0);

    gl_Position = projection * vertex;
    fogFactor   = getFogFactor(length(vertex));
    texCoord    = _texCoord;
}
