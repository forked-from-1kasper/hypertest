#include <libgen.h>
#include <string.h>

#include <Lua.hpp>

namespace Stream {
    std::ostream & error(std::ostream & stream, const int err) {
        switch (err) {
            case LUA_ERRSYNTAX: return stream << "LUA_ERRSYNTAX";
            case LUA_ERRMEM:    return stream << "LUA_ERRSYNTAX";
            case LUA_ERRFILE:   return stream << "LUA_ERRFILE";
            case LUA_ERRRUN:    return stream << "LUA_ERRRUN";
            case LUA_ERRERR:    return stream << "LUA_ERRERR";
            default:            return stream;
        }
    }

    std::ostream & string(std::ostream & stream, lua_State * machine) {
        stream << lua_tostring(machine, -1);
        lua_pop(machine, 1); return stream;
    }
}

namespace Lua {

VM::VM() {
    machine = luaL_newstate();
    luaL_openlibs(machine);
}

VM::~VM() {
    lua_close(machine);
}

inline int warning(lua_State * machine, int error) {
    Stream::string(Stream::error(std::cout << "[", error) << "] Lua: ", machine) << std::endl;
    return error;
}

int VM::loadfile(const char * filename, int nresults) {
    if (auto error = luaL_loadfile(machine, filename))
        return warning(machine, error);

    if (auto error = lua_pcall(machine, 0, nresults, 0))
        return warning(machine, error);

    return 0;
}

const char proxyname[] = "hypertest";

int VM::go(const char * filename) {
    char * buff;

    lua_getglobal(machine, proxyname);

    lua_pushstring(machine, filename);
    lua_setfield(machine, -2, "filename");

    buff = strdup(filename); lua_pushstring(machine, dirname(buff));
    lua_setfield(machine, -2, "dirname"); free(buff);

    buff = strdup(filename); lua_pushstring(machine, basename(buff));
    lua_setfield(machine, -2, "basename"); free(buff);

    lua_pop(machine, 1);

    return loadfile(filename, 0);
}

static const luaL_Reg externs[] = {
    {NULL, NULL}
};

void VM::loadapi() {
    luaL_register(machine, proxyname, externs);

    lua_pop(machine, 1);
}

}