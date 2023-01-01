#version 330 core
layout (location = 0) in vec2 _texCoord;
layout (location = 1) in vec3 _pos;

out vec2 texCoord;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * vec4(_pos, 1.0);
    texCoord = _texCoord;
}