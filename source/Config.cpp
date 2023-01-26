#include <Hyper/Config.hpp>

Config::Config(Lua::VM * vm, const char * filename) {
    vm->loadfile(filename, 1);

    vm->withfield("window", [&]() {
        if (!vm->instanceof<Lua::Type::Table>())
            throw std::runtime_error("`window` expected to be a table");

        window.width = vm->withfield("width", [&]() {
            return vm->get<std::optional<lua_Integer>>();
        }).value_or(900);

        window.height = vm->withfield("height", [&]() {
            return vm->get<std::optional<lua_Integer>>();
        }).value_or(900);
    });

    vm->withfield("camera", [&]() {
        if (!vm->instanceof<Lua::Type::Table>())
            throw std::runtime_error("`camera` expected to be a table");

        camera.chunkRenderDistance = vm->withfield("chunkRenderDistance", [&]() {
            return vm->get<std::optional<lua_Number>>();
        }).value_or(10.0);

        camera.fov = vm->withfield("fov", [&]() {
            return vm->get<std::optional<lua_Number>>();
        }).value_or(80.0);

        camera.near = vm->withfield("near", [&]() {
            return vm->get<std::optional<lua_Number>>();
        }).value_or(1e-3);

        camera.far = vm->withfield("far", [&]() {
            return vm->get<std::optional<lua_Number>>();
        }).value_or(150.0);

        camera.model = Model(vm->withfield("model", [&]() {
            return vm->get<std::optional<lua_Integer>>();
        }).value_or(1));
    });

    vm->withfield("fog", [&]() {
        if (!vm->instanceof<Lua::Type::Table>())
            throw std::runtime_error("`fog` expected to be a table");

        fog.enabled = vm->withfield("enabled", [&]() {
            return vm->get<std::optional<bool>>();
        }).value_or(false);

        fog.near = vm->withfield("near", [&]() {
            return vm->get<std::optional<lua_Number>>();
        }).value_or(1.0);

        fog.far = vm->withfield("far", [&]() {
            return vm->get<std::optional<lua_Number>>();
        }).value_or(10.0);

        fog.color = vm->withfield("color", [&]() {
            return vm->get<std::optional<glm::vec4>>();
        }).value_or(glm::vec4(1.0f));
    });

    vm->withfield("gui", [&]() {
        if (!vm->instanceof<Lua::Type::Table>())
            throw std::runtime_error("`gui` expected to be a table");

        gui.aimSize = vm->withfield("aimSize", [&]() {
            return vm->get<std::optional<lua_Number>>();
        }).value_or(0.015);
    });
}