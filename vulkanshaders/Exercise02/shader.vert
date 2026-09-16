#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;

layout(location = 0) out vec4 interColor;

void main()
{
	vec4 pos = vec4(position, 1.0);
	
	gl_Position = pos;

	interColor = color;	
}
