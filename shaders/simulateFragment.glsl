#version 430 core

layout(location=0) out vec4 fragColor;

in vec4 color;

void main()
{
    fragColor = color.a*vec4(color.xyz, 1.0);
}