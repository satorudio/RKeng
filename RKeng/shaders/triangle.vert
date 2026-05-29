#version 450

// ── Вершинный буфер: pos(3) + color(3) + normal(3) ──────────────────────────
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;

// ── UBO binding=0: матрицы камеры + свет ─────────────────────────────────────
layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    // Directional light (sun)
    vec4 sunDir;        // xyz = dir from sun (normalized), w = unused
    vec4 sunColor;      // xyz = color/intensity, w = unused
    vec4 ambientColor;  // xyz = ambient, w = unused
} ubo;

// ── Push constant: модельная матрица сцены ────────────────────────────────────
layout(push_constant) uniform PushConst {
    mat4 model;
} pc;

// ── Выходы во фрагментный ─────────────────────────────────────────────────────
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragWorldPos;

void main() {
    vec4 worldPos = pc.model * vec4(inPosition, 1.0);
    fragWorldPos  = worldPos.xyz;

    // Normal в world space (без неравномерного масштаба, поэтому просто mat3)
    fragNormal = normalize(mat3(pc.model) * inNormal);

    fragColor = inColor;

    gl_Position = ubo.proj * ubo.view * worldPos;
}
