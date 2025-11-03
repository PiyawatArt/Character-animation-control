#version 330 core
out vec4 FragColor;
in vec2 vUV;

uniform vec2 uResolution;
uniform float uTime;
uniform vec3 topColor;
uniform vec3 bottomColor;

void main() {
    // vertical gradient
    float t = smoothstep(0.0, 1.0, vUV.y);
    vec3 col = mix(bottomColor, topColor, t);

    // subtle vignette
    vec2 uv = vUV * 2.0 - 1.0;
    float r = length(uv);
    float vignette = smoothstep(1.2, 0.4, r); // ขอบมืดนิด ๆ
    col *= vignette;

    // shimmer นุ่ม ๆ (แทบไม่เห็น แต่ช่วยให้ฉากมีชีวิต)
    col += 0.02 * sin(uTime * 0.1 + vUV.y * 3.14159);

    FragColor = vec4(col, 1.0);
}
