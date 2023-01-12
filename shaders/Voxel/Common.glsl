#version 330 core

struct Moebius {
    vec2 a;
    vec2 b;
    vec2 c;
    vec2 d;
};

struct Fog {
    bool  enabled;
    vec4  color;
    float near;
    float far;
};
