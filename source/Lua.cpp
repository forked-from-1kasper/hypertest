#include <Lua.hpp>

namespace Lua {

VM::VM() {
    machine = luaL_newstate();
    luaL_openlibs(machine);
}

VM::~VM() {
    lua_close(machine);
}

int VM::loadfile(const char * filename, int nresults) {
    luaL_loadfile(machine, filename);
    auto err = lua_pcall(machine, 0, nresults, 0);

    if (err == LUA_ERRRUN) std::cout << "Lua: " << lua_tostring(machine, -1) << std::endl;
    if (err == LUA_ERRMEM) { throw std::runtime_error("Lua memory allocation error"); }

    return err;
}

}