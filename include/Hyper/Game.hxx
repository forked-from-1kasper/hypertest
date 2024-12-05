#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <Hyper/Sheet.hxx>
#include <Hyper/Physics.hxx>
#include <Hyper/Geometry.hxx>
#include <Hyper/Fundamentals.hxx>

enum class Action { Remove, Place };

namespace Game {
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

            Standard(const Model m) : meter(m.length(Tesselation::meter)), model(m) {}
        };

        extern unsigned int vmax; extern Real hmax;

        extern Real fov, near, far;
        extern Standard * standard;

        extern vec4 background;
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