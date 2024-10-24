#pragma once

#include <glm/vec4.hpp>
#include <GL/glew.h>

#include <Hyper/Fundamentals.hpp>
#include <Lua.hpp>

struct Config {
    std::string world;

    struct {
        bool enabled;
        GLfloat near, far;
        glm::vec4 color;
    } fog;

    struct {
        int width, height;
    } window;

    struct {
        Real chunkRenderDistance, fov, near, far;
        Model model;
    } camera;

    struct {
        GLfloat aimSize;
    } gui;

    Config(Lua::VM *, const char *);
};