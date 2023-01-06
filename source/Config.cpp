#include <Hyper/Config.hpp>

Config::Config(Lua::VM * vm, const char * filename) {
    vm->loadfile(filename, 1);

    fov = vm->withfield("fov", [&]() {
        return vm->get<std::optional<lua_Number>>();
    }).value_or(80.0);

    near = vm->withfield("near", [&]() {
        return vm->get<std::optional<lua_Number>>();
    }).value_or(1e-3);

    far = vm->withfield("far", [&]() {
        return vm->get<std::optional<lua_Number>>();
    }).value_or(150.0);

    vm->withfield("fog", [&]() {
        if (!vm->instanceof<Lua::Type::Table>())
            throw std::runtime_error("`fog` expected to be a table");

        fog.enabled = vm->withfield("enabled", [&]() {
            return vm->get<std::optional<bool>>();
        }).value_or(false);

        fog.min = vm->withfield("min", [&]() {
            return vm->get<std::optional<lua_Number>>();
        }).value_or(1.0);

        fog.max = vm->withfield("max", [&]() {
            return vm->get<std::optional<lua_Number>>();
        }).value_or(10.0);

        fog.color = vm->withfield("color", [&]() {
            return vm->get<std::optional<glm::vec4>>();
        }).value_or(glm::vec4(1.0f));
    });
}