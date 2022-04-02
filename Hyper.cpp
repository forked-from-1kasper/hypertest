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
        glVertex3d(cos(φ), sin(φ), 0);
    }
    glEnd();
}

void drawCurve(std::function<Gyrovector<double>(double)> g, size_t steps, double t1, double t2) {
    auto step = (t2 - t1) / steps;

    glBegin(GL_LINE_STRIP);
    for (size_t idx = 0; idx <= steps; idx++) {
        auto N = g(t1 + idx * step);
        glVertex3d(N.x(), N.y(), 0);
    }
    glEnd();
}

void drawRectangle(Gyrovector<double> i, Gyrovector<double> j, Möbius<double> origin) {
    drawCurve(compose(Transform(origin), Line(+i, +j)), 64, 0, 1);
    drawCurve(compose(Transform(origin), Line(-i, +j)), 64, 0, 1);
    drawCurve(compose(Transform(origin), Line(+i, -j)), 64, 0, 1);
    drawCurve(compose(Transform(origin), Line(-i, -j)), 64, 0, 1);
}

double squareHalfDiag(double α) {
    auto A = cos(α); return sqrt(2 * A) / (sqrt(1 + A) + sqrt(1 - A));
}

const auto k = τ / 5;
const auto L = squareHalfDiag(k);
const auto A = Gyrovector<double>(L, 0);
const auto B = Gyrovector<double>(0, L);

const auto i = 2.0 * midpoint(+A, +B);
const auto j = 2.0 * midpoint(-A, +B);

const auto M1 = Möbius<double>::translate(i);
const auto M2 = Möbius<double>::translate(j);
const auto M3 = M1.inverse();
const auto M4 = M2.inverse();

const auto speed = 0.5;

constexpr auto mouseSpeed = 0.7;
auto velocity = Gyrovector<double>(0, 0);
auto position = Gyrovector<double>(0, 0);

double horizontal = 0, vertical = 0.0;
double xpos, ypos;

double globaltime = 0;
void display(GLFWwindow* window) {
    auto dt = glfwGetTime() - globaltime; globaltime += dt;
    glfwGetCursorPos(window, &xpos, &ypos);
    glfwSetCursorPos(window, 900/2, 900/2);

    horizontal += mouseSpeed * dt * (900/2 - xpos);
    vertical   += mouseSpeed * dt * (900/2 - ypos);

    /*
    auto dx = cos(vertical) * sin(horizontal);
    auto dy = sin(vertical);
    auto dz = cos(vertical) * cos(horizontal);
    */

    auto v = Gyrovector<double>(velocity.val * std::exp(-horizontal * 1i));

    auto n₁ = Gyrovector<double>(0.0, 1.0);
    auto P₁ = position;
    auto P₂ = P₁ + dt * v;
    auto n₂ = gyr(P₂, -P₁, n₁);
    auto Δφ = std::arg(n₁.val / n₂.val);

    position = P₂;
    horizontal += Δφ;

    auto origin = Möbius<double>::translate(position);

    glPushMatrix(); glRotatef((360 / τ) * horizontal, 0.0f, 0.0f, 1.0f);

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
    drawRectangle(A, B, origin * M3 * M4);
    drawRectangle(A, B, origin * M4 * M3);
    drawRectangle(A, B, origin * M3 * M4 * M4);
    drawRectangle(A, B, origin * M3 * M4 * M3);
    drawRectangle(A, B, origin * M4 * M3 * M4);
    drawRectangle(A, B, origin * M4 * M3 * M3);

    drawRectangle(A, B, origin * M1 * M2);
    drawRectangle(A, B, origin * M2 * M1);

    drawRectangle(A, B, origin * M1 * M1 * M2);
    drawRectangle(A, B, origin * M1 * M2 * M1);
    drawRectangle(A, B, origin * M1 * M1 * M2 * M1);

    glPopMatrix();
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, GL_TRUE); break;
            case GLFW_KEY_W:      velocity.val = -speed * 1i; break;
            case GLFW_KEY_S:      velocity.val = +speed * 1i; break;
            case GLFW_KEY_D:      velocity.val = -speed; break;
            case GLFW_KEY_A:      velocity.val = +speed; break;
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

    auto window = glfwCreateWindow(900, 900, "Hypertest", nullptr, nullptr);
    if (!window) return -1;

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    glfwSetKeyCallback(window, keyboardCallback);
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    glewInit();

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