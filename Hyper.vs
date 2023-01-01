#version 330 core
layout (location = 0) in vec2  _texCoord;
layout (location = 1) in vec2  _gyrovector;
layout (location = 2) in float _height;

out vec2 texCoord;

struct Moebius {
    vec2 a;
    vec2 b;
    vec2 c;
    vec2 d;
};

uniform Moebius origin;
uniform Moebius relative;

uniform mat4 projection;
uniform mat4 view;

#define norm(z) (z.x * z.x + z.y * z.y)

#define mul(a, b) vec2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x)
#define div(a, b) vec2((a.x * b.x + a.y * b.y) / norm(b), (a.y * b.x - a.x * b.y) / norm(b))
#define apply(M, z) div((mul(M.a, z) + M.b), (mul(M.c, z) + M.d))

#define Gans(z) vec2(2.0f * z.x / (1.0f - norm(z)), 2.0f * z.y / (1.0f - norm(z)))
#define Klein(z) vec2(2.0f * z.x / (1.0f + norm(z)), 2.0f * z.y / (1.0f + norm(z)))
#define Poincare(z) (z)

void main()
{
    vec2 gyrovector = Gans(apply(origin, apply(relative, _gyrovector)));
    gl_Position = projection * view * vec4(gyrovector.x, _height, gyrovector.y, 1.0);
    texCoord = _texCoord;
}