#version 440 core

layout(location=0) in vec4 position;
layout(location=1) in vec4 velocity;
layout(location=2) in vec2 texcoord;

out vec2 vtexcoord;

void main() {
    vtexcoord = texcoord.xy;
    gl_Position = vec4(position.xy, 0.0, 1.0);
}
