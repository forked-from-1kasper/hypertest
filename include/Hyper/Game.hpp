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

    constexpr size_t hotbarSize = 9;
    extern NodeId hotbar[hotbarSize];
    extern size_t activeSlot;

    namespace Render {
        struct Standard {
            const Real meter;
            const Model model;

            Standard(const Model m) :
                meter(Projection::length(m, Tesselation::meter)),
                model(m)
            {}
        };

        extern Real       fov, near, far;
        extern Real       distance;
        extern Standard * standard;
    }

    namespace GUI {
        extern int aimSize;
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