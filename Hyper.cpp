#include <map>
#include <cmath>
#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Sheet.hpp"
#include "Geometry.hpp"
#include "PicoPNG.hpp"

#include "Matrix.hpp"
#include "Fuchsian.hpp"
#include "Gyrovector.hpp"
#include "Tesselation.hpp"
#include "Fundamentals.hpp"

using namespace std::complex_literals;

enum class Side { Top, Down, Left, Right };

void errorCallback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

namespace Keyboard {
    bool forward  = false;
    bool backward = false;
    bool left     = false;
    bool right    = false;
};

bool windowHovered = true, windowFocused = true, mouseGrabbed = false;

constexpr auto fov  = 80;
constexpr auto near = 10e-4;
constexpr auto far  = 150.0;

const GLfloat lightDiffuse[] = {1.0, 1.0, 1.0, 1.0};
const GLfloat matDiffuse[] = {1.0, 1.0, 1.0, 1.0};

const GLfloat lightPosition[] = {0.0f, 32.0f, 0.0f, 0.0f};

int width = 900, height = 900;

Real speed = 2 * Fundamentals::meter, mouseSpeed = 0.7;

constexpr Real jumpHeight = 1.25;

Real freeFallAccel = 9.8;
Real normalJumpSpeed = sqrt(2 * freeFallAccel * jumpHeight);

Möbius<Real> position {1, 0, 0, 1};

bool isFlying = true;
Real playerHeight = 1.8, normalVelocity = 0, normalLevel = 10;

Real horizontal = 0, vertical = 0, xpos, ypos;

Fuchsian<Integer> localIsometry(Tesselation::I);
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
    localIsometry = Tesselation::I;
    chunkPos      = localIsometry.origin();
    currentChunk  = localAtlas.lookup(chunkPos);
    position      = {1, 0, 0, 1};
    normalLevel   = 5;
}

Chunk * buildFloor(Chunk * chunk) {
    using namespace Fundamentals;

    for (size_t i = 0; i < chunkSize; i++)
        for (size_t j = 0; j < chunkSize; j++)
            chunk->set(i, 0, j, {1});

    chunk->set(0, 1, 0, {2});

    return chunk;
}

void moveHorizontally(const Gyrovector<Real> & v, const Real dt) {
    auto G(localIsometry); auto g(chunkPos); auto C(currentChunk);

    auto P₁ = position * Möbius<Real>::translate(v.scale(dt));
    P₁ = P₁.normalize(); auto [i, j] = roundOff(P₁);

    if (i == Fundamentals::exterior && j == Fundamentals::exterior)
    for (std::size_t k = 0; k < Tesselation::neighbours.size(); k++) {
        const auto Δ   = Tesselation::neighbours[k];
        const auto Δ⁻¹ = Tesselation::neighbours⁻¹[k];
        const auto P₂  = (Δ⁻¹ * P₁).normalize();

        std::tie(i, j) = roundOff(P₂);

        if (i != Fundamentals::exterior && j != Fundamentals::exterior)
        { G *= Δ; g = G.origin(); C = localAtlas.lookup(g); P₁ = P₂;
          if (!C) C = buildFloor(localAtlas.poll(G)); break; }
    }

    if (isFree(C, i, normalLevel, j))
    { localIsometry = G; chunkPos = g; currentChunk = C; position = P₁; }
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

    if (mouseGrabbed) {
        glfwGetCursorPos(window, &xpos, &ypos);
        glfwSetCursorPos(window, width/2, height/2);

        horizontal += mouseSpeed * dt * (width/2 - xpos);
        vertical   += mouseSpeed * dt * (height/2 - ypos);

        horizontal = std::fmod(horizontal, τ);
        vertical = std::clamp(vertical, -τ/4 + ε, τ/4 - ε);
    }

    auto dx = cos(vertical) * sin(horizontal);
    auto dy = sin(vertical);
    auto dz = cos(vertical) * cos(horizontal);

    Vector3<Real> direction(dx, dy, dz), right(sin(horizontal - τ/4), 0.0, cos(horizontal - τ/4));
    auto up = cross(right, direction);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix();

    gluLookAt(0, 0, 0, dx, dy, dz, up.x, up.y, up.z);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

    glScalef(1.0f, Fundamentals::meter, 1.0f);
    glTranslatef(0, -normalLevel - playerHeight, 0);

    glColor3f(0.0f, 0.0f, 0.0f);

    for (auto & chunk : localAtlas.get())
        chunk->render(nodeRegistry, origin, localIsometry);

    glPopMatrix();
}

Texture texture1, texture2;

void setupLighting() {
    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);
    glEnable(GL_LIGHT0);

    glShadeModel(GL_SMOOTH);

    glEnable(GL_DEPTH_TEST);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
}

void setupTexture() {
    glEnable(GL_TEXTURE_RECTANGLE); glEnable(GL_CULL_FACE);

    auto sheet = Sheet(Fundamentals::textureSize, Fundamentals::sheetSize);

    texture1 = sheet.attach("texture1.png");
    texture2 = sheet.attach("texture2.png");

    sheet.pack(); glBindTexture(GL_TEXTURE_RECTANGLE, sheet.texture());
}

void setupMaterial() {
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, matDiffuse);
}

void grabMouse(GLFWwindow * window) {
    glfwSetCursorPos(window, width/2, height/2);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    mouseGrabbed = true;
}

void freeMouse(GLFWwindow * window) {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);

    mouseGrabbed = false;
}

void mouseButtonCallback(GLFWwindow * window, int button, int action, int mods)
{
    if (windowHovered && windowFocused && !mouseGrabbed) grabMouse(window);
}

void cursorEnterCallback(GLFWwindow * window, int entered) {
    windowHovered = entered;
    if (!windowHovered) freeMouse(window);
}

void windowFocusCallback(GLFWwindow * window, int focused) {
    windowFocused = focused;
    if (!windowFocused) freeMouse(window);
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

void setupWindowSize(GLFWwindow * window, int newWidth, int newHeight) {
    width = newWidth; height = newHeight;
    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(fov, Real(width) / Real(height), near, far);
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

    return chunk;
}

Chunk * markChunk(Chunk * chunk) {
    for (size_t i = 1; i <= 8; i++)
        for (size_t j = 1; j <= i; j++)
            chunk->set(i, j, 9, {2});

    return chunk;
}

int main() {
    glfwSetErrorCallback(errorCallback);

    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_SAMPLES, 16);

    auto window = glfwCreateWindow(width, height, "Hypertest", nullptr, nullptr);
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
    setupLighting();
    setupMaterial();

    setupWindowSize(window, width, height);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    nodeRegistry.attach(1UL, NodeDef("Stuff", texture1));
    nodeRegistry.attach(2UL, NodeDef("Weird Stuff", texture2));

    using namespace Tesselation;

    currentChunk = buildTestStructure(localAtlas.poll(I));

    for (std::size_t k = 0; k < Tesselation::neighbours.size(); k++) {
        auto C = buildTestStructure(localAtlas.poll(Tesselation::neighbours[k]));
        if (k == 0) markChunk(C);
    }

    while (!glfwWindowShouldClose(window))
    {
        display(window);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
