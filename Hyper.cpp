#include <cmath>
#include <complex>
#include <iostream>
#include <functional>

#include <GL/glew.h>
#include <SOIL/SOIL.h>
#include <GLFW/glfw3.h>

#include "Matrix.hpp"
#include "Gyrovector.hpp"

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

void drawDisk(size_t accuracy) {
    auto step = τ / accuracy;

    glBegin(GL_POLYGON);
    for (size_t idx = 0; idx <= accuracy; idx++) {
        auto φ = float(idx) * step;
        glVertex3d(cos(φ), 0, sin(φ));
    }
    glEnd();
}

void drawVertical(std::function<Gyrovector<double>(double)> g, size_t steps, double t1, double t2, double height) {
    auto step = (t2 - t1) / steps;
    auto P₁ = g(t1);

    glBegin(GL_QUADS);

    for (size_t idx = 1; idx <= steps; idx++) {
        auto q₁ = (idx - 1) / double(steps), q₂ = idx / double(steps);
        auto P₂ = g(t1 + idx * step);

        auto v₁ = vector(0.0, -height, 0.0);
        auto v₂ = vector(P₂.x() - P₁.x(), 0.0, P₂.y() - P₁.y());
        auto n  = cross(v₁, v₂);

        glNormal3d(n[0][0], n[1][0], n[2][0]);

        glTexCoord2d(q₁, 0); glVertex3d(P₁.x(), 0, P₁.y());
        glTexCoord2d(q₁, 1); glVertex3d(P₁.x(), height, P₁.y());
        glTexCoord2d(q₂, 1); glVertex3d(P₂.x(), height, P₂.y());
        glTexCoord2d(q₂, 0); glVertex3d(P₂.x(), 0, P₂.y());

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

void glVertexGyro(const Gyrovector<double> & vect, double height) {
    glVertex3d(vect.x(), height, vect.y());
}

void drawHorizontal(std::function<Gyrovector<double>(double)> g, size_t steps, double t1, double t2, Vector<double> origin, Side side, double dir) {
    auto step = (t2 - t1) / steps;
    auto P₁ = g(t1);

    glBegin(GL_TRIANGLES);
    glNormal3d(0, dir, 0);

    for (size_t idx = 1; idx <= steps; idx++) {
        auto q₁ = (idx - 1) / double(steps), q₂ = idx / double(steps);
        auto P₂ = g(t1 + idx * step);

        texCoordSide(side, dir > 0 ? q₁ : q₂);
        glVertexGyro(dir > 0 ? P₁ : P₂, origin[1][0]);

        glTexCoord2d(0.5, 0.5);
        glVertex3d(origin[0][0], origin[1][0], origin[2][0]);

        texCoordSide(side, dir > 0 ? q₂ : q₁);
        glVertexGyro(dir > 0 ? P₂ : P₁, origin[1][0]);

        P₁ = P₂;
    }

    glEnd();

}

constexpr auto accuracy = 16;

void drawSide(Gyrovector<double> i, Gyrovector<double> j, Möbius<double> M, double height) {
    drawVertical(compose(Transform(M), Line(+j, +i)), accuracy, 0, 1, height);
    drawVertical(compose(Transform(M), Line(+i, -j)), accuracy, 0, 1, height);
    drawVertical(compose(Transform(M), Line(-j, -i)), accuracy, 0, 1, height);
    drawVertical(compose(Transform(M), Line(-i, +j)), accuracy, 0, 1, height);
}

void drawCap(Gyrovector<double> i, Gyrovector<double> j, Möbius<double> M, double height, double dir) {
    auto v = M.origin().elevate(height);

    drawHorizontal(compose(Transform(M), Line(+j, +i)), accuracy, 0, 1, v, Side::Top, dir);
    drawHorizontal(compose(Transform(M), Line(+i, -j)), accuracy, 0, 1, v, Side::Right, dir);
    drawHorizontal(compose(Transform(M), Line(-j, -i)), accuracy, 0, 1, v, Side::Down, dir);
    drawHorizontal(compose(Transform(M), Line(-i, +j)), accuracy, 0, 1, v, Side::Left, dir);
}

void drawCube(Gyrovector<double> i, Gyrovector<double> j, Möbius<double> M, double height) {
    drawSide(i, j, M, height);
    drawCap(i, j, M, height, 1);
    drawCap(i, j, M, 0, -1);
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

const GLfloat lightDiffuse[] = {1.0, 1.0, 1.0, 1.0};
const GLfloat matDiffuse[] = {1.0, 1.0, 1.0, 1.0};

const GLfloat lightPosition[] = {0.0f, 5.0f, 5.0f, 1.0f};

const auto k = τ / 5;

const auto L = squareHalfMiddleLine(k);
const auto D = squareHalfDiag(k) / sqrt(2);

const double ground = 3 * L;

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

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

    glPushMatrix();

    gluLookAt(0, 0, 0, dx, dy, dz, up[0][0], up[1][0], up[2][0]);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

    glTranslatef(0, -ground, 0);

    glColor3f(0.0f, 0.0f, 0.0f);

    drawCube(A, B, origin, 2 * L);

    drawCube(A, B, origin * M1, 2 * L);
    drawCube(A, B, origin * M1 * M1, 2 * L);
    drawCube(A, B, origin * M2, 2 * L);
    drawCube(A, B, origin * M2 * M2, 2 * L);

    drawCube(A, B, origin * M2 * M2 * M2, 2 * L);
    drawCube(A, B, origin * M2 * M2 * M2 * M2, 2 * L);
    drawCube(A, B, origin * M2 * M2 * M2 * M2 * M2, 2 * L);
    drawCube(A, B, origin * M2 * M2 * M2 * M2 * M2 * M2, 2 * L);
    drawCube(A, B, origin * M2 * M2 * M2 * M2 * M2 * M2 * M2, 2 * L);
    drawCube(A, B, origin * M2 * M2 * M2 * M2 * M2 * M2 * M2 * M2, 2 * L);
    drawCube(A, B, origin * M2 * M2 * M2 * M2 * M2 * M2 * M2 * M2 * M2, 2 * L);
    drawCube(A, B, origin * M2 * M2 * M2 * M2 * M2 * M2 * M2 * M2 * M2 * M2, 2 * L);

    drawCube(A, B, origin * M3, 2 * L);
    drawCube(A, B, origin * M4, 2 * L);

    drawCube(A, B, origin * M4 * M1, 2 * L);
    drawCube(A, B, origin * M4 * M3, 2 * L);
    drawCube(A, B, origin * M3 * M2, 2 * L);
    drawCube(A, B, origin * M3 * M4, 2 * L);

    drawCube(A, B, origin * M3 * M4 * M4, 2 * L);
    drawCube(A, B, origin * M3 * M4 * M3, 2 * L);
    drawCube(A, B, origin * M4 * M3 * M4, 2 * L);
    drawCube(A, B, origin * M4 * M3 * M3, 2 * L);

    drawCube(A, B, origin * M1 * M2, 2 * L);
    drawCube(A, B, origin * M1 * M4, 2 * L);
    drawCube(A, B, origin * M2 * M1, 2 * L);
    drawCube(A, B, origin * M2 * M3, 2 * L);

    drawCube(A, B, origin * M1 * M1 * M2, 2 * L);
    drawCube(A, B, origin * M1 * M2 * M1, 2 * L);
    drawCube(A, B, origin * M1 * M1 * M2 * M1, 2 * L);

    glTranslatef(0, 4 * L, 0);
    drawCube(A, B, origin, 2 * L);

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

    setupTexture();
    setupLighting();
    setupMaterial();

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