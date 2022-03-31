#include <cmath>
#include <complex>
#include <iostream>
#include <functional>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Matrix.hpp"
#include "Gyrovector.hpp"

using namespace std::complex_literals;

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

auto velocity = Gyrovector<double>(0, 0);
auto position = Gyrovector<double>(0, 0);

double globaltime = 0;
void display() {
    auto dt = glfwGetTime() - globaltime; globaltime += dt;

    glPushMatrix();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(1.0f, 1.0f, 1.0f); drawDisk(256);

    glColor3f(0.0f, 0.0f, 0.0f);

    position = dt * velocity + position;
    auto origin = Möbius<double>::translate(position);

    drawRectangle(A, B, origin);
    drawRectangle(A, B, origin * M1);
    drawRectangle(A, B, origin * M1 * M1);
    drawRectangle(A, B, origin * M2);
    drawRectangle(A, B, origin * M2 * M2);
    drawRectangle(A, B, origin * M2 * M2 * M2);
    drawRectangle(A, B, origin * M2 * M2 * M2 * M2);
    drawRectangle(A, B, origin * M2 * M2 * M2 * M2 * M2);
    drawRectangle(A, B, origin * M2 * M2 * M2 * M2 * M2 * M2);

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

    glfwSetKeyCallback(window, keyboardCallback);
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    glewInit();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    while (!glfwWindowShouldClose(window))
    {
        display();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}