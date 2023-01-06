#pragma once

#include <luajit-2.1/lua.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <functional>
#include <exception>
#include <optional>
#include <iostream>

template<typename T> concept Optional = requires(T t)
{ typename T::value_type; std::same_as<T, std::optional<typename T::value_type>>; };

template<typename F> using Retval = decltype((std::declval<F>())());

namespace Lua {

enum class Type {
    None          = LUA_TNONE,
    Nil           = LUA_TNIL,
    Boolean       = LUA_TBOOLEAN,
    LightUserData = LUA_TLIGHTUSERDATA,
    Number        = LUA_TNUMBER,
    String        = LUA_TSTRING,
    Table         = LUA_TTABLE,
    Function      = LUA_TFUNCTION,
    UserData      = LUA_TUSERDATA,
    Thread        = LUA_TTHREAD
};

class VM {
private:
    lua_State * machine;

public:
    VM();
    ~VM();

    int loadfile(const char *, int);

    inline Type type(const int index) { return static_cast<Type>(lua_type(machine, index)); }

    inline void pop() { lua_pop(machine, 1); }
    inline void pop(const int n) { lua_pop(machine, n); }

    template<typename T> T get(const int);

    template<Optional T> inline T get(const int index)
    { return instanceof<typename T::value_type>(index)
           ? std::optional(get<typename T::value_type>(index))
           : std::nullopt; }

    template<typename T> bool instanceof(const int);

    template<Type T> inline bool instanceof(const int index)
    { return type(index) == T; }

    inline bool instanceof(const int index, const Type T)
    { return type(index) == T; }

    template<typename T> inline T get() { return get<T>(-1); }
    template<typename T> inline bool instanceof() { return instanceof<T>(-1); }
    template<Type T> inline bool instanceof() { return instanceof<T>(-1); }

    template<typename F> inline Retval<F> withfield(const int index, const char * key, const F f) {
        lua_getfield(machine, index, key);

        if constexpr(std::same_as<Retval<F>, void>) { f(); pop(); }
        else { auto retval = f(); pop(); return retval; }
    }

    template<typename F> inline Retval<F> withfield(const int index, const int k, const F f) {
        lua_rawgeti(machine, index, k);

        if constexpr(std::same_as<Retval<F>, void>) { f(); pop(); }
        else { auto retval = f(); pop(); return retval; }
    }

    template<typename F, typename Ix> inline Retval<F> withfield(Ix k, const F f)
    { return withfield<F>(-1, k, f); }
};

template<> inline bool VM::instanceof<void>(const int index)
{ return lua_isnil(machine, index); }

template<> inline bool VM::instanceof<bool>(const int index)
{ return lua_isboolean(machine, index); }

template<> inline bool VM::get<bool>(const int index)
{ return lua_toboolean(machine, index); }

template<> inline bool VM::instanceof<lua_Number>(const int index)
{ return lua_isnumber(machine, index); }

template<> inline lua_Number VM::get<lua_Number>(const int index)
{ return lua_tonumber(machine, index); }

template<> inline bool VM::instanceof<lua_Integer>(const int index)
{ return lua_isnumber(machine, index); }

template<> inline lua_Integer VM::get<lua_Integer>(const int index)
{ return lua_tointeger(machine, index); }

template<> inline bool VM::instanceof<const char*>(const int index)
{ return lua_isstring(machine, index); }

template<> inline const char * VM::get<const char *>(const int index)
{ return lua_tostring(machine, index); }

template<> inline bool VM::instanceof<glm::vec2>(const int index) {
    if (!lua_istable(machine, index)) return false;

    auto φ = [this, index]() { return this->instanceof<lua_Number>(index); };
    return withfield(1, φ) && withfield(2, φ);
}

template<> inline glm::vec2 VM::get<glm::vec2>(const int index) {
    if (!lua_istable(machine, index)) throw std::runtime_error("VM::get expected glm::vec2");

    auto φ = [this, index]() { return this->get<lua_Number>(index); };
    return glm::vec2(withfield(1, φ), withfield(2, φ));
}

template<> inline bool VM::instanceof<glm::vec3>(const int index) {
    if (!lua_istable(machine, index)) return false;

    auto φ = [this, index]() { return this->instanceof<lua_Number>(index); };
    return withfield(1, φ) && withfield(2, φ) && withfield(3, φ);
}

template<> inline glm::vec3 VM::get<glm::vec3>(const int index) {
    if (!lua_istable(machine, index)) throw std::runtime_error("VM::get expected glm::vec3");

    auto φ = [this, index]() { return this->get<lua_Number>(index); };
    return glm::vec3(withfield(1, φ), withfield(2, φ), withfield(3, φ));
}

template<> inline bool VM::instanceof<glm::vec4>(const int index) {
    if (!lua_istable(machine, index)) return false;

    auto φ = [this, index]() { return this->instanceof<lua_Number>(index); };
    return withfield(1, φ) && withfield(2, φ) && withfield(3, φ) && withfield(4, φ);
}

template<> inline glm::vec4 VM::get<glm::vec4>(const int index) {
    if (!lua_istable(machine, index)) throw std::runtime_error("VM::get expected glm::vec4");

    auto φ = [this, index]() { return this->get<lua_Number>(index); };
    return glm::vec4(withfield(1, φ), withfield(2, φ), withfield(3, φ), withfield(4, φ));
}

}