#version 450

layout(location = 0) in vec3 pos;

layout(push_constant) uniform PushConstants {
    mat4 modelMat;
} pc;

void main() {
    //gl_Position = vec4(pos, 1.0f);

    vec4 pos = vec4(pos, 1.0f);
    pos = pc.modelMat * pos;
    gl_Position = pos;
}
