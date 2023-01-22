#include <Hyper/Game.hpp>

namespace Game {

using namespace Fundamentals;

GLFWwindow * window;

namespace Registry {
    Sheet sheet(textureSize, sheetSize);
    NodeRegistry node;
}

Atlas atlas;
Entity player(&atlas);

namespace Render {
    Real fov, near, far;
    Real distance;
}

namespace GUI {
    GLfloat aimSize;
}

namespace Keyboard {
    bool forward  = false;
    bool backward = false;
    bool left     = false;
    bool right    = false;
}

namespace Mouse {
    bool grabbed = false;
    Real xpos, ypos, speed = 0.7;
}

namespace Window {
    bool hovered = true, focused = true;

    int width, height;
    Real aspect;
}

}