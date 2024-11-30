#version 330 core

const float pi = 3.1415926535897932384626433832795;
const float tau = pi * 2.0;

struct Moebius { vec2 a, b, c, d; };

float norm(vec2 z) { return dot(z, z); }

vec2 mul(vec2 a, vec2 b)
{ return vec2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x); }

vec2 div(vec2 a, vec2 b)
{ return vec2(a.x * b.x + a.y * b.y, a.y * b.x - a.x * b.y) / norm(b); }

vec2 apply(Moebius M, vec2 z)
{ return div(mul(M.a, z) + M.b, mul(M.c, z) + M.d); }

vec2 applyModel(vec2 z);

uniform mat4 projection;
uniform mat4 view;

uniform Moebius domain;
uniform Moebius origin;

vec3 model(vec3 v)
{ return vec3(applyModel(apply(origin, apply(domain, v.xy))), v.z); }

struct Fog { bool enabled; vec4 color; float near, far; };

uniform Fog fog;

float getFogFactor(float d) {
    if (fog.enabled)
        return clamp(1.0 - (fog.far - d) / (fog.far - fog.near), 0.0, 1.0);
    else
        return 0.0;
}
