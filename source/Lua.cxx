#include <libgen.h>
#include <string.h>
#include <stdio.h>

#include <Hyper/Game.hxx>
#include <Lua.hxx>

LuaJIT::LuaJIT() {
    vm = luaL_newstate();
    luaL_openlibs(vm);
}

LuaJIT::~LuaJIT() {
    lua_close(vm);
}

const char * error(const int errcode) {
    switch (errcode) {
        case LUA_ERRSYNTAX: return "LUA_ERRSYNTAX";
        case LUA_ERRMEM:    return "LUA_ERRSYNTAX";
        case LUA_ERRFILE:   return "LUA_ERRFILE";
        case LUA_ERRRUN:    return "LUA_ERRRUN";
        case LUA_ERRERR:    return "LUA_ERRERR";
        default:            return "LUA_UNKNOWN";
    }
}

inline LuaRef warning(lua_State * vm, int errcode) {
    fprintf(stderr, "[%s] Lua: %s\n", error(errcode), lua_tostring(vm, -1));
    lua_pop(vm, 1);

    lua_pushnil(vm);
    return LuaRef(vm);
}

LuaRef LuaJIT::require(const char * filename) {
    if (auto error = luaL_loadfile(vm, filename))
        return warning(vm, error);

    if (auto error = lua_pcall(vm, 0, 1, 0))
        return warning(vm, error);

    return LuaRef(vm);
}

static const char proxyname[] = "core";

LuaRef LuaJIT::go(const char * filename) {
    char * buff;

    lua_getglobal(vm, proxyname);

    lua_pushstring(vm, filename);
    lua_setfield(vm, -2, "filename");

    buff = strdup(filename); lua_pushstring(vm, dirname(buff));
    lua_setfield(vm, -2, "dirname"); free(buff);

    buff = strdup(filename); lua_pushstring(vm, basename(buff));
    lua_setfield(vm, -2, "basename"); free(buff);

    lua_pop(vm, 1);

    return require(filename);
}

namespace API {
    enum Kind {
        Texture,
        Node
    };

    int setHotbar(lua_State * vm) {
        auto index = luaL_checkinteger(vm, 1);
        auto id = size_t(luaL_checkinteger(vm, 2));

        if (id < Game::hotbarSize) Game::hotbar[index] = id;

        return 0;
    }

    vec4 readRGBA(lua_State * vm, int argn) {
        lua_Integer rgba = luaL_checkinteger(vm, argn);

        int r = (rgba >> 24) & 0xFF, g = (rgba >> 16) & 0xFF;
        int b = (rgba >>  8) & 0xFF, a = (rgba >>  0) & 0xFF;

        return vec4(float(r) / 255.0f, float(g) / 255.0f, float(b) / 255.0f, float(a) / 255.0f);
    }

    int attachTexture(lua_State * vm) {
        using namespace Game::Registry;

        size_t retval;

        if (lua_gettop(vm) > 2)
            retval = sheet.attach(readRGBA(vm, 2), readRGBA(vm, 3), readRGBA(vm, 4), readRGBA(vm, 5));
        else
            retval = sheet.attach(readRGBA(vm, 2));

        lua_pushnumber(vm, retval);
        return 1;
    }

    int attachNode(lua_State * vm) {
        using namespace Game::Registry; NodeDef def;
        luaL_checktype(vm, 2, LUA_TTABLE);

        lua_getfield(vm, 2, "name");
        def.name = std::string(luaL_checkstring(vm, -1));
        lua_pop(vm, 1);

        lua_getfield(vm, 2, "textures");
        lua_rawgeti(vm, -1, 1); def.cube.top    = sheet.get(luaL_checkinteger(vm, -1)); lua_pop(vm, 1);
        lua_rawgeti(vm, -1, 2); def.cube.bottom = sheet.get(luaL_checkinteger(vm, -1)); lua_pop(vm, 1);
        lua_rawgeti(vm, -1, 3); def.cube.left   = sheet.get(luaL_checkinteger(vm, -1)); lua_pop(vm, 1);
        lua_rawgeti(vm, -1, 4); def.cube.right  = sheet.get(luaL_checkinteger(vm, -1)); lua_pop(vm, 1);
        lua_rawgeti(vm, -1, 5); def.cube.front  = sheet.get(luaL_checkinteger(vm, -1)); lua_pop(vm, 1);
        lua_rawgeti(vm, -1, 6); def.cube.back   = sheet.get(luaL_checkinteger(vm, -1)); lua_pop(vm, 1);
        lua_pop(vm, 1);

        auto retval = node.attach(def);
        lua_pushnumber(vm, retval);
        return 1;
    }

    static int attach(lua_State * vm) {
        auto kind = Kind(luaL_checknumber(vm, 1));

        switch (kind) {
            case Texture: return attachTexture(vm);
            case Node:    return attachNode(vm);
            default:      return 0;
        }
    }

    static int override(lua_State * vm) {
        using namespace Game;
        luaL_checktype(vm, 1, LUA_TTABLE);

        lua_getfield(vm, 1, "eye"); player.eye = luaL_checknumber(vm, -1); lua_pop(vm, 1);
        lua_getfield(vm, 1, "height"); player.height = luaL_checknumber(vm, -1); lua_pop(vm, 1);
        lua_getfield(vm, 1, "gravity"); player.gravity = luaL_checknumber(vm, -1); lua_pop(vm, 1);
        lua_getfield(vm, 1, "jump"); player.jumpHeight(luaL_checknumber(vm, -1)); lua_pop(vm, 1);
        lua_getfield(vm, 1, "walk"); player.walkSpeed = luaL_checknumber(vm, -1) * Tesselation::meter; lua_pop(vm, 1);

        return 0;
    }

    static int background(lua_State * vm) {
        using namespace Game;

        Render::background[0] = luaL_checknumber(vm, 1);
        Render::background[1] = luaL_checknumber(vm, 2);
        Render::background[2] = luaL_checknumber(vm, 3);
        Render::background[3] = luaL_checknumber(vm, 4);

        return 0;
    }
}

static const luaL_Reg externs[] = {
    {"register",   API::attach},
    {"override",   API::override},
    {"setHotbar",  API::setHotbar},
    {"background", API::background},
    {NULL,         NULL}
};

void LuaJIT::loadapi() {
    luaL_register(vm, proxyname, externs);

    lua_pushnumber(vm, API::Kind::Texture);
    lua_setfield(vm, -2, "TEXTURE");

    lua_pushnumber(vm, API::Kind::Node);
    lua_setfield(vm, -2, "NODE");

    lua_pop(vm, 1);
}
