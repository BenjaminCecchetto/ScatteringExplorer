#version 440 core

layout(location=0) in vec2 vertex;
layout(location=1) in vec2 texcoord;

out vec4 xfbPosition;
out vec4 xfbVelocity;

out vec2 xfbTexCoord;

void main() {
    xfbPosition = vec4(vertex, texcoord);
    xfbVelocity = vec4(texcoord, vertex);
    xfbTexCoord = texcoord;
    gl_Position = xfbPosition;
}
