#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <Hyper/Sheet.hpp>
#include <Hyper/Physics.hpp>
#include <Hyper/Geometry.hpp>
#include <Hyper/Fundamentals.hpp>

#pragma once

enum class Action { Remove, Place };

namespace Game {
    extern GLFWwindow * window;

    namespace Registry {
        extern NodeRegistry node;
        extern Sheet sheet;
    }

    extern Atlas atlas;
    extern Entity player;

    namespace Render {
        extern Real fov, near, far;
        extern Real distance;
    }

    namespace GUI {
        extern GLfloat aimSize;
    }

    namespace Keyboard {
        extern bool forward;
        extern bool backward;
        extern bool left;
        extern bool right;
        extern bool space;
        extern bool lshift;
    }

    namespace Mouse {
        extern Real xpos, ypos, speed;
        extern bool grabbed;
    }

    namespace Window {
        extern bool hovered, focused;
        extern int width, height;
        extern Real aspect;
    }
}