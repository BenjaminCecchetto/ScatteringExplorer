#version 440 core

layout(vertices=4) out;

in vec2 vtexcoord[];
out vec2 tcstexcoord[];

void main() {
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    tcstexcoord[gl_InvocationID] = vtexcoord[gl_InvocationID];
    
    if (gl_InvocationID == 0) {
        gl_TessLevelOuter[0] = 2.0;
        gl_TessLevelOuter[1] = 2.0;
        gl_TessLevelOuter[2] = 2.0;
        gl_TessLevelOuter[3] = 2.0;
        
        gl_TessLevelInner[0] = 2.0;
        gl_TessLevelInner[1] = 2.0;
    }
    
   
}