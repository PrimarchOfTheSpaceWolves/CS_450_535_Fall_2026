#version 450
 
layout(location = 0) out vec4 out_color;

layout(location = 0) in vec4 interColor;
layout(location = 1) in vec2 interTex;

layout(set = 0, binding = 0) uniform sampler2D screenTexture;

void main()
{	
	out_color = texture(screenTexture, interTex);
}
