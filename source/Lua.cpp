#include <libgen.h>
#include <string.h>

#include <Hyper/Game.hpp>
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

namespace API {
    enum Kind {
        Texture,
        Node
    };

    int attachTexture(lua_State * machine) {
        auto filename = luaL_checkstring(machine, 2);
        auto retval = Game::Registry::sheet.attach(filename);

        lua_pushnumber(machine, retval);
        return 1;
    }

    int attachNode(lua_State * machine) {
        using namespace Game::Registry; NodeDef def;
        luaL_checktype(machine, 2, LUA_TTABLE);

        lua_getfield(machine, 2, "name");
        def.name = std::string(luaL_checkstring(machine, -1));
        lua_pop(machine, 1);

        lua_getfield(machine, 2, "textures");
        lua_rawgeti(machine, -1, 1); def.cube.top    = sheet.get(luaL_checkinteger(machine, -1)); lua_pop(machine, 1);
        lua_rawgeti(machine, -1, 2); def.cube.bottom = sheet.get(luaL_checkinteger(machine, -1)); lua_pop(machine, 1);
        lua_rawgeti(machine, -1, 3); def.cube.left   = sheet.get(luaL_checkinteger(machine, -1)); lua_pop(machine, 1);
        lua_rawgeti(machine, -1, 4); def.cube.right  = sheet.get(luaL_checkinteger(machine, -1)); lua_pop(machine, 1);
        lua_rawgeti(machine, -1, 5); def.cube.front  = sheet.get(luaL_checkinteger(machine, -1)); lua_pop(machine, 1);
        lua_rawgeti(machine, -1, 6); def.cube.back   = sheet.get(luaL_checkinteger(machine, -1)); lua_pop(machine, 1);
        lua_pop(machine, 1);

        auto retval = node.attach(def);
        lua_pushnumber(machine, retval);
        return 1;
    }

    static int attach(lua_State * machine) {
        auto kind = Kind(luaL_checknumber(machine, 1));

        switch (kind) {
            case Texture: return attachTexture(machine);
            case Node:    return attachNode(machine);
            default:      return 0;
        }
    }

    static int override(lua_State * machine) {
        using namespace Game;
        luaL_checktype(machine, 1, LUA_TTABLE);

        lua_getfield(machine, 1, "eye"); player.eye = luaL_checknumber(machine, -1); lua_pop(machine, 1);
        lua_getfield(machine, 1, "height"); player.height = luaL_checknumber(machine, -1); lua_pop(machine, 1);
        lua_getfield(machine, 1, "gravity"); player.gravity = luaL_checknumber(machine, -1); lua_pop(machine, 1);
        lua_getfield(machine, 1, "jump"); player.jumpHeight(luaL_checknumber(machine, -1)); lua_pop(machine, 1);
        lua_getfield(machine, 1, "walk"); player.walkSpeed = luaL_checknumber(machine, -1) * Tesselation::meter; lua_pop(machine, 1);

        return 0;
    }
}

static const luaL_Reg externs[] = {
    {"register", API::attach},
    {"override", API::override},
    {NULL,       NULL}
};

void VM::loadapi() {
    luaL_register(machine, proxyname, externs);

    lua_pushnumber(machine, API::Kind::Texture);
    lua_setfield(machine, -2, "TEXTURE");

    lua_pushnumber(machine, API::Kind::Node);
    lua_setfield(machine, -2, "NODE");

    lua_pop(machine, 1);
}

}