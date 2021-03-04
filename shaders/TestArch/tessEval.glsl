#version 440 core
 
layout(quads,equal_spacing) in;
 
 in vec2 tcstexcoord[];
 out vec2 testexcoord;
 
void main(void) {
    vec4 p0 = gl_in[0].gl_Position;
    vec4 p1 = gl_in[1].gl_Position;
    vec4 p2 = gl_in[2].gl_Position;
    vec4 p3 = gl_in[3].gl_Position;
     
    vec2 tc0 = tcstexcoord[0];
    vec2 tc1 = tcstexcoord[1];
    vec2 tc2 = tcstexcoord[2];
    vec2 tc3 = tcstexcoord[3];
    
    vec2 uv = gl_TessCoord.xy;
 
    //mix positions
    vec4 t1 = mix(p0, p1, uv.x);
    vec4 t2 = mix(p2, p3, uv.x);
    vec4 t = mix(t1, t2, uv.y);
    
    //mix texcoords
    vec2 tcx1 = mix(tc0, tc1, uv.x);
    vec2 tcx2 = mix(tc2, tc3, uv.x);
    vec2 tc = mix(tcx1, tcx2, uv.y);
    
    
    gl_Position = t;
    testexcoord = tc;
}