#include <iostream>
#include <cmath>
#include <map>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <PicoPNG.hpp>
#include <Lua.hpp>

#include <Hyper/Fundamentals.hpp>
#include <Hyper/Tesselation.hpp>
#include <Hyper/Gyrovector.hpp>
#include <Hyper/Fuchsian.hpp>
#include <Hyper/Geometry.hpp>
#include <Hyper/Physics.hpp>
#include <Hyper/Config.hpp>
#include <Hyper/Shader.hpp>
#include <Hyper/Sheet.hpp>

using namespace std::complex_literals;

void errorCallback(int error, const char * description) {
    fprintf(stderr, "Error: %s\n", description);
}

struct Game {
    GLFWwindow * window;
    NodeRegistry nodeRegistry;
    Atlas        atlas;
};

namespace Keyboard {
    bool forward  = false;
    bool backward = false;
    bool left     = false;
    bool right    = false;
};

namespace Window {
    bool hovered = true;
    bool focused = true;

    int width = 900, height = 900;
}

namespace Mouse {
    bool grabbed = false;
    Real xpos, ypos, speed = 0.7;
}

Real fov, near, far;
Real speed = 2 * Fundamentals::meter;

Game game; Entity player(&game.atlas);

glm::mat4 view, projection;
Shader * shader;

bool move(Entity & E, const Gyrovector<Real> & v, Real Δt) {
    constexpr Real Δtₘₐₓ = 1.0/5.0; bool P = false;

    while (Δt >= Δtₘₐₓ) { auto Q = E.move(v, Δtₘₐₓ); P = P || Q; Δt -= Δtₘₐₓ; }
    auto R = E.move(v, Δt); return P || R;
}

double globaltime = 0;
void display(GLFWwindow * window) {
    auto dt = glfwGetTime() - globaltime; globaltime += dt;

    auto dir = 0i;
    if (Keyboard::forward)  dir += +1i;
    if (Keyboard::backward) dir += -1i;
    if (Keyboard::left)     dir += +1;
    if (Keyboard::right)    dir += -1;

    if (dir != 0.0) dir /= std::abs(dir);

    auto n = std::polar(1.0, -player.camera().yaw);
    Gyrovector<Real> velocity(speed * dir * n);

    bool chunkChanged = move(player, velocity, dt);
    if (chunkChanged) game.atlas.updateMatrix(player.camera().position.action());

    auto origin = player.camera().position.domain().inverse();

    if (Mouse::grabbed) {
        glfwGetCursorPos(window, &Mouse::xpos, &Mouse::ypos);
        glfwSetCursorPos(window, Window::width/2, Window::height/2);

        player.rotate(
            Mouse::speed * dt * (Window::width/2 - Mouse::xpos),
            Mouse::speed * dt * (Window::height/2 - Mouse::ypos)
        );
    }

    auto direction = player.camera().direction(), right = player.camera().right(), up = glm::cross(right, direction);

    view = glm::lookAt(glm::vec3(0.0f), direction, up);
    view = glm::scale(view, glm::vec3(1.0f, Fundamentals::meter, 1.0f));
    view = glm::translate(view, glm::vec3(0.0f, -player.camera().climb - player.eye, 0.0f));

    shader->uniform("view", view);
    shader->uniform("projection", projection);

    shader->uniform("origin.a", origin.a);
    shader->uniform("origin.b", origin.b);
    shader->uniform("origin.c", origin.c);
    shader->uniform("origin.d", origin.d);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (auto chunk : game.atlas.get()) {
        if (chunk->needRefresh())
            chunk->refresh(game.nodeRegistry, player.camera().position.action());

        chunk->render(shader);
    }
}

Texture texture1, texture2;

void setupTexture() {
    glEnable(GL_TEXTURE_2D); glEnable(GL_CULL_FACE);

    auto sheet = Sheet(Fundamentals::textureSize, Fundamentals::sheetSize);

    texture1 = sheet.attach("texture1.png");
    texture2 = sheet.attach("texture2.png");

    sheet.pack();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sheet.texture());
}

void grabMouse(GLFWwindow * window) {
    glfwSetCursorPos(window, Window::width/2, Window::height/2);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    Mouse::grabbed = true;
}

void freeMouse(GLFWwindow * window) {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);

    Mouse::grabbed = false;
}

void mouseButtonCallback(GLFWwindow * window, int button, int action, int mods)
{ if (Window::hovered && Window::focused && !Mouse::grabbed) grabMouse(window); }

void cursorEnterCallback(GLFWwindow * window, int entered) {
    Window::hovered = entered;
    if (!Window::hovered) freeMouse(window);
}

void windowFocusCallback(GLFWwindow * window, int focused) {
    Window::focused = focused;
    if (!Window::focused) freeMouse(window);
}

void jump() { if (!player.camera().flying) player.jump(); }

void setBlock(Rank i, Real L, Rank k, NodeId id) {
    if (player.chunk() == nullptr) return;

    if (Chunk::outside(player.camera().climb)) return;

    if (i >= Fundamentals::chunkSize
     || k >= Fundamentals::chunkSize)
        return;

    auto j = Level(std::floor(player.camera().climb));

    player.chunk()->set(i, j, k, {id});
    player.chunk()->requestRefresh();
}

void placeBlockNextToPlayer()
{ setBlock(player.i() + 1, player.camera().climb, player.j(), 1); }

void deleteBlockNextToPlayer()
{ setBlock(player.i() + 1, player.camera().climb, player.j(), 0); }

void returnToSpawn() {
    player.teleport(Position(), 5); player.roc(0);
    game.atlas.updateMatrix(player.camera().position.action());
}

