#include <cstdio>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

#include <Lua.hxx>

#include <Math/Gyrovector.hxx>
#include <Math/Fuchsian.hxx>

#include <Hyper/Config.hxx>
#include <Hyper/Shader.hxx>
#include <Hyper/Game.hxx>

using namespace std::complex_literals;

void errorCallback(int error, const char * description) {
    fprintf(stderr, "Error: %s\n", description);
}

glm::mat4 view, projection;

DummyShader * dummyShader = nullptr;
VoxelShader * voxelShader = nullptr;

DummyShader::VAO aimVao, hotbarVao;

PBO<GLfloat, Action> pbo(GL_DEPTH_COMPONENT, 1, 1);

inline auto vec2(GLfloat x, GLfloat y) { return glm::vec3(x / Game::Window::aspect, y, 0); }

const auto origin = glm::vec2(0.0f);

const auto white  = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
const auto black  = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

void drawAim(DummyShader::VAO & vao) {
    using namespace Game;

    vao.clear();

    auto wpixel = 1.0 / GLfloat(Window::width), hpixel = 1.0 / GLfloat(Window::height);

    vao.push(); vao.emit(glm::vec3(-GLfloat(GUI::aimSize) * wpixel, 0, 0), white, origin, 1.0f);
    vao.push(); vao.emit(glm::vec3(+GLfloat(GUI::aimSize) * wpixel, 0, 0), white, origin, 1.0f);
    vao.push(); vao.emit(glm::vec3(0, -GLfloat(GUI::aimSize) * hpixel, 0), white, origin, 1.0f);
    vao.push(); vao.emit(glm::vec3(0, +GLfloat(GUI::aimSize) * hpixel, 0), white, origin, 1.0f);

    vao.upload(GL_STATIC_DRAW);
}

void drawRectangle(DummyShader::VAO & vao, GLfloat x₀, GLfloat y₀, GLfloat Δx, GLfloat Δy, Texture & T, glm::vec4 color, GLfloat mix) {
    auto index = vao.index();

    vao.emit(vec2(x₀ + 0,  y₀ + 0),  color, glm::vec2(T.left(),  T.up()),   mix);
    vao.emit(vec2(x₀ + Δx, y₀ + 0),  color, glm::vec2(T.right(), T.up()),   mix);
    vao.emit(vec2(x₀ + Δx, y₀ + Δy), color, glm::vec2(T.right(), T.down()), mix);
    vao.emit(vec2(x₀ + 0,  y₀ + Δy), color, glm::vec2(T.left(),  T.down()), mix);

    vao.push(index); vao.push(index + 1); vao.push(index + 2);
    vao.push(index); vao.push(index + 2); vao.push(index + 3);
}

void drawHotbar(DummyShader::VAO & vao) {
    using namespace Game;

    const GLfloat size = 0.1f, gap = 0.01f;
    const auto gray = glm::vec4(0.5f);

    auto hotbarLength = hotbarSize * (size + gap);
    auto x₀ = -hotbarLength / 2, y₀ = gap - 1;
    auto T₀ = Registry::sheet.get(0);

    vao.clear();

    for (size_t i = 0; i < hotbarSize; i++) {
        auto id = hotbar[i]; auto x = x₀ + i * (size + gap), y = y₀, side = size;
        if (i == activeSlot) { x -= gap/2; y -= gap/2; side += gap; }

        if (id != 0 && Registry::node.has(id)) {
            auto nodeDef = Registry::node.get(id);
            drawRectangle(vao, x, y, side, side, nodeDef.cube.front, black, 0.0f);
        } else drawRectangle(vao, x, y, side, side, T₀, gray, 1.0f);
    }

    vao.upload(GL_STATIC_DRAW);
}

void updateHotbar() { drawHotbar(hotbarVao); }

