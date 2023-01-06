#pragma once

#include <Hyper/Fundamentals.hpp>
#include <glm/vec4.hpp>
#include <GL/glew.h>
#include <Lua.hpp>

struct Config {
    struct {
        bool enabled;
        GLfloat min, max;
        glm::vec4 color;
    } fog;

    Real fov, near, far;

    Config(Lua::VM *, const char *);
};