#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;

layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 model;
} pc;

void main() {
    // pc.model == identity для вокселей стен,
    // pc.model == transform машины для car draw call
    mat4 finalModel = ubo.model * pc.model;
    gl_Position = ubo.proj * ubo.view * finalModel * vec4(inPos, 1.0);
    fragColor   = inColor;
    fragNormal  = mat3(transpose(inverse(finalModel))) * inNormal;
}