Real chunkDiameter(const Real n) {
    using namespace Fundamentals;

    constexpr Gyrovector<Real> i(D½, 0), j(0, D½), k = Coadd(i, j);

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
    using namespace Game;
    using namespace Render;

    auto h = glm::vec3(0.0f, H, 0.0f);

    auto w₀ = standard->model.unapply(unproject(view, projection, 2.0f * zbuffer - 1.0f));
    auto dist₀ = glm::length(w₀ - h);

    const GLfloat ε = standard->meter / 3.0f;
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
    if (C == nullptr || !C->ready() || Chunk::outside(L))
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
    const auto maxₕ = 5.0 * Tesselation::meter, maxᵥ = 4.0;
    const auto H = Game::player.camera().climb + Game::player.eye;

    auto v = trace(view, projection, zbuffer, H, action == Action::Remove);
    auto P = Gyrovector(v.x, v.z);

    if (P.abs() <= maxₕ && fabs(v.y - H) <= maxᵥ) {
        if (auto ret = getNeighbour(origin.inverse().apply(P))) {
            auto [C, Q] = *ret; auto [i, k] = Chunk::round(Q);

            if (action == Action::Place && Game::activeSlot < Game::hotbarSize) {
                auto id = Game::hotbar[Game::activeSlot];
                if (id != 0 && Game::Registry::node.has(id))
                    setBlock(C, i, v.y, k, id);
            }
            if (action == Action::Remove) setBlock(C, i, v.y, k, 0);
        }
    }
}

void pollNeighbours() {
    using namespace Game;

    atlas.updateMatrix(player.camera().position.action());

    for (size_t k = 0; k < Tesselation::neighbours.size(); k++) {
        auto G = player.chunk()->isometry() * Tesselation::neighbours[k];
        atlas.poll(player.camera().position.action(), G);
    }

    /*for (size_t i = 0; i < Tesselation::neighbours.size(); i++)
        for (size_t j = 0; j < Tesselation::neighbours.size(); j++) {
            auto G = player.chunk()->isometry() * Tesselation::neighbours[i] * Tesselation::neighbours[j];
            atlas.poll(player.camera().position.action(), G);
    }*/
}

void returnToSpawn() {
    using namespace Game;

    player.teleport(Position(), 5);
    player.roc(0); pollNeighbours();
}

const double saveInterval = 1.0;

double globaltime = 0, saveTimer = 0;
void display(GLFWwindow * window) {
    using namespace Game;

    auto dt = glfwGetTime() - globaltime;
    globaltime += dt; saveTimer += dt;

    auto dir = 0i;
    if (Keyboard::forward)  dir += +1i;
    if (Keyboard::backward) dir += -1i;
    if (Keyboard::left)     dir += +1;
    if (Keyboard::right)    dir += -1;

    if (dir != 0.0) dir /= std::abs(dir);

    auto n = std::polar(1.0, -player.camera().yaw);
    Gyrovector<Real> velocity(player.walkSpeed * dir * n);

    bool chunkChanged = move(player, velocity, dt);
    if (chunkChanged) pollNeighbours();

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
    view = glm::scale(view, glm::vec3(1.0f, Render::standard->meter, 1.0f));
    view = glm::translate(view, eye);

    voxelShader->activate();
    glEnable(GL_DEPTH_TEST);

    voxelShader->uniform("view", view);
    voxelShader->uniform("projection", projection);

    voxelShader->uniform("origin.a", origin.a);
    voxelShader->uniform("origin.b", origin.b);
    voxelShader->uniform("origin.c", origin.c());
    voxelShader->uniform("origin.d", origin.d());

    glBlendFunc(GL_ONE, GL_ZERO);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (auto it = atlas.pool.begin(); it != atlas.pool.end();) {
        auto chunk = *it;

        if (!chunk->ready()) { it++; continue; }

        if (chunk->needRefresh())
            chunk->refresh(Registry::node);

        if (chunk->awayness() <= Render::distance)
            chunk->render(voxelShader);
        else chunk->unload();

        if (chunk->needUnload() && !chunk->dirty()) {
            delete chunk; it = atlas.pool.erase(it);
        } else it++;
    }

    if (auto value = pbo.read(Window::width/2 - 1, Window::height/2))
    { auto [zbuffer, action] = *value; click(origin, zbuffer, action); }

    dummyShader->activate();
    glDisable(GL_DEPTH_TEST);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    hotbarVao.draw(GL_TRIANGLES);

    glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);
    aimVao.draw(GL_LINES);

    if (saveTimer >= saveInterval)
    { atlas.dump(); saveTimer = 0; }
}

