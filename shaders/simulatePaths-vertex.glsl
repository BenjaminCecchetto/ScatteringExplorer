#version 430 core

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 intensity;
uniform mat4 MVP;

flat out double vsIntensity;

void main()
{
    gl_Position = MVP*position;

    vsIntensity = intensity.x;
}
