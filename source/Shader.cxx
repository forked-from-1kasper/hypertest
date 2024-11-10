#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <complex>
#include <string>

#include <Hyper/Shader.hxx>

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

template class ShaderProgram<VoxelShaderSpec>;
template class ShaderProgram<DummyShaderSpec>;