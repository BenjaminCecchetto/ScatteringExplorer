#version 430 core
 
 layout(binding=0) uniform sampler2D tex;
 
  in vec2 texCoord;
 
layout(location=0) out vec4 fragColor;
 
 uniform uint numRays;
 
void main()
{
    fragColor = texture(tex,texCoord);//log(texture(tex, texCoord)+1.0)/10;
    fragColor.a = 1.0;
}