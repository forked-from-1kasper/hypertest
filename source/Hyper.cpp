#include <iostream>
#include <cmath>
#include <map>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <PicoPNG.hpp>

#include <Hyper/Fundamentals.hpp>
#include <Hyper/Tesselation.hpp>
#include <Hyper/Gyrovector.hpp>
#include <Hyper/Fuchsian.hpp>
#include <Hyper/Geometry.hpp>
#include <Hyper/Shader.hpp>
#include <Hyper/Sheet.hpp>

using namespace std::complex_literals;

void errorCallback(int error, const char* description) {
    fprintf(stderr, "Error: %s\n", description);
}

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

Real fov = 80, near = 1e-3, far = 150.0;

Real speed = 2 * Fundamentals::meter;

Real jumpHeight = 1.25, freeFallAccel = 9.8, normalJumpSpeed = sqrt(2 * freeFallAccel * jumpHeight);

auto position = Möbius<Real>::identity();

bool isFlying = true;
Real playerHeight = 1.8, normalVelocity = 0, normalLevel = 10;

Real horizontal = 0, vertical = 0;

auto localIsometry = Tesselation::I;
auto chunkPos = localIsometry.origin();
Chunk * currentChunk;

NodeRegistry nodeRegistry;
Atlas localAtlas;

auto roundOff(const Möbius<Real> & M) {
    auto N = (currentChunk->isometry().inverse() * localIsometry).field<Real>() * M;
    return Chunk::cell(N.origin());
}

bool isFree(Chunk * C, Rank x, Real L, Rank z)
{ return !C || (C->walkable(x, std::floor(L), z) && C->walkable(x, std::floor(L + playerHeight), z)); }

void jump() { if (!isFlying) normalVelocity += normalJumpSpeed; }

void setBlock(Rank i, Real L, Rank k, NodeId id) {
    if (!currentChunk) return;

    if (Chunk::outside(normalLevel)) return;

    if (i >= Fundamentals::chunkSize
     || k >= Fundamentals::chunkSize)
        return;

    auto j = Level(std::floor(normalLevel));

    currentChunk->set(i, j, k, {id});
    currentChunk->refresh(nodeRegistry, localIsometry);
}

void placeBlockNextToPlayer() {
    auto [i, j] = roundOff(position);
    setBlock(i + 1, normalLevel, j, 1);
}

void deleteBlockNextToPlayer() {
    auto [i, j] = roundOff(position);
    setBlock(i + 1, normalLevel, j, 0);
}

void returnToSpawn() {
    localIsometry  = Tesselation::I;
    chunkPos       = localIsometry.origin();
    currentChunk   = localAtlas.lookup(chunkPos);
    position       = {1, 0, 0, 1};
    normalLevel    = 5;
    normalVelocity = 0;

    localAtlas.updateMatrix(localIsometry);
}

Chunk * buildFloor(Chunk * chunk) {
    using namespace Fundamentals;

    for (size_t i = 0; i < chunkSize; i++)
        for (size_t j = 0; j < chunkSize; j++)
            chunk->set(i, 0, j, {1});

    chunk->set(0, 1, 0, {2});

    chunk->refresh(nodeRegistry, localIsometry);
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

    chunk->refresh(nodeRegistry, localIsometry);
    return chunk;
}

Chunk * markChunk(Chunk * chunk) {
    for (size_t i = 1; i <= 15; i++)
        for (size_t j = 1; j <= i; j++)
            chunk->set(i, j, 9, {2});

    chunk->set(13, 16, 9, {0});
    chunk->set(14, 16, 9, {0});
    chunk->set(15, 16, 9, {0});

    chunk->refresh(nodeRegistry, localIsometry);
    return chunk;
}

void moveHorizontally(const Gyrovector<Real> & v, const Real dt) {
    bool chunkChanged = false; auto G(localIsometry); auto g(chunkPos); auto C(currentChunk);

    auto P₁ = position * Möbius<Real>::translate(v.scale(dt));
    P₁ = P₁.normalize(); auto [i, j] = roundOff(P₁);

    if (i == Fundamentals::exterior && j == Fundamentals::exterior)
    for (size_t k = 0; k < Tesselation::neighbours.size(); k++) {
        const auto Δ   = Tesselation::neighbours[k];
        const auto Δ⁻¹ = Tesselation::neighbours⁻¹[k];
        const auto P₂  = (Δ⁻¹ * P₁).normalize();

        std::tie(i, j) = roundOff(P₂);

        if (i != Fundamentals::exterior && j != Fundamentals::exterior)
        { G *= Δ; g = G.origin(); C = localAtlas.lookup(g); P₁ = P₂; chunkChanged = true;
          if (!C) C = buildFloor(localAtlas.poll(localIsometry, G)); break; }
    }

    if (isFree(C, i, normalLevel, j)) {
        localIsometry = G; chunkPos = g; position = P₁;
        if (chunkChanged) { currentChunk = C; localAtlas.updateMatrix(localIsometry); }
    }
}

void moveVertically(const Real dt) {
    auto [i, j] = roundOff(position);

    normalVelocity -= dt * freeFallAccel;

    auto L = normalLevel + dt * normalVelocity;

    if (isFree(currentChunk, i, L, j)) { normalLevel = L; isFlying = true; }
    else normalVelocity = 0;

    if (!currentChunk->walkable(i, std::floor(L), j)) isFlying = false;
}

