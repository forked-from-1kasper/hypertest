#include <map>
#include <cmath>
#include <complex>
#include <iostream>
#include <functional>

#include <GL/glew.h>
#include <SOIL/SOIL.h>
#include <GLFW/glfw3.h>

#include "Matrix.hpp"
#include "Gyrovector.hpp"
#include "Fuchsian.hpp"

using namespace std::complex_literals;

enum class Side { Top, Down, Left, Right };

template<class F, class G> auto compose(F f, G g) {
    return [f, g](auto &&... args) {
        return f(g(std::forward<decltype(args)>(args)...));
    };
}

constexpr double τ = 2 * 3.141592653589793238462643383279502884197169399375105820974944;

void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

enum class Model { Poincaré, Klein, Gans };
constexpr auto model = Model::Gans;

constexpr auto projection(double y₁, double y₂) {
    double x₁, x₂;

    switch (model) {
        case Model::Poincaré: x₁ = y₁; x₂ = y₂; break;

        case Model::Klein: {
            auto σ = 1 + y₁ * y₁ + y₂ * y₂;
            x₁ = 2 * y₁ / σ; x₂ = 2 * y₂ / σ; break;
        };

        case Model::Gans: {
            auto σ = 1 - y₁ * y₁ - y₂ * y₂;
            x₁ = 2 * y₁ / σ; x₂ = 2 * y₂ / σ; break;
        }
    }

    return Vector2<double>(x₁, x₂);
}

constexpr auto projectGyro(const Gyrovector<double> & v) { return projection(v.x(), v.y()); }

int width = 900, height = 900;

constexpr auto fov  = 80;
constexpr auto near = 0.01;
constexpr auto far  = 150.0;

const GLfloat lightDiffuse[] = {1.0, 1.0, 1.0, 1.0};
const GLfloat matDiffuse[] = {1.0, 1.0, 1.0, 1.0};

const GLfloat lightPosition[] = {0.0f, 32.0f, 0.0f, 0.0f};

constexpr auto k = τ / 6;                      // π/3
constexpr auto D = sqrt(2/(tan(k/2) + 1) - 1); // s√(2 − √3)
constexpr auto L = sqrt(cos(k));               // s/√2

constexpr auto A = Gyrovector<double>(+D, +0);
constexpr auto B = Gyrovector<double>(+0, +D);

constexpr auto i = Coadd(A,  B); // s(1 + i)/√6
constexpr auto j = Coadd(A, -B); // s(1 − i)/√6

constexpr auto M1 = Möbius<double>::translate(i);

constexpr Fuchsian<int64_t> Δ1 {{+6, +0}, {+6, +6}, {+1, -1}, {+6, +0}};
constexpr Fuchsian<int64_t> Δ2 {{+6, +0}, {+6, -6}, {+1, +1}, {+6, +0}};
constexpr Fuchsian<int64_t> Δ3 {{+6, +0}, {-6, -6}, {-1, +1}, {+6, +0}};
constexpr Fuchsian<int64_t> Δ4 {{+6, +0}, {-6, +6}, {-1, -1}, {+6, +0}};

constexpr int chunkSize   = 16;
constexpr int worldHeight = 256;

constexpr auto meter = projectGyro(A).abs() / double(chunkSize);

const double speed       = 4.317 * meter;
const double normalSpeed = 1.0;

constexpr auto mouseSpeed = 0.7;

double level = 2.8 * meter;

namespace Keyboard {
    bool forward  = false;
    bool backward = false;
    bool left     = false;
    bool right    = false;
    bool up       = false;
    bool down     = false;
};

auto position = Gyrovector<double>(0, 0);

double horizontal = 0.0, vertical = 0.0;
double xpos, ypos;

template<typename T> constexpr T sign(T x) { return (x > 0) - (x < 0); }

constexpr auto Φ(double x, double y) {
    if (x == 0 && y == 0) return Gyrovector<double>(0, 0);

    auto L = fabs(x) + fabs(y);

    Gyrovector<double> u(sign(x) * L, 0), v(0, sign(y) * L);
    return u + fabs(y / L) * (-u + v);
}

constexpr auto Ψ(double x, double y) {
    auto u = (x + y) / 2;
    auto v = (x - y) / 2;

    return Φ(u * D, v * D);
}

constexpr auto yieldGrid(int i, int j) {
    auto x = double(2 * i - chunkSize) / double(chunkSize);
    auto y = double(2 * j - chunkSize) / double(chunkSize);
    return Ψ(x, y);
}

template<typename T, int N> using Array² = std::array<std::array<T, N>, N>;

constexpr auto initGrid() {
    Array²<Gyrovector<double>, chunkSize + 1> retval;

    for (int i = 0; i <= chunkSize; i++)
        for (int j = 0; j <= chunkSize; j++)
            retval[i][j] = yieldGrid(i, j);

    return retval;
}

constexpr auto grid = initGrid();

