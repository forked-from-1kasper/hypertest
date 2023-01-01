#include "Shader.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

constexpr size_t infoBufferSize = 2048;

Shader::Shader(const char * vsfile, const char * fsfile) {
    std::ifstream ivs, ifs;

    ivs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    ivs.open(vsfile); ifs.open(fsfile);

    std::stringstream vsbuf, fsbuf;
    vsbuf << ivs.rdbuf(); fsbuf << ifs.rdbuf();

    ivs.close(); ifs.close();

    GLint success; char info[infoBufferSize];

    auto vs₁ = vsbuf.str(), fs₁ = fsbuf.str();
    auto vs₂ = vs₁.c_str(), fs₂ = fs₁.c_str();

    auto vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vs₂, 0);
    glCompileShader(vertex);

    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, infoBufferSize, 0, info);
        std::cout << "Vertex shader compilation error:\n" << info << std::endl;
    }

    auto fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fs₂, 0);
    glCompileShader(fragment);

    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, infoBufferSize, 0, info);
        std::cout << "Fragment shader compilation error:\n" << info << std::endl;
    }

    _index = glCreateProgram();
    glAttachShader(_index, vertex);
    glAttachShader(_index, fragment);
    glLinkProgram(_index);

    glGetProgramiv(_index, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(_index, infoBufferSize, 0, info);
        std::cout << "Shader program linking failure:\n" << info << std::endl;
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

Shader::~Shader() { glDeleteProgram(_index); }

void Shader::activate() { glUseProgram(_index); }

template<> void Shader::uniform<bool>(const char * name, const bool & value) const
{ glUniform1i(glGetUniformLocation(_index, name), int(value)); }

template<> void Shader::uniform<int>(const char * name, const int & value) const
{ glUniform1i(glGetUniformLocation(_index, name), value); }

template<> void Shader::uniform<float>(const char * name, const float & value) const
{ glUniform1f(glGetUniformLocation(_index, name), value); }

template<> void Shader::uniform<glm::mat4>(const char * name, const glm::mat4 & value) const
{ glUniformMatrix4fv(glGetUniformLocation(_index, name), 1, GL_FALSE, glm::value_ptr(value)); }