#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <complex>
#include <string>

#include <Hyper/Shader.hxx>

#define lengthof(array) (sizeof(array) / sizeof(array[0]))

template<typename Spec> Shader<Spec>::Shader(const char * comfile, const char * vsfile, const char * fsfile) {
    std::ifstream cvs, ivs, ifs;

    cvs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    ivs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    cvs.open(comfile); ivs.open(vsfile); ifs.open(fsfile);

    std::stringstream csbuf, vsbuf, fsbuf;
    csbuf << cvs.rdbuf(); vsbuf << ivs.rdbuf(); fsbuf << ifs.rdbuf();

    cvs.close(); ivs.close(); ifs.close();

    GLint success; char info[Spec::infoBufferSize];

    auto cs₁ = csbuf.str(), vs₁ = vsbuf.str(), fs₁ = fsbuf.str();
    auto cs₂ = cs₁.c_str(), vs₂ = vs₁.c_str(), fs₂ = fs₁.c_str();

    const char * vertexText[] = {cs₂, vs₂}, * fragmentText[] = {cs₂, fs₂};

    auto vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, lengthof(vertexText), vertexText, 0);
    glCompileShader(vertex);

    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, Spec::infoBufferSize, 0, info);
        std::cout << "Vertex shader compilation error:\n" << info << std::endl;
    }

    auto fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, lengthof(fragmentText), fragmentText, 0);
    glCompileShader(fragment);

    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, Spec::infoBufferSize, 0, info);
        std::cout << "Fragment shader compilation error:\n" << info << std::endl;
    }

    _index = glCreateProgram();
    glAttachShader(_index, vertex);
    glAttachShader(_index, fragment);
    GVA::bind<stride, Params>(_index);
    glLinkProgram(_index);

    glGetProgramiv(_index, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(_index, Spec::infoBufferSize, 0, info);
        std::cout << "Shader program linking failure:\n" << info << std::endl;
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

template<typename Spec> Shader<Spec>::~Shader() { glDeleteProgram(_index); }

template<typename Spec> void Shader<Spec>::activate() { glUseProgram(_index); }

namespace GL {
    template<> void uniform<bool>(GLuint index, const char * name, const bool & value)
    { glUniform1i(glGetUniformLocation(index, name), int(value)); }

    template<> void uniform<int>(GLuint index, const char * name, const int & value)
    { glUniform1i(glGetUniformLocation(index, name), value); }

    template<> void uniform<float>(GLuint index, const char * name, const float & value)
    { glUniform1f(glGetUniformLocation(index, name), value); }

    template<> void uniform<std::complex<float>>(GLuint index, const char * name, const std::complex<float> & value)
    { glUniform2fv(glGetUniformLocation(index, name), 1, (GLfloat*) &value); }

    template<> void uniform<std::complex<double>>(GLuint index, const char * name, const std::complex<double> & value)
    { std::complex<float> z(value.real(), value.imag()); glUniform2fv(glGetUniformLocation(index, name), 1, (GLfloat*) &z); }

    template<> void uniform<glm::vec2>(GLuint index, const char * name, const glm::vec2 & value)
    { glUniform2fv(glGetUniformLocation(index, name), 1, glm::value_ptr(value)); }

    template<> void uniform<glm::vec3>(GLuint index, const char * name, const glm::vec3 & value)
    { glUniform3fv(glGetUniformLocation(index, name), 1, glm::value_ptr(value)); }

    template<> void uniform<glm::vec4>(GLuint index, const char * name, const glm::vec4 & value)
    { glUniform4fv(glGetUniformLocation(index, name), 1, glm::value_ptr(value)); }

    template<> void uniform<glm::mat4>(GLuint index, const char * name, const glm::mat4 & value)
    { glUniformMatrix4fv(glGetUniformLocation(index, name), 1, GL_FALSE, glm::value_ptr(value)); }
}

template class Shader<VoxelShader>;
template class Shader<DummyShader>;