template<typename T> struct Parallelogram {
    Vector2<T> A, B, C, D;

    constexpr auto rev() const { return Parallelogram<T>(D, C, B, A); }
};

void drawParallelogram(const Parallelogram<double> & P, const Vector3<double> n, double h) {
    glNormal3d(n.x, n.y, n.z);
    glTexCoord2d(0, 0); glVertex3d(P.A.x, h, P.A.y);
    glTexCoord2d(1, 0); glVertex3d(P.B.x, h, P.B.y);
    glTexCoord2d(1, 1); glVertex3d(P.C.x, h, P.C.y);
    glTexCoord2d(0, 1); glVertex3d(P.D.x, h, P.D.y);
}

void drawSide(const Vector2<double> & A, const Vector2<double> & B, double h₁, double h₂) {
    auto v₁ = Vector3<double>(0.0, h₂ - h₁, 0.0);
    auto v₂ = Vector3<double>(B.x - A.x, 0.0, B.y - A.y);
    auto n  = cross(v₁, v₂);

    glNormal3d(n.x, n.y, n.z);
    glTexCoord2d(1, 1); glVertex3d(A.x, h₁, A.y);
    glTexCoord2d(1, 0); glVertex3d(A.x, h₂, A.y);
    glTexCoord2d(0, 0); glVertex3d(B.x, h₂, B.y);
    glTexCoord2d(0, 1); glVertex3d(B.x, h₁, B.y);
}

void drawRightParallelogrammicPrism(double h₁, double h₂, const Parallelogram<double> & P) {
    glBegin(GL_QUADS);

    drawParallelogram(P, Vector3<double>(0, +1, 0), h₂); // Top
    drawParallelogram(P.rev(), Vector3<double>(0, -1, 0), h₁); // Bottom

    drawSide(P.B, P.A, h₁, h₂);
    drawSide(P.C, P.B, h₁, h₂);
    drawSide(P.D, P.C, h₁, h₂);
    drawSide(P.A, P.D, h₁, h₂);

    glEnd();
}

void drawNode(Möbius<double> M, uint16_t x, uint16_t y, uint16_t z) {
    drawRightParallelogrammicPrism(double(y) * meter, double(y + 1) * meter,
        { projectGyro(M.apply(grid[x + 0][z + 0])),
          projectGyro(M.apply(grid[x + 1][z + 0])),
          projectGyro(M.apply(grid[x + 1][z + 1])),
          projectGyro(M.apply(grid[x + 0][z + 1])) }
    );
}

void drawGrid(Möbius<double> M) {
    for (uint16_t i = 0; i < chunkSize; i++)
        for (uint16_t j = 0; j < chunkSize; j++)
            drawNode(M, i, 0, j);
}

using NodeId = uint64_t;

struct NodeDef { std::string name; };
struct Node    { NodeId id; };
struct Chunk   { Fuchsian<int64_t> pos; Node data[chunkSize][worldHeight][chunkSize]; };

using NodeRegistry = std::map<NodeId, NodeDef>;
using Atlas = std::vector<Chunk>;

Chunk * lookup(Atlas atlas, Fuchsian<int64_t> key) {
    for (auto & chunk : atlas)
        if (chunk.pos == key)
            return &chunk;

    return nullptr;
}

NodeRegistry nodeRegistry;
Atlas localAtlas;

bool inCell(const Gyrovector<double> & w, uint16_t i, int16_t j) {
    auto A = grid[i + 0][j + 0];
    auto B = grid[i + 1][j + 0];
    auto C = grid[i + 1][j + 1];
    auto D = grid[i + 0][j + 1];

    auto α = (w.x() - A.x()) * (B.y() - A.y()) - (B.x() - A.x()) * (w.y() - A.y());
    auto β = (w.x() - B.x()) * (C.y() - B.y()) - (C.x() - B.x()) * (w.y() - B.y());
    auto γ = (w.x() - C.x()) * (D.y() - C.y()) - (D.x() - C.x()) * (w.y() - C.y());
    auto δ = (w.x() - D.x()) * (A.y() - D.y()) - (A.x() - D.x()) * (w.y() - D.y());

    return (α < 0) == (β < 0) && (β < 0) == (γ < 0) && (γ < 0) == (δ < 0);
}

std::pair<uint16_t, uint16_t> roundOff(const Gyrovector<double> & w) {
    for (uint16_t i = 0; i < chunkSize; i++)
        for (uint16_t j = 0; j < chunkSize; j++)
            if (inCell(w, i, j)) return std::pair(i, j);

    return std::pair(255, 255);
}

