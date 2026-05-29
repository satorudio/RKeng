#version 450

// ── UBO binding=0 (тот же layout что в .vert) ────────────────────────────────
layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec4 sunDir;
    vec4 sunColor;
    vec4 ambientColor;
} ubo;

// ── Вход из вершинного шейдера ────────────────────────────────────────────────
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragWorldPos;

// ── Выход: итоговый цвет пикселя ─────────────────────────────────────────────
layout(location = 0) out vec4 outColor;

void main() {
    vec3 N = normalize(fragNormal);

    // Directional light (sun): sunDir указывает ОТ источника света
    vec3 L = normalize(-ubo.sunDir.xyz);   // направление к источнику

    // Diffuse (Lambertian)
    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = fragColor * ubo.sunColor.xyz * NdotL;

    // Ambient
    vec3 ambient = fragColor * ubo.ambientColor.xyz;

    // Specular (Blinn-Phong) — небольшой блик
    // Позиция камеры из view matrix (4-я колонка обратной матрицы вида)
    vec3 camPos = -mat3(ubo.view) * ubo.view[3].xyz;
    vec3 V = normalize(camPos - fragWorldPos);
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), 32.0) * 0.3;
    vec3 specular = ubo.sunColor.xyz * spec;

    vec3 result = ambient + diffuse + specular;

    // Tone mapping (Reinhard) — чтобы HDR солнце не выжигало
    result = result / (result + vec3(1.0));

    // Gamma correction (linear → sRGB)
    result = pow(result, vec3(1.0 / 2.2));

    outColor = vec4(result, 1.0);
}
