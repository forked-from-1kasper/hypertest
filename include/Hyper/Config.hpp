#pragma once

#include <glm/vec4.hpp>
#include <GL/glew.h>

#include <Hyper/Fundamentals.hpp>
#include <Lua.hpp>

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
    } window;

    struct {
        Real chunkRenderDistance = 10.0;
        Real fov = 80.0, near = 1e-3, far = 150.0;
        Model model = Model::Gans;
    } camera;

    struct {
        GLfloat aimSize = 15.0;
    } gui;

    Config(LuaJIT *, const char *);
};