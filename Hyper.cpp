#include <cmath>
#include <complex>
#include <iostream>
#include <functional>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Matrix.hpp"
#include "Gyrovector.hpp"

using namespace std::complex_literals;

enum class Direction {
    Forward, Staying, Backward
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

void drawDisk(size_t accuracy) {
    auto step = τ / accuracy;

    glBegin(GL_POLYGON);
    for (size_t idx = 0; idx <= accuracy; idx++) {
        auto φ = float(idx) * step;
        glVertex3d(cos(φ), 0, sin(φ));
    }
    glEnd();
}

void drawCurve(std::function<Gyrovector<double>(double)> g, size_t steps, double t1, double t2) {
    auto step = (t2 - t1) / steps;

    glBegin(GL_LINE_STRIP);
    for (size_t idx = 0; idx <= steps; idx++) {
        auto N = g(t1 + idx * step);
        glVertex3d(N.x(), 0, N.y());
    }
    glEnd();
}

void drawRectangle(Gyrovector<double> i, Gyrovector<double> j, Möbius<double> origin) {
    drawCurve(compose(Transform(origin), Line(+i, +j)), 64, 0, 1);
    drawCurve(compose(Transform(origin), Line(-i, +j)), 64, 0, 1);
    drawCurve(compose(Transform(origin), Line(+i, -j)), 64, 0, 1);
    drawCurve(compose(Transform(origin), Line(-i, -j)), 64, 0, 1);
}

double squareHalfDiag(double θ) {
    return sqrt(1 / tan(θ / 2 + τ / 8));
}

double squareHalfMiddleLine(double θ) {
    auto A = cos(θ / 2); constexpr auto B = 1.0 / sqrt(2);
    return sqrt((A - B) / (A + B));
}

constexpr auto wsize = 900;

constexpr auto fov  = 80;
constexpr auto near = 0.01;
constexpr auto far  = 50.0;

constexpr double ground = 0.3;

const auto k = τ / 5;

const auto L = squareHalfMiddleLine(k);
const auto D = squareHalfDiag(k) / sqrt(2);

const auto i = 2.0 * Gyrovector<double>(L, 0);
const auto j = 2.0 * Gyrovector<double>(0, L);

const auto A = Gyrovector<double>(+D, +D);
const auto B = Gyrovector<double>(+D, -D);

const auto M1 = Möbius<double>::translate(i);
const auto M2 = Möbius<double>::translate(j);
const auto M3 = M1.inverse();
const auto M4 = M2.inverse();

const auto speed = 0.5;

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
    glfwSetCursorPos(window, wsize/2, wsize/2);

    horizontal += mouseSpeed * dt * (wsize/2 - xpos);
    vertical   += mouseSpeed * dt * (wsize/2 - ypos);

    horizontal = std::fmod(horizontal, τ);
    vertical = std::max(std::min(vertical, τ/4 - ε), -τ/4 + ε);

    auto dx = cos(vertical) * sin(horizontal);
    auto dy = sin(vertical);
    auto dz = cos(vertical) * cos(horizontal);

    auto direction = vector(dx, dy, dz);
    auto right     = vector(sin(horizontal - τ/4), 0.0, cos(horizontal - τ/4));
    auto up        = cross(right, direction);

    glPushMatrix();

    gluLookAt(0, 0, 0, dx, dy, dz, up[0][0], up[1][0], up[2][0]);
    glTranslatef(0, -ground, 0);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(1.0f, 1.0f, 1.0f); drawDisk(256);

    glColor3f(0.0f, 0.0f, 0.0f);

    drawRectangle(A, B, origin);
    drawRectangle(A, B, origin * M1);
    drawRectangle(A, B, origin * M1 * M1);
    drawRectangle(A, B, origin * M2);
    drawRectangle(A, B, origin * M2 * M2);

    drawRectangle(A, B, origin * M2 * M2 * M2);
    drawRectangle(A, B, origin * M2 * M2 * M2 * M2);
    drawRectangle(A, B, origin * M2 * M2 * M2 * M2 * M2);
    drawRectangle(A, B, origin * M2 * M2 * M2 * M2 * M2 * M2);
    drawRectangle(A, B, origin * M2 * M2 * M2 * M2 * M2 * M2 * M2);
    drawRectangle(A, B, origin * M2 * M2 * M2 * M2 * M2 * M2 * M2 * M2);
    drawRectangle(A, B, origin * M2 * M2 * M2 * M2 * M2 * M2 * M2 * M2 * M2);
    drawRectangle(A, B, origin * M2 * M2 * M2 * M2 * M2 * M2 * M2 * M2 * M2 * M2);

    drawRectangle(A, B, origin * M3);
    drawRectangle(A, B, origin * M4);

    drawRectangle(A, B, origin * M4 * M1);
    drawRectangle(A, B, origin * M4 * M3);
    drawRectangle(A, B, origin * M3 * M2);
    drawRectangle(A, B, origin * M3 * M4);

    drawRectangle(A, B, origin * M3 * M4 * M4);
    drawRectangle(A, B, origin * M3 * M4 * M3);
    drawRectangle(A, B, origin * M4 * M3 * M4);
    drawRectangle(A, B, origin * M4 * M3 * M3);

    drawRectangle(A, B, origin * M1 * M2);
    drawRectangle(A, B, origin * M1 * M4);
    drawRectangle(A, B, origin * M2 * M1);
    drawRectangle(A, B, origin * M2 * M3);

    drawRectangle(A, B, origin * M1 * M1 * M2);
    drawRectangle(A, B, origin * M1 * M2 * M1);
    drawRectangle(A, B, origin * M1 * M1 * M2 * M1);

    glPopMatrix();
}

void keyboardCallback(GLFWwindow * window, int key, int scancode, int action, int mods)
{
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

    auto window = glfwCreateWindow(wsize, wsize, "Hypertest", nullptr, nullptr);
    if (!window) return -1;

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    glfwSetKeyCallback(window, keyboardCallback);
    glfwSetCursorPos(window, wsize/2, wsize/2);

    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    glewInit();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(fov, 1, near, far);

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