#version 330 core

in vec3 my_color;

out vec4 fragOutput;

void main()
{
    fragOutput = vec4(my_color, 1.0);
}
