#pragma once

#include <string>
#include <vector>

#include <Hyper/Fundamentals.hxx>

class Texture {
    vec4 _lu, _ru, _rd, _ld;

public:
    inline Texture() {}

    inline Texture(const vec4 & rgba) :
    _lu(rgba), _ru(rgba), _rd(rgba), _ld(rgba) {}

    inline Texture(const vec4 & lu, const vec4 & ru, const vec4 & rd, const vec4 & ld) :
    _lu(lu), _ru(ru), _rd(rd), _ld(ld) {}

    inline constexpr auto & lu() { return _lu; }
    inline constexpr auto & ru() { return _ru; }
    inline constexpr auto & rd() { return _rd; }
    inline constexpr auto & ld() { return _ld; }
};

class Sheet {
private:
    std::vector<Texture> _textures;

public:

    template<typename... Ts> inline size_t attach(const Ts &... ts) {
        size_t index = _textures.size();
        _textures.emplace_back(ts...);

        return index;
    }

    inline auto size() const { return _textures.size(); }
    inline auto get(size_t idx) { return _textures[idx]; }
};