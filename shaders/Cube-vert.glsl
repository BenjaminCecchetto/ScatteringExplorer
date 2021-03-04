#version 430 core

layout(location = 0) in vec4 position;

uniform mat4 MVP;
uniform vec3 cubeBound;

void main()
{
	vec4 scaledPos = vec4(position.x*cubeBound.x*0.5f,
						  position.y*cubeBound.y*0.5f,
						  position.z*cubeBound.z*0.5f, 1.0f	);

    gl_Position = MVP*scaledPos;
}
