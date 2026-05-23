#version 450

// ── Per-vertex (binding 0) ────────────────────────────────────────────────
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;

// ── Per-instance (binding 1) ──────────────────────────────────────────────
// mat4 занимает locations 3..6 (4 vec4)
layout(location = 3) in vec4 instanceModel0;
layout(location = 4) in vec4 instanceModel1;
layout(location = 5) in vec4 instanceModel2;
layout(location = 6) in vec4 instanceModel3;
layout(location = 7) in vec3 instanceColor;
layout(location = 8) in float instanceWireframe; // 0 = solid, 1 = wireframe

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
    mat4 instanceM = mat4(instanceModel0, instanceModel1, instanceModel2, instanceModel3);

    // Если инстанс-матрица нулевая (не instanced draw) — используем pc.model как раньше
    float isInstanced = instanceM[3][3]; // 1.0 для инстансов, 0.0 если пусто
    mat4 finalModel = (isInstanced > 0.5)
        ? ubo.model * instanceM
        : ubo.model * pc.model;

    gl_Position = ubo.proj * ubo.view * finalModel * vec4(inPos, 1.0);

    // Wireframe кубы берут цвет из instanceColor (зелёный хитбокс)
    // Solid кубы тоже берут из instanceColor (индивидуальный цвет куба)
    fragColor  = (isInstanced > 0.5) ? instanceColor : inColor;
    fragNormal = mat3(transpose(inverse(finalModel))) * inNormal;
}