double globaltime = 0;
void display(GLFWwindow * window) {
    constexpr auto ε = 0.001;

    auto dt = glfwGetTime() - globaltime; globaltime += dt;

    auto dir = 0i;
    if (Keyboard::forward)  dir += +1i;
    if (Keyboard::backward) dir += -1i;
    if (Keyboard::left)     dir += +1;
    if (Keyboard::right)    dir += -1;

    if (dir != 0.0) dir /= std::abs(dir);

    auto normalDir = 0;
    if (Keyboard::up)   normalDir += 1;
    if (Keyboard::down) normalDir -= 1;

    auto velocity = Gyrovector<double>(speed * dir * std::polar(1.0, -horizontal));

    auto P₁ = position;
    auto P₂ = P₁ + dt * velocity;
    auto Δφ = holonomy(P₁, P₂);

    position = P₂; horizontal += Δφ;
    level += dt * normalDir * normalSpeed;

    auto [X, Y] = roundOff(position);
    std::cout << X << " " << Y << std::endl;

    auto origin = Möbius<double>::translate(-position);

    glfwGetCursorPos(window, &xpos, &ypos);
    glfwSetCursorPos(window, width/2, height/2);

    horizontal += mouseSpeed * dt * (width/2 - xpos);
    vertical   += mouseSpeed * dt * (height/2 - ypos);

    horizontal = std::fmod(horizontal, τ);
    vertical = std::max(std::min(vertical, τ/4 - ε), -τ/4 + ε);

    auto dx = cos(vertical) * sin(horizontal);
    auto dy = sin(vertical);
    auto dz = cos(vertical) * cos(horizontal);

    auto direction = Vector3<double>(dx, dy, dz);
    auto right     = Vector3<double>(sin(horizontal - τ/4), 0.0, cos(horizontal - τ/4));
    auto up        = cross(right, direction);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix();

    gluLookAt(0, 0, 0, dx, dy, dz, up.x, up.y, up.z);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

    glTranslatef(0, -level, 0);

    glColor3f(0.0f, 0.0f, 0.0f);

    drawNode(origin, 9, 1, 9);
    drawNode(origin, 9, 3, 9);

    drawGrid(origin);
    drawGrid(origin * Δ1.field<double>());
    drawGrid(origin * Δ2.field<double>());
    drawGrid(origin * Δ3.field<double>());
    drawGrid(origin * Δ4.field<double>());

    auto M = Δ1 * Δ2;

    drawGrid(origin * M.field<double>());
    drawGrid(origin * (Δ1 * Δ2 * Δ3).field<double>());
    drawGrid(origin * (Δ1 * Δ2 * Δ3 * Δ4).field<double>());

    glPopMatrix();
}

int texWidth, texHeight;
GLuint texture;

void setupLighting() {
    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);
    glEnable(GL_LIGHT0);

    glShadeModel(GL_SMOOTH);

    glEnable(GL_DEPTH_TEST);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
}

void setupTexture() {
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    auto image = SOIL_load_image("texture.jpg", &texWidth, &texHeight, 0, SOIL_LOAD_RGB);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    SOIL_free_image_data(image);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_CULL_FACE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

void setupMaterial() {
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, matDiffuse);
}

void keyboardCallback(GLFWwindow * window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_ESCAPE:       glfwSetWindowShouldClose(window, GL_TRUE); break;
            case GLFW_KEY_W:            Keyboard::forward  = true; break;
            case GLFW_KEY_S:            Keyboard::backward = true; break;
            case GLFW_KEY_A:            Keyboard::left     = true; break;
            case GLFW_KEY_D:            Keyboard::right    = true; break;
            case GLFW_KEY_LEFT_SHIFT:   Keyboard::up       = true; break;
            case GLFW_KEY_LEFT_CONTROL: Keyboard::down     = true; break;
        }
    }

    if (action == GLFW_RELEASE) {
        switch (key) {
            case GLFW_KEY_W:            Keyboard::forward  = false; break;
            case GLFW_KEY_S:            Keyboard::backward = false; break;
            case GLFW_KEY_A:            Keyboard::left     = false; break;
            case GLFW_KEY_D:            Keyboard::right    = false; break;
            case GLFW_KEY_LEFT_SHIFT:   Keyboard::up       = false; break;
            case GLFW_KEY_LEFT_CONTROL: Keyboard::down     = false; break;
        }
    }
}

void setupWindowSize(GLFWwindow * window, int newWidth, int newHeight) {
    width = newWidth; height = newHeight;
    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(fov, double(width) / double(height), near, far);
}

int main() {
    nodeRegistry.insert({0UL, NodeDef("Air")});
    nodeRegistry.insert({1UL, NodeDef("Stuff")});

    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_SAMPLES, 16);

    auto window = glfwCreateWindow(width, height, "Hypertest", nullptr, nullptr);
    if (!window) return -1;

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    glfwSetKeyCallback(window, keyboardCallback);
    glfwSetWindowSizeCallback(window, setupWindowSize);

    glfwSetCursorPos(window, width/2, height/2);

    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    glewInit();

    setupTexture();
    setupLighting();
    setupMaterial();

    setupWindowSize(window, width, height);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

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
