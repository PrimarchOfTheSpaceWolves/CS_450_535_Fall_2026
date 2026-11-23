#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec4 color;
layout(location = 2) in vec3 normal;

layout(location = 0) out vec4 interColor;
layout(location = 1) out vec3 interNormal;

layout(push_constant) uniform PushConstants {
    mat4 modelMat;
} pc;

layout(set = 0, binding = 0) uniform UBOVertex {
    mat4 viewMat;
    mat4 projMat;
} ubo;

void main() {
    //gl_Position = vec4(pos, 1.0f);

    vec4 pos = vec4(pos, 1.0f);
    pos = ubo.projMat * ubo.viewMat * pc.modelMat * pos;
    gl_Position = pos;

    interColor = color;

    mat3 normMat = transpose(inverse(mat3(pc.modelMat)));
    interNormal = normMat*normal;
}
