#pragma once

#include <functional>
#include <exception>
#include <optional>
#include <iostream>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <luajit-2.1/lua.hpp>

#include <Meta/Basic.hxx>
#include <Meta/List.hxx>

template<typename T> inline T decode(lua_State * vm, const int index) {
    if constexpr(std::same_as<T, bool>)
        return lua_toboolean(vm, index);
    else if constexpr(std::same_as<T, lua_Integer>)
        return lua_tointeger(vm, index);
    else if constexpr(std::same_as<T, lua_Number>)
        return lua_tonumber(vm, index);
    else if constexpr(std::same_as<T, const char *>)
        return lua_tostring(vm, index);
    else
        static_assert(falsehood<T>);
}

template<typename T> inline bool valid(lua_State * vm, const int index) {
    if constexpr(std::same_as<T, void>)
        return lua_isnil(vm, index);
    else if constexpr(std::same_as<T, bool>)
        return lua_isboolean(vm, index);
    else if constexpr(std::same_as<T, lua_Integer>)
        return lua_isnumber(vm, index);
    else if constexpr(std::same_as<T, lua_Number>)
        return lua_isnumber(vm, index);
    else if constexpr(std::same_as<T, const char *>)
        return lua_isstring(vm, index);
    else
        static_assert(falsehood<T>);
}

class LuaRef {
protected:
    lua_State * vm; int index;
public:
    inline LuaRef(lua_State * vm) : vm(vm), index(lua_gettop(vm)) {}
    inline ~LuaRef() { if (index > 0) lua_pop(vm, 1); }

    inline LuaRef(LuaRef && rvalue) {
        vm = rvalue.vm;
        index = rvalue.index;

        rvalue.vm = nullptr;
        rvalue.index = 0;
    };

    LuaRef(const LuaRef &) = delete;
    LuaRef & operator=(const LuaRef &) = delete;
    LuaRef & operator=(LuaRef && rvalue) = delete;

    inline operator bool() const { return !lua_isnil(vm, index); }

    inline void invalidate() { lua_pushnil(vm); lua_replace(vm, index - 1); }
};

template<typename T> class LuaVal : public LuaRef {
public:
    LuaVal(lua_State *) = delete;

    inline LuaVal(LuaRef && rvalue) : LuaRef(std::move(rvalue))
    { if (!valid<T>(vm, index)) invalidate(); };

    inline T decode() const { return ::decode<T>(vm, index); }
};

class LuaTable : public LuaRef {
public:
    LuaTable(lua_State *) = delete;

    inline LuaTable(LuaRef && rvalue) : LuaRef(std::move(rvalue))
    { if (!lua_istable(vm, index)) invalidate(); };

    inline LuaRef getitem(const char * key)
    { lua_getfield(vm, index, key); return LuaRef(vm); }

    inline LuaRef getitem(const lua_Integer key)
    { lua_rawgeti(vm, index, key); return LuaRef(vm); }
};

class LuaJIT {
private:
    lua_State * vm;

public:
    LuaJIT();
    ~LuaJIT();

    LuaRef loadfile(const char *);

    LuaRef go(const char *);

    void loadapi();
};

template<typename... Is> struct LuaTupleM {
    static inline bool valid(lua_State * vm, const int index) {
        if (!lua_istable(vm, index)) return false;

        int table = lua_gettop(vm); (lua_rawgeti(vm, table, sizeof...(Is) - Is::index), ...);

        auto retval = (::valid<typename Is::typeval>(vm, -Is::index - 1) && ...);
        lua_pop(vm, int(sizeof...(Is))); return retval;
    }

    template<typename Tuple> static inline Tuple decode(lua_State * vm, const int index) {
        int table = lua_gettop(vm); (lua_rawgeti(vm, table, sizeof...(Is) - Is::index), ...);

        Tuple retval{::decode<typename Is::typeval>(vm, -Is::index - 1)...};
        lua_pop(vm, int(sizeof...(Is))); return retval;
    }
};

template<typename... Ts> using LuaTuple = Apply<LuaTupleM, Enumerate<Ts...>>;

using LuaVec2 = LuaTuple<lua_Number, lua_Number>;

template<> inline bool valid<glm::vec2>(lua_State * vm, const int index)
{ return LuaVec2::valid(vm, index); }

template<> inline glm::vec2 decode<glm::vec2>(lua_State * vm, const int index)
{ return LuaVec2::decode<glm::vec2>(vm, index); }

using LuaVec3 = LuaTuple<lua_Number, lua_Number, lua_Number>;

template<> inline bool valid<glm::vec3>(lua_State * vm, const int index)
{ return LuaVec3::valid(vm, index); }

template<> inline glm::vec3 decode<glm::vec3>(lua_State * vm, const int index)
{ return LuaVec3::decode<glm::vec3>(vm, index); }

using LuaVec4 = LuaTuple<lua_Number, lua_Number, lua_Number, lua_Number>;

template<> inline bool valid<glm::vec4>(lua_State * vm, const int index)
{ return LuaVec4::valid(vm, index); }

template<> inline glm::vec4 decode<glm::vec4>(lua_State * vm, const int index)
{ return LuaVec4::decode<glm::vec4>(vm, index); }
