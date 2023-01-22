#pragma once

#include <string>
#include <vector>

#include <Hyper/Fundamentals.hpp>

class Texture; class Sheet;

class Texture {
private:
    Sheet * sheet; size_t index;
    GLfloat _left, _right, _down, _up;

public:
    Texture();
    Texture(Sheet *, size_t);

    inline constexpr bool empty() const { return sheet == nullptr; }

    inline constexpr auto left()  const { return _left;  }
    inline constexpr auto right() const { return _right; }
    inline constexpr auto down()  const { return _down;  }
    inline constexpr auto up()    const { return _up;    }
};

class Sheet {
private:
    GLuint _texture;
    unsigned long _size, _total;
    std::vector<std::string> _files;

public:
    Sheet(unsigned long, unsigned long);
    Texture attach(const std::string &);
    void pack();

    inline constexpr auto texture()  const { return _texture;         }
    inline constexpr auto size()     const { return _size;            }
    inline constexpr auto total()    const { return _total;           }
    inline constexpr auto capacity() const { return total() / size(); }

    inline const std::vector<std::string> files() const { return _files; }
    inline auto occupancy() const { return _files.size(); }

    inline auto nth(size_t idx) { return Texture(this, idx); }

    inline const bool full() const { return _files.size() == Math::sqr(capacity()); }
    inline const auto index(size_t k) { return std::pair(k / capacity(), k % capacity()); }

};