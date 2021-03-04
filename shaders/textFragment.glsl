#version 430 core

layout(location = 0) out vec4 fragColor;
layout(binding=0) uniform sampler2D textTexture;

in vec2 texCoord;

void main() {
  fragColor = texture(textTexture, texCoord);
}
