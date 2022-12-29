#include <map>
#include <cmath>
#include <complex>
#include <iostream>
#include <functional>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "PicoPNG.hpp"

#include "Matrix.hpp"
#include "Gyrovector.hpp"
#include "Fuchsian.hpp"
#include "Fundamentals.hpp"
#include "Tesselation.hpp"

using namespace std::complex_literals;

enum class Side { Top, Down, Left, Right };

template<typename T> struct Parallelogram {
    Vector2<T> A, B, C, D;

    constexpr auto rev() const { return Parallelogram<T>(D, C, B, A); }
};

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

void drawParallelogram(const Parallelogram<Real> & P, const Vector3<Real> n, Real h) {
    glNormal3d(n.x, n.y, n.z);
    glTexCoord2d(0, 0); glVertex3d(P.A.x, h, P.A.y);
    glTexCoord2d(1, 0); glVertex3d(P.B.x, h, P.B.y);
    glTexCoord2d(1, 1); glVertex3d(P.C.x, h, P.C.y);
    glTexCoord2d(0, 1); glVertex3d(P.D.x, h, P.D.y);
}

void drawSide(const Vector2<Real> & A, const Vector2<Real> & B, Real h₁, Real h₂) {
    Vector3<Real> v₁(0.0, h₂ - h₁, 0.0), v₂(B.x - A.x, 0.0, B.y - A.y);

    auto n = cross(v₁, v₂);
    glNormal3d(n.x, n.y, n.z);

    glTexCoord2d(1, 1); glVertex3d(A.x, h₁, A.y);
    glTexCoord2d(1, 0); glVertex3d(A.x, h₂, A.y);
    glTexCoord2d(0, 0); glVertex3d(B.x, h₂, B.y);
    glTexCoord2d(0, 1); glVertex3d(B.x, h₁, B.y);
}

void drawRightParallelogrammicPrism(Real h, Real Δh, const Parallelogram<Real> & P) {
    const auto h₁ = h, h₂ = h + Δh;

    glBegin(GL_QUADS);

    drawParallelogram(P, Vector3<Real>(0, +1, 0), h₂); // Top
    drawParallelogram(P.rev(), Vector3<Real>(0, -1, 0), h₁); // Bottom

    drawSide(P.B, P.A, h₁, h₂);
    drawSide(P.C, P.B, h₁, h₂);
    drawSide(P.D, P.C, h₁, h₂);
    drawSide(P.A, P.D, h₁, h₂);

    glEnd();
}

void drawNode(Möbius<Real> M, Rank x, Level y, Rank z) {
    drawRightParallelogrammicPrism(Real(y), 1,
        { Projection::apply(M.apply(grid[x + 0][z + 0])),
          Projection::apply(M.apply(grid[x + 1][z + 0])),
          Projection::apply(M.apply(grid[x + 1][z + 1])),
          Projection::apply(M.apply(grid[x + 0][z + 1])) }
    );
}

struct NodeDef { std::string name; GLuint texture; };

struct Node { NodeId id; };

struct Chunk {
    Fuchsian<Integer> isometry; // used only for drawing
    Gaussian²<Integer> pos; // used for indexing, should be equal to `isometry.origin()`
    Node data[Fundamentals::chunkSize][Fundamentals::worldHeight][Fundamentals::chunkSize];
};

using NodeRegistry = std::map<NodeId, NodeDef>;
using Atlas = std::vector<Chunk*>;

Chunk * lookup(Atlas & atlas, Gaussian²<Integer> & key) {
    for (auto chunk : atlas)
        if (chunk->pos == key)
            return chunk;

    return nullptr;
}

void drawChunk(NodeRegistry & nodeRegistry, Möbius<Real> & M, const Fuchsian<Integer> & G, const Chunk * chunk) {
    using namespace Fundamentals;

    auto N = M * (G.inverse() * chunk->isometry).field<Real>();

    NodeDef nodeDef;
    for (Level j = 0; true; j++) {
        for (Rank k = 0; k < chunkSize; k++) {
            for (Rank i = 0; i < chunkSize; i++) {
                auto id = chunk->data[i][j][k].id;

                if (id == 0) continue; // don’t draw air

                nodeDef = nodeRegistry[id];

                glBindTexture(GL_TEXTURE_2D, nodeDef.texture);
                drawNode(N, i, j, k);
            }
        }

        if (j == worldTop) break;
    }
}

Chunk * pollChunk(Atlas & atlas, const Fuchsian<Integer> & isometry) {
    auto pos = isometry.origin();

    for (auto chunk : atlas)
        if (chunk->pos == pos)
            return chunk;

    auto chunk = new Chunk(isometry, pos, {});

    // there should be better way to do this
    auto P = isometry.field<Real>();
    auto ω = P.det() / (P.d * P.d);

    if (ω.real() > 0 && ω.imag() >= 0) {}
    else if (ω.real() <= 0 && ω.imag() > 0) { chunk->isometry.a.mulnegi(); chunk->isometry.c.mulnegi(); }
    else if (ω.real() < 0 && ω.imag() <= 0) { chunk->isometry.a.negate(); chunk->isometry.c.negate(); }
    else if (ω.real() >= 0 && ω.imag() < 0) { chunk->isometry.a.muli(); chunk->isometry.c.muli(); }

    atlas.push_back(chunk);
    return chunk;
}

void unloadChunk(Atlas & atlas, const Gaussian²<Integer> & pos) {
    for (auto it = atlas.begin(); it != atlas.end();)
        if ((*it)->pos == pos) { delete *it; it = atlas.erase(it); }
        else it++;
}

void freeAtlas(Atlas & atlas) {
    for (auto chunk : atlas)
        delete chunk;
}

inline void setNode(Chunk * chunk, size_t i, size_t j, size_t k, const Node & node)
{ chunk->data[i][j][k] = node; }

