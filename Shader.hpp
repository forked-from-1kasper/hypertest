#pragma once

#include <GL/glew.h>
#include <string>

class Shader {
private:
    GLuint _index;

public:
    Shader(const char * vertex, const char * fragment);
    ~Shader();

    template<typename T> void uniform(const char *, const T &) const;
    void activate();

    inline constexpr auto index() const { return _index; }
};