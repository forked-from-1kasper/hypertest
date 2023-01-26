in  vec2  _texCoord;
in  vec2  _gyrovector;
in  float _height;
out vec2  texCoord;
out float fogFactor;

uniform Model model;

uniform Moebius origin;
uniform Moebius relative;

uniform mat4 projection;
uniform mat4 view;

uniform Fog fog;

#define norm(z) dot(z, z)
#define add(a, b) (a + b)
#define mul(a, b) vec2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x)
#define div(a, b) vec2(dot(a, b) / norm(b), (a.y * b.x - a.x * b.y) / norm(b))

void apply(in Moebius M, in vec2 z1, out vec2 z2)
{ z2 = div(add(mul(M.a, z1), M.b), add(mul(M.c, z1), M.d)); }

void Gans(inout vec2 z)
{ z *= 2.0f / (1.0f - norm(z)); }

void Klein(inout vec2 z)
{ z *= 2.0f / (1.0f + norm(z)); }

float getFogFactor(in float d) {
    if (fog.enabled)
        return clamp(1.0 - (fog.far - d) / (fog.far - fog.near), 0.0, 1.0);
    else return 0.0;
}

void main() {
    vec2 gyrovector;

    apply(relative, _gyrovector, gyrovector);
    apply(origin, gyrovector, gyrovector);

    if (model == POINCARE) {}
    if (model == KLEIN) Klein(gyrovector);
    if (model == GANS) Gans(gyrovector);

    vec4 vertex = view * vec4(gyrovector.x, _height, gyrovector.y, 1.0);

    gl_Position = projection * vertex;
    fogFactor   = getFogFactor(length(vertex));
    texCoord    = _texCoord;
}
