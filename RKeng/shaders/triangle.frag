#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

void main() {
    // Простое диффузное освещение сверху
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.5));
    float diff    = max(dot(normalize(fragNormal), lightDir), 0.15);
    outColor      = vec4(fragColor * diff, 1.0);
}
