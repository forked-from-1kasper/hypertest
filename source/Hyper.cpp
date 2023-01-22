#include <iostream>
#include <cmath>
#include <map>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

#include <PicoPNG.hpp>
#include <Lua.hpp>

#include <Hyper/Gyrovector.hpp>
#include <Hyper/Fuchsian.hpp>
#include <Hyper/Config.hpp>
#include <Hyper/Shader.hpp>
#include <Hyper/Game.hpp>

using namespace std::complex_literals;

void errorCallback(int error, const char * description) {
    fprintf(stderr, "Error: %s\n", description);
}

glm::mat4 view, projection;

Shader<DummyShader> * dummyShader;
Shader<VoxelShader> * voxelShader;

Shader<DummyShader>::VAO aimVao;

PBO<GLfloat, Action> pbo(GL_DEPTH_COMPONENT, 1, 1);

Real chunkDiameter(const Real n) {
    static const auto i = Gyrovector<Real>(Fundamentals::D½, 0);
    static const auto j = Gyrovector<Real>(0, Fundamentals::D½);
    static const auto k = Coadd(i, j);
    return (n * k).abs();
}

bool move(Entity & E, const Gyrovector<Real> & v, Real Δt) {
    constexpr Real Δtₘₐₓ = 1.0/5.0; bool P = false;

    while (Δt >= Δtₘₐₓ) { auto Q = E.move(v, Δtₘₐₓ); P = P || Q; Δt -= Δtₘₐₓ; }
    auto R = E.move(v, Δt); return P || R;
}

glm::vec3 unproject(const glm::mat4 & view, const glm::mat4 & projection, const GLfloat depth) {
    auto v = glm::inverse(view) * glm::inverse(projection) * glm::vec4(0.0f, 0.0f, depth, 1.0f);
    return glm::vec3(v.x / v.w, v.y / v.w, v.z / v.w);
}

glm::vec3 trace(const glm::mat4 & view, const glm::mat4 & projection, const GLfloat zbuffer, const GLfloat H, bool forward) {
    auto h = glm::vec3(0.0f, H, 0.0f);

    auto w₀ = Projection::unapply(unproject(view, projection, 2.0f * zbuffer - 1.0f));
    auto dist₀ = glm::length(w₀ - h);

    const GLfloat ε = Fundamentals::meter / 3.0f;
    GLfloat dist = dist₀ + (forward ? +ε : -ε);

    return (dist / dist₀) * (w₀ - h) + h;
}

std::optional<std::pair<Chunk *, Gyrovector<Real>>> getNeighbour(const Gyrovector<Real> & P) {
    if (Chunk::isInsideOfDomain(P)) {
        auto Q = Game::player.chunk()->relative().inverse().apply(P);
        return std::optional(std::pair(Game::player.chunk(), Q));
    }

    for (size_t k = 0; k < Tesselation::neighbours.size(); k++) {
        auto G = Game::player.chunk()->isometry() * Tesselation::neighbours[k];
        if (auto C = Game::atlas.lookup(G.origin())) {
            auto Q = C->relative().inverse().apply(P);

            if (Chunk::isInsideOfDomain(Q))
                return std::optional(std::pair(C, Q));
        }
    }

    return std::nullopt;
}

const Real elevationRate = 3.0;

void pressLShift() { if (Game::player.noclip) Game::player.roc(-elevationRate); }
void releaseLShift() { if (Game::player.noclip) Game::player.roc(0); }

void pressSpace() {
    if (Game::player.noclip) Game::player.roc(elevationRate);
    else if (!Game::player.camera().flying) Game::player.jump();
}
void releaseSpace() { if (Game::player.noclip) Game::player.roc(0); }

void setBlock(Chunk * C, Rank i, Real L, Rank k, NodeId id) {
    if (C == nullptr || Chunk::outside(L))
        return;

    if (i >= Fundamentals::chunkSize || k >= Fundamentals::chunkSize)
        return;

    auto j = Level(std::floor(L));

    if (id != 0 && C->get(i, j, k).id != 0)
        return;

    C->set(i, j, k, {id});

    if (Game::player.stuck())
        C->set(i, j, k, {0});

    C->requestRefresh();
}

void click(const Aut𝔻<Real> & origin, const GLfloat zbuffer, const Action action) {
    const auto maxₕ = 3.0 * Fundamentals::meter, maxᵥ = 4.0;
    const auto H = Game::player.camera().climb + Game::player.eye;

    auto v = trace(view, projection, zbuffer, H, action == Action::Remove);
    auto P = Gyrovector(v.x, v.z);

    if (P.abs() <= maxₕ && fabs(v.y - H) <= maxᵥ) {
        if (auto ret = getNeighbour(origin.inverse().apply(P))) {
            auto [C, Q] = *ret; auto [i, k] = Chunk::round(Q);

            if (action == Action::Place)  setBlock(C, i, v.y, k, 2);
            if (action == Action::Remove) setBlock(C, i, v.y, k, 0);
        }
    }
}

