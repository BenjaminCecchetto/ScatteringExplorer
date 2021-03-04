#version 430 core

layout(location = 0) out vec4 fragColor;

uniform float pathDensities;

flat in double vsIntensity;

void main()
{
    fragColor = vec4(vsIntensity*pathDensities*dvec4(1.0,1.0,1.0,1.0));
}
