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

enum class Direction {
    Forward, Staying, Backward
};

enum class Side {
    Top, Down, Left, Right
};

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

auto project(double y₁, double y₂) {
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

void glVertexH(double y₁, double y₂, double t) {
    auto v = project(y₁, y₂);
    glVertex3d(v.x, t, v.y);
}

inline void glVertexGyro(const Gyrovector<double> & vect, double height) {
    glVertexH(vect.x(), vect.y(), height);
}

void drawVertical(std::function<Gyrovector<double>(double)> g, size_t steps, double t1, double t2, double h₁, double h₂) {
    auto step = (t2 - t1) / steps;
    auto P₁ = g(t1);

    glBegin(GL_QUADS);

    for (size_t idx = 1; idx <= steps; idx++) {
        auto q₁ = (idx - 1) / double(steps), q₂ = idx / double(steps);
        auto P₂ = g(t1 + idx * step);

        auto v₁ = Vector3<double>(0.0, h₁ - h₂, 0.0);
        auto v₂ = Vector3<double>(P₂.x() - P₁.x(), 0.0, P₂.y() - P₁.y());
        auto n  = cross(v₁, v₂);

        glNormal3d(n.x, n.y, n.z);

        glTexCoord2d(q₁, 0); glVertexGyro(P₁, h₁);
        glTexCoord2d(q₁, 1); glVertexGyro(P₁, h₂);
        glTexCoord2d(q₂, 1); glVertexGyro(P₂, h₂);
        glTexCoord2d(q₂, 0); glVertexGyro(P₂, h₁);

        P₁ = P₂;
    }

    glEnd();
}

void texCoordSide(Side side, double t) {
    switch (side) {
        case Side::Top:   glTexCoord2d(t, 1);     break;
        case Side::Down:  glTexCoord2d(1 - t, 0); break;
        case Side::Left:  glTexCoord2d(0, t);     break;
        case Side::Right: glTexCoord2d(1, 1 - t); break;
    }
}

void drawHorizontal(std::function<Gyrovector<double>(double)> g, size_t steps, double t1, double t2, Gyrovector<double> origin, double height, Side side, double dir) {
    auto step = (t2 - t1) / steps;
    auto P₁ = g(t1);

    glBegin(GL_TRIANGLES);
    glNormal3d(0, dir, 0);

    for (size_t idx = 1; idx <= steps; idx++) {
        auto q₁ = (idx - 1) / double(steps), q₂ = idx / double(steps);
        auto P₂ = g(t1 + idx * step);

        texCoordSide(side, dir > 0 ? q₁ : q₂);
        glVertexGyro(dir > 0 ? P₁ : P₂, height);

        glTexCoord2d(0.5, 0.5);
        glVertexGyro(origin, height);

        texCoordSide(side, dir > 0 ? q₂ : q₁);
        glVertexGyro(dir > 0 ? P₂ : P₁, height);

        P₁ = P₂;
    }

    glEnd();

}

constexpr auto accuracy = 16;

void drawSide(Gyrovector<double> i, Gyrovector<double> j, Möbius<double> M, double h₁, double h₂) {
    drawVertical(compose(Transform(M), Line(+j, +i)), accuracy, 0, 1, h₁, h₂);
    drawVertical(compose(Transform(M), Line(+i, -j)), accuracy, 0, 1, h₁, h₂);
    drawVertical(compose(Transform(M), Line(-j, -i)), accuracy, 0, 1, h₁, h₂);
    drawVertical(compose(Transform(M), Line(-i, +j)), accuracy, 0, 1, h₁, h₂);
}

void drawCap(Gyrovector<double> i, Gyrovector<double> j, Möbius<double> M, double height, double dir) {
    drawHorizontal(compose(Transform(M), Line(+j, +i)), accuracy, 0, 1, M.origin(), height, Side::Top, dir);
    drawHorizontal(compose(Transform(M), Line(+i, -j)), accuracy, 0, 1, M.origin(), height, Side::Right, dir);
    drawHorizontal(compose(Transform(M), Line(-j, -i)), accuracy, 0, 1, M.origin(), height, Side::Down, dir);
    drawHorizontal(compose(Transform(M), Line(-i, +j)), accuracy, 0, 1, M.origin(), height, Side::Left, dir);
}

void drawCube(Gyrovector<double> i, Gyrovector<double> j, Möbius<double> M, double h₁, double h₂) {
    drawSide(i, j, M, h₁, h₂);
    drawCap(i, j, M, h₁, -1);
    drawCap(i, j, M, h₂, 1);
}

void drawGrid(Gyrovector<double> A, Gyrovector<double> B, Möbius<double> M, size_t chunkSize) {
    {
        auto Δ₁ = Line(+B, +A);
        auto Δ₂ = Line(-A, -B);

        for (size_t idx = 0; idx <= chunkSize; idx++) {
            auto t₁ = idx / double(chunkSize);

            auto u = Δ₁(t₁); auto v = Δ₂(t₁); auto n = -u + v;
            auto Γ = [u, n](double t) { return u + n.scale(t); };

            glBegin(GL_LINE_STRIP);
            for (size_t idy = 0; idy <= chunkSize; idy++) {
                auto t₂ = idy / double(chunkSize);
                glVertexGyro(M.apply(Γ(t₂)), 0);
            }
            glEnd();
        }
    }

    {
        auto Δ₁ = Line(+B, -A);
        auto Δ₂ = Line(+A, -B);

        for (size_t idx = 0; idx <= chunkSize; idx++) {
            auto t₁ = idx / double(chunkSize);

            auto u = Δ₁(t₁); auto v = Δ₂(t₁); auto n = -u + v;
            auto Γ = [u, n](double t) { return u + n.scale(t); };

            glBegin(GL_LINE_STRIP);
            for (size_t idy = 0; idy <= chunkSize; idy++) {
                auto t₂ = idy / double(chunkSize);
                glVertexGyro(M.apply(Γ(t₂)), 0);
            }
            glEnd();
        }
    }
}

constexpr auto width  = 1920;
constexpr auto height = 900;

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

constexpr size_t chunkSize = 16;

constexpr auto meter = 1.0/double(chunkSize);

const double ground = 1.8 / 16.0;
const auto speed = 10 * meter;

constexpr auto mouseSpeed = 0.7;
auto velocity = Gyrovector<double>(0, 0);
auto position = Gyrovector<double>(0, 0);

double horizontal = 0.0, vertical = 0.0;
double xpos, ypos;

double globaltime = 0;
void display(GLFWwindow * window) {
    constexpr auto ε = 0.001;

    auto dt = glfwGetTime() - globaltime; globaltime += dt;

    auto v = Gyrovector<double>(velocity.val * std::exp(-horizontal * 1i));

    auto P₁ = position;
    auto P₂ = P₁ + dt * v;
    auto Δφ = holonomy(P₁, P₂);

    position = P₂; horizontal += Δφ;

    auto origin = Möbius<double>::translate(position);

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

    glTranslatef(0, -ground, 0);

    glColor3f(0.0f, 0.0f, 0.0f);

    drawGrid(A, B, origin, chunkSize);
    drawGrid(A, B, origin * Δ1.field<double>(), chunkSize);
    drawGrid(A, B, origin * Δ2.field<double>(), chunkSize);
    drawGrid(A, B, origin * Δ3.field<double>(), chunkSize);
    drawGrid(A, B, origin * Δ4.field<double>(), chunkSize);

    auto M = Δ1 * Δ2;

    drawGrid(A, B, origin * M.field<double>(), chunkSize);
    drawGrid(A, B, origin * (Δ1 * Δ2 * Δ3).field<double>(), chunkSize);
    drawGrid(A, B, origin * (Δ1 * Δ2 * Δ3 * Δ4).field<double>(), chunkSize);

    glPopMatrix();
}

int texWidth, texHeight;
GLuint texture;

void setupLighting() {
    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);
    glEnable(GL_LIGHT0);

    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);

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
            case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, GL_TRUE); break;
            case GLFW_KEY_W:      velocity.val = -speed * 1i; break;
            case GLFW_KEY_S:      velocity.val = +speed * 1i; break;
            case GLFW_KEY_A:      velocity.val = -speed; break;
            case GLFW_KEY_D:      velocity.val = +speed; break;
        }
    }

    if (action == GLFW_RELEASE) velocity.val = 0;
}

int main() {
    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_SAMPLES, 16);

    auto window = glfwCreateWindow(width, height, "Hypertest", nullptr, nullptr);
    if (!window) return -1;

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    glfwSetKeyCallback(window, keyboardCallback);
    glfwSetCursorPos(window, width/2, height/2);

    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    glewInit();

    setupTexture();
    setupLighting();
    setupMaterial();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(fov, double(width) / double(height), near, far);

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
