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

    struct {
        int width, height;
    } window;

    struct {
        Real fov, near, far;
    } camera;

    struct {
        GLfloat aimSize;
    } gui;

    Config(Lua::VM *, const char *);
};