#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec4 color;

layout(location = 0) out vec4 interColor;

layout(push_constant) uniform PushConstants {
    mat4 modelMat;
} pc;

void main() {
    //gl_Position = vec4(pos, 1.0f);

    vec4 pos = vec4(pos, 1.0f);
    pos = pc.modelMat * pos;
    gl_Position = pos;

    interColor = color;
}
