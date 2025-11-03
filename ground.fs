#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 worldPos;
    vec3 worldNormal;
    vec2 uv;
} fs_in;

uniform vec3 uCamPos;

uniform vec3 uGridColor;     // สีเส้นกริด
uniform vec3 uBaseColor;     // สีพื้น
uniform vec3 uHorizonColor;  // สีไกล ๆ ใกล้เส้นขอบฟ้า

uniform float uGridScale;    // ความถี่กริด (มาก = ถี่)
uniform float uLineWidth;    // ความหนาเส้น (หน่วยเชิงสัดส่วน)
uniform float uFogDensity;   // ความหนาแน่นหมอก

// Anti-aliased grid line using fwidth()
float gridLineAA(vec2 uv, float width) {
    // ระยะถึงเส้นกริดแนว X/Y
    vec2 g = abs(fract(uv - 0.5) - 0.5) / fwidth(uv);
    float lineX = 1.0 - smoothstep(0.0, width, g.x);
    float lineY = 1.0 - smoothstep(0.0, width, g.y);
    return clamp(lineX + lineY, 0.0, 1.0);
}

void main() {
    // สร้างกริดจาก uv ที่ถูกขยาย
    vec2 guv = fs_in.uv * uGridScale * 200.0; // 200 = จาก model scale 200x
    float lines = gridLineAA(guv, uLineWidth * 50.0); // ปรับตาม scale

    // base shading ง่าย ๆ ด้วย lambert
    vec3 N = normalize(fs_in.worldNormal);
    vec3 L = normalize(vec3(0.3, 1.0, 0.2));
    float diff = clamp(dot(N, L), 0.0, 1.0);
    vec3 base = mix(uBaseColor * 0.9, uBaseColor, diff);

    // ผสมเส้นกริด
    vec3 color = mix(base, uGridColor, lines);

    // fog (exponential) เข้าหาขอบฟ้า
    float dist = distance(uCamPos, fs_in.worldPos);
    float fog = 1.0 - exp(-uFogDensity * uFogDensity * dist * dist);
    color = mix(color, uHorizonColor, clamp(fog, 0.0, 1.0));

    FragColor = vec4(color, 1.0);
}
