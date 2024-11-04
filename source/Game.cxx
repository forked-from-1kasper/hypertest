#include <Hyper/Game.hxx>

namespace Game {

using namespace Fundamentals;

GLFWwindow * window;

namespace Registry {
    Sheet sheet(textureSize, sheetSize);
    NodeRegistry node;
}

Atlas atlas;
Entity player(&atlas);

NodeId hotbar[hotbarSize] = {0};
size_t activeSlot = 0;

namespace Render {
    Real       fov, near, far;
    Real       distance;
    Standard * standard;
}

namespace GUI {
    int aimSize;
}

namespace Keyboard {
    bool forward  = false;
    bool backward = false;
    bool left     = false;
    bool right    = false;
    bool space    = false;
    bool lshift   = false;
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