Fuchsian<Integer> localIsometry(Tesselation::I);
auto chunkPos = localIsometry.origin();
Chunk * currentChunk;

NodeRegistry nodeRegistry;
Atlas localAtlas;

bool inCell(const Gyrovector<Real> & w, Rank i, Rank j) {
    const auto A = grid[i + 0][j + 0];
    const auto B = grid[i + 1][j + 0];
    const auto C = grid[i + 1][j + 1];
    const auto D = grid[i + 0][j + 1];

    const auto α = (w.x() - A.x()) * (B.y() - A.y()) - (B.x() - A.x()) * (w.y() - A.y());
    const auto β = (w.x() - B.x()) * (C.y() - B.y()) - (C.x() - B.x()) * (w.y() - B.y());
    const auto γ = (w.x() - C.x()) * (D.y() - C.y()) - (D.x() - C.x()) * (w.y() - C.y());
    const auto δ = (w.x() - D.x()) * (A.y() - D.y()) - (A.x() - D.x()) * (w.y() - D.y());

    return (α < 0) == (β < 0) && (β < 0) == (γ < 0) && (γ < 0) == (δ < 0);
}

std::pair<Rank, Rank> roundOff(const Gyrovector<Real> & w) {
    using namespace Fundamentals;

    for (Rank i = 0; i < chunkSize; i++)
        for (Rank j = 0; j < chunkSize; j++)
            if (inCell(w, i, j)) return std::pair(i, j);

    return std::pair(exterior, exterior);
}

std::pair<Rank, Rank> roundOff(Möbius<Real> M) {
    auto N = (currentChunk->isometry.inverse() * localIsometry).field<Real>() * M;
    return roundOff(N.origin());
}

inline bool isOutside(Real L) { return L < 0 || L >= Fundamentals::worldHeight; }

bool isWalkable(Chunk * C, Rank x, Real L, Rank z) {
    if (!C || x >= Fundamentals::chunkSize || z >= Fundamentals::chunkSize) return true;
    return isOutside(L) || (C->data[x][Level(L)][z].id == 0);
}

bool isFree(Chunk * C, Rank x, Real L, Rank z)
{ return isWalkable(C, x, std::floor(L), z) && isWalkable(C, x, std::floor(L + playerHeight), z); }

void jump() { if (!isFlying) normalVelocity += normalJumpSpeed; }

void setBlock(Rank i, Real L, Rank k, NodeId id) {
    if (!currentChunk) return;

    if (isOutside(normalLevel)) return;

    if (i >= Fundamentals::chunkSize
     || k >= Fundamentals::chunkSize)
        return;

    auto j = Level(std::floor(normalLevel));
    currentChunk->data[i][j][k].id = id;
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
    currentChunk  = lookup(localAtlas, chunkPos);
    position      = {1, 0, 0, 1};
    normalLevel   = 5;
}


Chunk * buildFloor(Chunk * chunk) {
    using namespace Fundamentals;

    for (size_t i = 0; i < chunkSize; i++)
        for (size_t j = 0; j < chunkSize; j++)
            setNode(chunk, i, 0, j, {1});

    setNode(chunk, 0, 1, 0, {2});

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
        { G *= Δ; g = G.origin(); C = lookup(localAtlas, g); P₁ = P₂;
          if (!C) C = buildFloor(pollChunk(localAtlas, G)); break; }
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

    if (!isWalkable(currentChunk, i, std::floor(L), j)) isFlying = false;
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

    for (auto & chunk : localAtlas)
        drawChunk(nodeRegistry, origin, localIsometry, chunk);

    glPopMatrix();
}

GLuint texture1, texture2;

void setupLighting() {
    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);
    glEnable(GL_LIGHT0);

    glShadeModel(GL_SMOOTH);

    glEnable(GL_DEPTH_TEST);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
}

void loadTexture(GLuint & ptr, const char * filepath) {
    unsigned long texWidth, texHeight;

    glGenTextures(1, &ptr);
    glBindTexture(GL_TEXTURE_2D, ptr);

    auto image = PNG::load(filepath, texWidth, texHeight);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data());
    image.clear();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

void setupTexture() {
    loadTexture(texture1, "texture1.png");
    loadTexture(texture2, "texture2.png");

    glBindTexture(GL_TEXTURE_2D, 0);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_CULL_FACE);
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

            setNode(chunk, i, 0, j, {id});
            setNode(chunk, i, 16, j, {3 - id});
        }
    }

    for (size_t i = 0; i < chunkSize; i += chunkSize - 1)
        for (size_t k = 0; k < chunkSize; k += chunkSize - 1)
            for (size_t j = 1; j < 16; j++)
                setNode(chunk, i, j, k, {2});

    return chunk;
}

Chunk * markChunk(Chunk * chunk) {
    setNode(chunk, 1, 1, 9, {2});

    setNode(chunk, 2, 1, 9, {2});
    setNode(chunk, 2, 2, 9, {2});

    setNode(chunk, 3, 1, 9, {2});
    setNode(chunk, 3, 2, 9, {2});
    setNode(chunk, 3, 3, 9, {2});
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

    nodeRegistry.insert({0UL, NodeDef("Air", 0)});
    nodeRegistry.insert({1UL, NodeDef("Stuff", texture1)});
    nodeRegistry.insert({2UL, NodeDef("Weird Stuff", texture2)});

    using namespace Tesselation;

    currentChunk = buildTestStructure(pollChunk(localAtlas, I));

    for (std::size_t k = 0; k < Tesselation::neighbours.size(); k++) {
        auto C = buildTestStructure(pollChunk(localAtlas, Tesselation::neighbours[k]));
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

    freeAtlas(localAtlas);

    return 0;
}
