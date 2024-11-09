#version 330 core

struct Moebius { vec2 a, b, c, d; };

#define norm(z) dot(z, z)
#define add(a, b) (a + b)
#define mul(a, b) vec2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x)
#define div(a, b) vec2(dot(a, b) / norm(b), (a.y * b.x - a.x * b.y) / norm(b))

void apply(in Moebius M, in vec2 z1, out vec2 z2)
{ z2 = div(add(mul(M.a, z1), M.b), add(mul(M.c, z1), M.d)); }

void applyModel(inout vec2 z);

struct Fog {
    bool  enabled;
    vec4  color;
    float near;
    float far;
};
