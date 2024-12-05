#pragma once

#include <glm/vec4.hpp>
#include <GL/glew.h>

#include <Hyper/Fundamentals.hxx>
#include <Lua.hxx>

struct Config {
    std::string world = "world.sqlite3";

    struct {
        bool enabled = false;
        GLfloat near = 1.0, far = 5.0;

        glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
    } fog;

    struct {
        int width  = 800;
        int height = 600;
        int msaa   = 0;
    } window;

    struct {
        unsigned int verticalRenderDistance = 2;
        Real horizontalRenderDistance = 10.0;

        Real fov = 80.0, near = 1e-3, far = 150.0;
        Model model = {Gans};
    } camera;

    struct {
        GLfloat aimSize = 15.0;
    } gui;

    Config(LuaJIT *, const char *);
};