void returnToSpawn() {
    Game::player.teleport(Position(), 5); Game::player.roc(0);
    Game::atlas.updateMatrix(Game::player.camera().position.action());
}

double globaltime = 0;
void display(GLFWwindow * window) {
    using namespace Game;

    auto dt = glfwGetTime() - globaltime; globaltime += dt;

    auto dir = 0i;
    if (Keyboard::forward)  dir += +1i;
    if (Keyboard::backward) dir += -1i;
    if (Keyboard::left)     dir += +1;
    if (Keyboard::right)    dir += -1;

    if (dir != 0.0) dir /= std::abs(dir);

    auto n = std::polar(1.0, -player.camera().yaw);
    Gyrovector<Real> velocity(player.walkSpeed * dir * n);

    bool chunkChanged = move(player, velocity, dt);
    if (chunkChanged) atlas.updateMatrix(player.camera().position.action());

    auto origin = player.camera().position.domain().inverse();

    if (Mouse::grabbed) {
        glfwGetCursorPos(window, &Mouse::xpos, &Mouse::ypos);
        glfwSetCursorPos(window, Window::width/2, Window::height/2);

        player.rotate(
            Mouse::speed * dt * (Window::width/2 - Mouse::xpos),
            Mouse::speed * dt * (Window::height/2 - Mouse::ypos),
            0.0f
        );
    }

    auto direction = player.camera().direction(), right = player.camera().right(), up = glm::cross(right, direction);
    auto eye = glm::vec3(0.0f, -player.camera().climb - player.eye, 0.0f);

    view = glm::lookAt(glm::vec3(0.0f), direction, up);
    view = glm::scale(view, glm::vec3(1.0f, Fundamentals::meter, 1.0f));
    view = glm::translate(view, eye);

    voxelShader->activate();

    voxelShader->uniform("view", view);
    voxelShader->uniform("projection", projection);

    voxelShader->uniform("origin.a", origin.a);
    voxelShader->uniform("origin.b", origin.b);
    voxelShader->uniform("origin.c", origin.c());
    voxelShader->uniform("origin.d", origin.d());

    glBlendFunc(GL_ONE, GL_ZERO);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (auto chunk : atlas.get()) {
        if (chunk->needRefresh())
            chunk->refresh(Registry::node, player.camera().position.action());

        if (chunk->awayness() <= Render::distance)
            chunk->render(voxelShader);
    }

    if (auto value = pbo.read(Window::width/2 - 1, Window::height/2))
    { auto [zbuffer, action] = *value; click(origin, zbuffer, action); }

    dummyShader->activate();

    glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);
    aimVao.draw(GL_LINES);
}

void setupSheet() {
    glEnable(GL_TEXTURE_2D); glEnable(GL_CULL_FACE);

    Game::Registry::sheet.pack();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, Game::Registry::sheet.texture());

    voxelShader->activate();
    voxelShader->uniform("textureSheet", 0);
}

void grabMouse(GLFWwindow * window) {
    using namespace Game;

    glfwSetCursorPos(window, Window::width/2, Window::height/2);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    Mouse::grabbed = true;
}

void freeMouse(GLFWwindow * window) {
    using namespace Game;

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);

    Mouse::grabbed = false;
}

void mouseButtonCallback(GLFWwindow * window, int button, int action, int mods) {
    using namespace Game;

    if (Mouse::grabbed) switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT: case GLFW_MOUSE_BUTTON_RIGHT: {
            if (action == GLFW_PRESS)
                pbo.issue((button == GLFW_MOUSE_BUTTON_LEFT) ? Action::Remove : Action::Place);
            break;
        }
    }

    if (Window::hovered && Window::focused && !Mouse::grabbed) grabMouse(window);
}

void cursorEnterCallback(GLFWwindow * window, int entered) {
    using namespace Game;

    Window::hovered = entered;
    if (!Window::hovered) freeMouse(window);
}

void windowFocusCallback(GLFWwindow * window, int focused) {
    using namespace Game;

    Window::focused = focused;
    if (!Window::focused) freeMouse(window);
}

void keyboardCallback(GLFWwindow * window, int key, int scancode, int action, int mods) {
    using namespace Game;

    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_ESCAPE:     glfwSetWindowShouldClose(window, GL_TRUE);     break;
            case GLFW_KEY_W:          Keyboard::forward  = true;                     break;
            case GLFW_KEY_S:          Keyboard::backward = true;                     break;
            case GLFW_KEY_A:          Keyboard::left     = true;                     break;
            case GLFW_KEY_D:          Keyboard::right    = true;                     break;
            case GLFW_KEY_O:          returnToSpawn();                               break;
            case GLFW_KEY_K:          player.roc(0); player.noclip = !player.noclip; break;
            case GLFW_KEY_SPACE:      Keyboard::space = true; pressSpace();          break;
            case GLFW_KEY_LEFT_SHIFT: Keyboard::lshift = true; pressLShift();        break;
        }
    }

    if (action == GLFW_RELEASE) {
        switch (key) {
            case GLFW_KEY_W:          Keyboard::forward  = false;                  break;
            case GLFW_KEY_S:          Keyboard::backward = false;                  break;
            case GLFW_KEY_A:          Keyboard::left     = false;                  break;
            case GLFW_KEY_D:          Keyboard::right    = false;                  break;
            case GLFW_KEY_SPACE:      Keyboard::space    = false; releaseSpace();  break;
            case GLFW_KEY_LEFT_SHIFT: Keyboard::lshift   = false; releaseLShift(); break;
        }
    }
}

