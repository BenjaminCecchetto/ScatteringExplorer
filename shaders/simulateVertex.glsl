#version 430 core

layout(std430, binding=0) buffer PathDataBuffer {
    uint numPaths;
    uint pathLength;
    uint numIterations;
};

layout(std430, binding=1) buffer PathBuffer {
    dvec4 paths[];
};

out vec4 color;

void main()
{
   
    color = vec4(0.0);
    
    if (gl_InstanceID < numPaths) {
        color.x = 1.0;
    }
    else if (gl_InstanceID < 2*numPaths) {
        color.y = 1.0;
    }
    else {
        color.z = 1.0;
    }
    
    dvec4 pathI = paths[gl_InstanceID * pathLength + gl_VertexID];
    
    //volume is [0, -5]x[10,5] to screen [-1,-1]x[1,1]
    vec2 pi = vec2(pathI.xy/5.0 + vec2(-1.0, 0.0));
    
    color = color*float(pathI.z)/float(numPaths*numIterations/50);
    //color = color*0.1;
    
    color = clamp(color, 0.0, 1.0);
    
    if (pathI.w > 0.0)
        color.a = 1.0;
    
    gl_Position = vec4(pi,0.0,1.0);
    
}