void update(Gyrovector<Real> & v, Real dt) { moveVertically(dt); moveHorizontally(v, dt); }

glm::mat4 view, projection;
Shader * shader;

constexpr Real Δtₘₐₓ = 1.0/5.0;

double globaltime = 0;
void display(GLFWwindow * window) {
    constexpr auto ε = 1e-6;

    auto dt = glfwGetTime() - globaltime; globaltime += dt;

    auto dir = 0i;
    if (Keyboard::forward)  dir += +1i;
    if (Keyboard::backward) dir += -1i;
    if (Keyboard::left)     dir += +1;
    if (Keyboard::right)    dir += -1;

    if (dir != 0.0) dir /= std::abs(dir);

    auto n = std::polar(1.0, -horizontal); Gyrovector<Real> velocity(speed * dir * n);
    auto Δt(dt); while (Δt >= Δtₘₐₓ) { update(velocity, Δtₘₐₓ); Δt -= Δtₘₐₓ; } update(velocity, Δt);

    auto origin = position.inverse();

    if (Mouse::grabbed) {
        glfwGetCursorPos(window, &Mouse::xpos, &Mouse::ypos);
        glfwSetCursorPos(window, Window::width/2, Window::height/2);

        horizontal += Mouse::speed * dt * (Window::width/2 - Mouse::xpos);
        vertical   += Mouse::speed * dt * (Window::height/2 - Mouse::ypos);

        horizontal = std::fmod(horizontal, τ);
        vertical = std::clamp(vertical, -τ/4 + ε, τ/4 - ε);
    }

    auto dx = cos(vertical) * sin(horizontal);
    auto dy = sin(vertical);
    auto dz = cos(vertical) * cos(horizontal);

    glm::vec3 direction(dx, dy, dz), right(sin(horizontal - τ/4), 0.0, cos(horizontal - τ/4));
    auto up = -glm::cross(right, direction);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    view = glm::lookAt(glm::vec3(0.0f), direction, up);
    view = glm::scale(view, glm::vec3(1.0f, Fundamentals::meter, 1.0f));
    view = glm::translate(view, glm::vec3(0.0f, -normalLevel - playerHeight, 0.0f));

    shader->uniform("view",       view);
    shader->uniform("projection", projection);

    shader->uniform("origin.a", origin.a);
    shader->uniform("origin.b", origin.b);
    shader->uniform("origin.c", origin.c);
    shader->uniform("origin.d", origin.d);

    for (auto & chunk : localAtlas.get())
        chunk->render(shader);
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
{
    if (Window::hovered && Window::focused && !Mouse::grabbed) grabMouse(window);
}

void cursorEnterCallback(GLFWwindow * window, int entered) {
    Window::hovered = entered;
    if (!Window::hovered) freeMouse(window);
}

void windowFocusCallback(GLFWwindow * window, int focused) {
    Window::focused = focused;
    if (!Window::focused) freeMouse(window);
}

void keyboardCallback(GLFWwindow * window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_ESCAPE:     glfwSetWindowShouldClose(window, GL_TRUE); break;
            case GLFW_KEY_W:          Keyboard::forward  = true; break;
            case GLFW_KEY_S:          Keyboard::backward = true; break;
            case GLFW_KEY_A:          Keyboard::left     = true; break;
            case GLFW_KEY_D:          Keyboard::right    = true; break;
            case GLFW_KEY_Z:          placeBlockNextToPlayer(); break;
            case GLFW_KEY_X:          deleteBlockNextToPlayer(); break;
            case GLFW_KEY_O:          returnToSpawn(); break;
            case GLFW_KEY_SPACE:      jump(); break;
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
    projection = glm::perspective(Real(fov), Real(width) / Real(height), near, far);
}

int main() {
    glfwSetErrorCallback(errorCallback);

    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    auto window = glfwCreateWindow(Window::width, Window::height, "Hypertest", nullptr, nullptr);
    if (!window) return -1;

    grabMouse(window);

    glfwSetKeyCallback(window, keyboardCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetWindowFocusCallback(window, windowFocusCallback);
    glfwSetCursorEnterCallback(window, cursorEnterCallback);
    glfwSetWindowSizeCallback(window, setupWindowSize);

    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    glewInit();

    setupTexture();

    glEnable(GL_DEPTH_TEST);
    shader = new Shader("Hyper.vs", "Hyper.fs");

    shader->activate();
    shader->uniform("textureSheet", 0);

    setupWindowSize(window, Window::width, Window::height);

    nodeRegistry.attach(1UL, NodeDef("Stuff", texture1));
    nodeRegistry.attach(2UL, NodeDef("Weird Stuff", texture2));

    using namespace Tesselation;

    currentChunk = buildTestStructure(localAtlas.poll(localIsometry, I));

    for (std::size_t k = 0; k < Tesselation::neighbours.size(); k++) {
        auto C = buildTestStructure(localAtlas.poll(localIsometry, Tesselation::neighbours[k]));
        if (k == 0) markChunk(C);
    }

    while (!glfwWindowShouldClose(window)) {
        display(window);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    delete shader;

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