void drawAim(Shader<DummyShader>::VAO & vao) {
    using namespace Game;

    vao.clear();

    constexpr auto white = glm::vec4(1.0);
    vao.push(); vao.emit(glm::vec3(-GUI::aimSize / Window::aspect, 0, 0), white);
    vao.push(); vao.emit(glm::vec3(+GUI::aimSize / Window::aspect, 0, 0), white);

    vao.push(); vao.emit(glm::vec3(0, -GUI::aimSize, 0), white);
    vao.push(); vao.emit(glm::vec3(0, +GUI::aimSize, 0), white);

    vao.upload(GL_STATIC_DRAW);
}

void setupWindowSize(GLFWwindow * window, int width, int height) {
    using namespace Game;
    using namespace Render;

    Window::width  = width;
    Window::height = height;
    Window::aspect = Real(width) / Real(height);

    int frameBufferWidth, frameBufferHeight;
    glfwGetFramebufferSize(window, &frameBufferWidth, &frameBufferHeight);
    glViewport(0, 0, frameBufferWidth, frameBufferHeight);

    projection = glm::perspective(glm::radians(fov), Game::Window::aspect, near, far);

    drawAim(aimVao);
}

constexpr auto title = "Hypertest";
GLFWwindow * setupWindow(Config & config) {
    using namespace Game;

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
    glewExperimental = GL_TRUE; glewInit();

    glEnable(GL_BLEND); glEnable(GL_DEPTH_TEST);

    voxelShader = new Shader<VoxelShader>("shaders/Voxel/Common.glsl", "shaders/Voxel/Vertex.glsl", "shaders/Voxel/Fragment.glsl");
    voxelShader->activate();

    voxelShader->uniform("fog.enabled", config.fog.enabled);
    voxelShader->uniform("fog.near",    config.fog.near);
    voxelShader->uniform("fog.far",     config.fog.far);
    voxelShader->uniform("fog.color",   config.fog.color);

    Game::Render::fov  = config.camera.fov;
    Game::Render::near = config.camera.near;
    Game::Render::far  = config.camera.far;

    dummyShader = new Shader<DummyShader>("shaders/Dummy/Common.glsl", "shaders/Dummy/Vertex.glsl", "shaders/Dummy/Fragment.glsl");
    dummyShader->activate();

    aimVao.initialize();
    Game::GUI::aimSize = config.gui.aimSize;
    setupWindowSize(window, Game::Window::width, Game::Window::height);

    pbo.initialize();
}

Chunk * buildFloor(Chunk * chunk) {
    using namespace Fundamentals;

    for (size_t i = 0; i < chunkSize; i++)
        for (size_t j = 0; j < chunkSize; j++)
            chunk->set(i, 0, j, {1});

    chunk->set(1, 1, 1, {2});
    chunk->set(1, 2, 1, {2});

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
    using namespace Game;

    atlas.onLoad = &buildFloor;

    player.eye = 1.62;
    player.height = 1.8;
    player.gravity = 9.8;
    player.jumpHeight(1.25);
    player.walkSpeed = 2 * Fundamentals::meter;

    Render::distance = chunkDiameter(config.camera.chunkRenderDistance);

    buildTestStructure(atlas.poll(Tesselation::I, Tesselation::I));

    for (std::size_t k = 0; k < Tesselation::neighbours.size(); k++) {
        auto C = buildTestStructure(atlas.poll(Tesselation::I, Tesselation::neighbours[k]));
        if (k == 0) markChunk(C);
    }

    player.teleport(Position(), 10);
}

void cleanUp(GLFWwindow * window) {
    pbo.free();
    aimVao.free();

    delete dummyShader;
    delete voxelShader;

    glfwDestroyWindow(window);
    glfwTerminate();
}

int main(int argc, char *argv[]) {
    Lua::VM vm;

    Config config(&vm, "config.lua");

    Game::window = setupWindow(config);

    setupGL(Game::window, config);

    vm.loadapi();
    for (int i = 1; i < argc; i++)
        vm.go(argv[i]);

    setupGame(config);
    setupSheet();

    while (!glfwWindowShouldClose(Game::window)) {
        display(Game::window);
        glfwSwapBuffers(Game::window);
        glfwPollEvents();
    }

    cleanUp(Game::window);

    return 0;
}