void keyboardCallback(GLFWwindow * window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_ESCAPE:     glfwSetWindowShouldClose(window, GL_TRUE); break;
            case GLFW_KEY_W:          Keyboard::forward  = true; break;
            case GLFW_KEY_S:          Keyboard::backward = true; break;
            case GLFW_KEY_A:          Keyboard::left     = true; break;
            case GLFW_KEY_D:          Keyboard::right    = true; break;
            case GLFW_KEY_Z:          placeBlockNextToPlayer();  break;
            case GLFW_KEY_X:          deleteBlockNextToPlayer(); break;
            case GLFW_KEY_O:          returnToSpawn();           break;
            case GLFW_KEY_SPACE:      jump();                    break;
        }
    }

    if (action == GLFW_RELEASE) {
        switch (key) {
            case GLFW_KEY_W:          Keyboard::forward  = false; break;
            case GLFW_KEY_S:          Keyboard::backward = false; break;
            case GLFW_KEY_A:          Keyboard::left     = false; break;
            case GLFW_KEY_D:          Keyboard::right    = false; break;
        }
    }
}

void setupWindowSize(GLFWwindow * window, int width, int height) {
    Window::width = width; Window::height = height; glViewport(0, 0, width, height);
    projection = glm::perspective(glm::radians(fov), Real(width) / Real(height), near, far);
}

constexpr auto title = "Hypertest";
GLFWwindow * setupWindow(Config & config) {
    glfwSetErrorCallback(errorCallback);
    if (!glfwInit()) throw std::runtime_error("glfwInit failure");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    Window::width  = config.window.width;
    Window::height = config.window.height;

    auto window = glfwCreateWindow(Window::width, Window::height, title, nullptr, nullptr);
    if (!window) throw std::runtime_error("glfwCreateWindow failure");

    grabMouse(window);

    glfwSetKeyCallback(window, keyboardCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetWindowFocusCallback(window, windowFocusCallback);
    glfwSetCursorEnterCallback(window, cursorEnterCallback);
    glfwSetWindowSizeCallback(window, setupWindowSize);

    return window;
}

void setupGL(GLFWwindow * window, Config & config) {
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    glewInit();

    glEnable(GL_DEPTH_TEST);

    shader = new Shader("Common.glsl", "Hyper.vs", "Hyper.fs");
    shader->activate();

    shader->uniform("fog.enabled", config.fog.enabled);
    shader->uniform("fog.min",     config.fog.min);
    shader->uniform("fog.max",     config.fog.max);
    shader->uniform("fog.color",   config.fog.color);

    fov  = config.camera.fov;
    near = config.camera.near;
    far  = config.camera.far;

    setupWindowSize(window, Window::width, Window::height);
    setupTexture(); shader->uniform("textureSheet", 0);
}

Chunk * buildFloor(Chunk * chunk) {
    using namespace Fundamentals;

    for (size_t i = 0; i < chunkSize; i++)
        for (size_t j = 0; j < chunkSize; j++)
            chunk->set(i, 0, j, {1});

    chunk->set(0, 1, 0, {2});

    chunk->requestRefresh();
    return chunk;
}

Chunk * buildTestStructure(Chunk * chunk) {
    using namespace Fundamentals;

    for (size_t i = 0; i < chunkSize; i++) {
        for (size_t j = 0; j < chunkSize; j++) {
            NodeId id = (i + j) % 2 == 0 ? 1 : 2;

            chunk->set(i, 0, j, {id});
            chunk->set(i, 16, j, {3 - id});
        }
    }

    for (size_t i = 0; i < chunkSize; i += chunkSize - 1)
        for (size_t k = 0; k < chunkSize; k += chunkSize - 1)
            for (size_t j = 1; j < 16; j++)
                chunk->set(i, j, k, {2});

    chunk->requestRefresh();
    return chunk;
}

Chunk * markChunk(Chunk * chunk) {
    for (size_t i = 1; i <= 15; i++)
        for (size_t j = 1; j <= i; j++)
            chunk->set(i, j, 9, {2});

    chunk->set(12, 16, 9, {0});
    chunk->set(13, 16, 9, {0});
    chunk->set(14, 16, 9, {0});
    chunk->set(15, 16, 9, {0});

    chunk->set(3, 1, 5, {2});
    chunk->set(4, 2, 5, {2});
    chunk->set(5, 1, 5, {2});

    chunk->requestRefresh();
    return chunk;
}

void setupGame(Config & config) {
    using namespace Tesselation;

    game.nodeRegistry.attach(1UL, NodeDef("Stuff", texture1));
    game.nodeRegistry.attach(2UL, NodeDef("Weird Stuff", texture2));

    buildTestStructure(game.atlas.poll(player.camera().position.action(), I));

    for (std::size_t k = 0; k < Tesselation::neighbours.size(); k++) {
        auto C = buildTestStructure(game.atlas.poll(player.camera().position.action(), Tesselation::neighbours[k]));
        if (k == 0) markChunk(C);
    }

    game.atlas.onLoad = &buildFloor;

    player.eye = 1.62;
    player.height = 1.8;
    player.jumpHeight(1.25);
    player.teleport(Position(), 10);
}

void cleanUp(GLFWwindow * window) {
    delete shader;

    glfwDestroyWindow(window);
    glfwTerminate();
}

int main() {
    Lua::VM vm;

    Config config(&vm, "config.lua");

    auto window = setupWindow(config);
    setupGL(window, config);
    setupGame(config);

    while (!glfwWindowShouldClose(window)) {
        display(window);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    cleanUp(window);

    return 0;
}
