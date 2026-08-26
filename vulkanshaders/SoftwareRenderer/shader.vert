#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texcoord;

layout(location = 0) out vec4 interColor;
layout(location = 1) out vec2 interTex;

void main()
{	
	gl_Position = vec4(position, 1.0);
	interTex = texcoord;
}