void setupSheet() {
    using namespace Game;

    glEnable(GL_TEXTURE_2D); glEnable(GL_CULL_FACE);

    Registry::sheet.pack();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, Registry::sheet.texture());

    voxelShader->activate();
    voxelShader->uniform("textureSheet", 0);

    dummyShader->activate();
    dummyShader->uniform("textureSheet", 0);
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

Blob blobBuffer;

void copyBlob() {
    using namespace Game;

    const auto & src = *player.chunk();
    if (src.blob() == nullptr) return;

    memcpy(&blobBuffer, src.blob(), sizeof(Blob));
}

void pasteBlob() {
    using namespace Game;

    auto dest = player.chunk()->blob();
    if (dest == nullptr) return;

    memcpy(dest, &blobBuffer, sizeof(Blob));
    player.chunk()->requestRefresh();
}

void rotateBlob() {
    using namespace Game;
    using namespace Fundamentals;

    auto src = player.chunk()->blob();
    if (src == nullptr) return;

    auto buf = new Blob; memcpy(buf, src, sizeof(Blob));

    for (size_t i = 0; i < chunkSize; i++)
        for (size_t j = 0; j < worldHeight; j++)
            for (size_t k = 0; k < chunkSize; k++)
                src->data[i][j][k] = buf->data[k][j][chunkSize - 1 - i];

    delete buf;

    player.chunk()->requestRefresh();
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
            case GLFW_KEY_1:          activeSlot = 0; updateHotbar();                break;
            case GLFW_KEY_2:          activeSlot = 1; updateHotbar();                break;
            case GLFW_KEY_3:          activeSlot = 2; updateHotbar();                break;
            case GLFW_KEY_4:          activeSlot = 3; updateHotbar();                break;
            case GLFW_KEY_5:          activeSlot = 4; updateHotbar();                break;
            case GLFW_KEY_6:          activeSlot = 5; updateHotbar();                break;
            case GLFW_KEY_7:          activeSlot = 6; updateHotbar();                break;
            case GLFW_KEY_8:          activeSlot = 7; updateHotbar();                break;
            case GLFW_KEY_9:          activeSlot = 8; updateHotbar();                break;
            case GLFW_KEY_X:          rotateBlob();                                  break;
            case GLFW_KEY_C:          copyBlob();                                    break;
            case GLFW_KEY_V:          pasteBlob();                                   break;
            case GLFW_KEY_BACKSLASH:  freeMouse(window);                             break;
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

void setupWindowSize(GLFWwindow * window, int width, int height) {
    using namespace Game;
    using namespace Render;

    Window::width  = width;
    Window::height = height;
    Window::aspect = Real(width) / Real(height);

    int frameBufferWidth, frameBufferHeight;
    glfwGetFramebufferSize(window, &frameBufferWidth, &frameBufferHeight);
    glViewport(0, 0, frameBufferWidth, frameBufferHeight);

    projection = glm::perspective(glm::radians(fov), Window::aspect, near, far);

    updateHotbar();
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

auto readText(const char * filepath) {
    size_t fsize; std::vector<char> retval;

    auto fd = std::fopen(filepath, "rb");
    if (fd == nullptr) goto error;

    if (std::fseek(fd, 0, SEEK_END) != 0) goto error;
    fsize = std::ftell(fd); if (fsize < 0) goto error;

    {
        std::rewind(fd);

        retval.resize(fsize + 1);
        std::fread(retval.data(), fsize, 1, fd);
        retval[fsize] = 0;

        std::fclose(fd);

        return retval;
    }

    error: fprintf(stderr, "Unable to read “%s”: %s\n", filepath, std::strerror(errno));
    return std::vector(1, '\0');
}

auto readModelShader(Model model) {
    switch (model) {
        case Poincaré:    return readText("shaders/Model/Poincare.glsl");
        case Klein:       return readText("shaders/Model/Klein.glsl");
        case Gans:        return readText("shaders/Model/Gans.glsl");
        case Equidistant: return readText("shaders/Model/Equidistant.glsl");
        case Lambert:     return readText("shaders/Model/Lambert.glsl");
        default:          return std::vector(1, '\0');
    }
}

void uploadShaders() {
    using namespace Game;

    delete voxelShader;

    auto cs₁ = readText("shaders/Voxel/Common.glsl");
    auto fs₁ = readText("shaders/Voxel/Fragment.glsl");
    auto vs₁ = readText("shaders/Voxel/Vertex.glsl");
    auto ms₁ = readModelShader(Render::standard->model);

    FragmentShader fragment₁(cs₁.data(), fs₁.data());
    VertexShader vertex₁(cs₁.data(), vs₁.data(), ms₁.data());

    voxelShader = new VoxelShader(fragment₁, vertex₁);

    delete dummyShader;

    auto cs₂ = readText("shaders/Dummy/Common.glsl");
    auto fs₂ = readText("shaders/Dummy/Fragment.glsl");
    auto vs₂ = readText("shaders/Dummy/Vertex.glsl");

    FragmentShader fragment₂(cs₂.data(), fs₂.data());
    VertexShader vertex₂(cs₂.data(), vs₂.data());

    dummyShader = new DummyShader(fragment₂, vertex₂);
}

void setupShaders(Config & config) {
    voxelShader->activate();
    voxelShader->uniform("fog.enabled", config.fog.enabled);
    voxelShader->uniform("fog.near",    config.fog.near);
    voxelShader->uniform("fog.far",     config.fog.far);
    voxelShader->uniform("fog.color",   config.fog.color);
}

void setupGL(GLFWwindow * window, Config & config) {
    using namespace Game;

    static Render::Standard standard(config.camera.model);
    Render::standard = &standard;

    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE; glewInit();

    glEnable(GL_BLEND);

    uploadShaders();
    setupShaders(config);

    Render::fov  = config.camera.fov;
    Render::near = config.camera.near;
    Render::far  = config.camera.far;

    dummyShader->activate();

    aimVao.initialize();
    hotbarVao.initialize();

    GUI::aimSize = config.gui.aimSize;
    setupWindowSize(window, Window::width, Window::height);

    pbo.initialize();
}

Chunk * buildFloor(Chunk * chunk) {
    using namespace Fundamentals;

    for (size_t i = 0; i < chunkSize; i++)
        for (size_t j = 0; j < chunkSize; j++)
            chunk->set(i, 0, j, {1});

    return chunk;
}

void setupGame(Config & config) {
    using namespace Tesselation;
    using namespace Game;

    atlas.generator = &buildFloor;
    Render::distance = chunkDiameter(config.camera.chunkRenderDistance);

    atlas.poll(Tesselation::I, Tesselation::I);

    for (std::size_t k = 0; k < Tesselation::neighbours.size(); k++)
        atlas.poll(Tesselation::I, Tesselation::neighbours[k]);

    player.teleport(Position(), 5);
}

void cleanUp(GLFWwindow * window) {
    pbo.free();
    aimVao.free();
    hotbarVao.free();

    delete dummyShader;
    delete voxelShader;

    glfwDestroyWindow(window);
    glfwTerminate();
}

int main(int argc, char * argv[]) {
    using namespace Game;

    LuaJIT luajit;

    Config config(&luajit, "config.lua");

    auto window = setupWindow(config);
    setupGL(window, config);

    luajit.loadapi();

    for (int i = 1; i < argc; i++)
        luajit.go(argv[i]);

    atlas.connect(config.world);
    setupGame(config);
    setupSheet();

    updateHotbar();

    while (!glfwWindowShouldClose(window)) {
        display(window);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    atlas.disconnect();
    cleanUp(window);

    return 0;
}
