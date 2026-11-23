#version 450

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec4 interColor;
layout(location = 1) in vec3 interNormal;

void main() {
    vec3 N = normalize(interNormal);

    //out_color = vec4(abs(N), 1);
    out_color = interColor;
}