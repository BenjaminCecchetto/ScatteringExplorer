#version 430 core

layout(location=0) in vec2 position;

out vec2 texCoord;

uniform vec2 texPosition;
uniform vec2 texSize;

void main() {
    texCoord = position*0.5+0.5; // in [0,0]x[1,1]

    vec2 screenCoord = texCoord*texSize + texPosition; //screen coordinates

    screenCoord = (screenCoord/1080.0)*2.0 - 1.0; //back to clipping space

    gl_Position = vec4(screenCoord, 0.0, 1.0);

    texCoord.y = 1.0-texCoord.y; //invert y
}
