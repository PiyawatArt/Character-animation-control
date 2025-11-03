#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;

out VS_OUT {
    vec3 worldPos;
    vec3 worldNormal;
    vec2 uv;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    vec4 wp = model * vec4(aPos, 1.0);
    vs_out.worldPos = wp.xyz;
    // normal (model is uniform scale, so use mat3(model))
    vs_out.worldNormal = normalize(mat3(model) * aNormal);
    // uv บน plane เดิม (เราจะคูณ scale ใน fragment)
    vs_out.uv = aUV;
    gl_Position = projection * view * wp;
}
