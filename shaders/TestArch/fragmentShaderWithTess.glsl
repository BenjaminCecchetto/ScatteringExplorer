#version 440 core

layout (std430, binding=0) buffer TestSSBO {
    vec4 ssboColor;
}; 

layout(std140) uniform TestBuffer {
    vec4 green;
    vec4 blue;
    float interpValue;
};

layout(std140) uniform TestBuffer2 {
    vec4 red;
    vec4 alpha;
};

layout(binding=0) uniform sampler2D tex0;
layout(binding=1) uniform sampler2D tex1;

in vec2 testexcoord;

layout(location=0) out vec4 fragColor;
layout(location=1) out vec4 fragColor2;

void main() {
    fragColor = mix(texture(tex0,testexcoord), max(texture(tex1,testexcoord), interpValue), gl_FragCoord.x/1920.0);
    fragColor2 = mix(green+red,vec4(1.0-fragColor.xyz, 1.0), 0.5);
}
