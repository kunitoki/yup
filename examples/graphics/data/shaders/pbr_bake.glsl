// Shared prelude for the cube bakes: the Bake block, the face parameterisation
// and the analytic sky. Included by each bake fragment shader.
layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 fragColor;
layout(set = 0, binding = 0) uniform Bake {
    int   face;
    float roughness;
    float flipY;
    float envSize;
    float sunIntensity;
} u;

const float PI = 3.14159265359;

// Maps a face index plus a [0,1] texel coordinate onto the direction the cube
// map hardware associates with that texel, matching the GL/Metal/D3D layout.
vec3 faceDirection(int face, vec2 uv) {
    vec2 c = uv * 2.0 - 1.0;
    if (face == 0) return normalize(vec3( 1.0, -c.y, -c.x));
    if (face == 1) return normalize(vec3(-1.0, -c.y,  c.x));
    if (face == 2) return normalize(vec3( c.x,  1.0,  c.y));
    if (face == 3) return normalize(vec3( c.x, -1.0, -c.y));
    if (face == 4) return normalize(vec3( c.x, -c.y,  1.0));
    return normalize(vec3(-c.x, -c.y, -1.0));
}

vec2 faceUV() {
    return vec2(v_uv.x, u.flipY > 0.5 ? 1.0 - v_uv.y : v_uv.y);
}

vec3 proceduralSky(vec3 d) {
    vec3 sunDir = normalize(vec3(0.35, 0.28, -0.90));
    float up = clamp(d.y * 0.5 + 0.5, 0.0, 1.0);
    vec3 sky = mix(vec3(0.58, 0.68, 0.84), vec3(0.10, 0.22, 0.55), pow(up, 0.6));
    float horizon = 1.0 - smoothstep(-0.15, 0.45, d.y); // warm haze band near the horizon
    sky += vec3(0.24, 0.13, 0.05) * horizon * 0.55;
    vec3 ground = vec3(0.16, 0.14, 0.12);
    vec3 col = mix(ground, sky, smoothstep(-0.06, 0.10, d.y));
    float cosSun = clamp(dot(d, sunDir), 0.0, 1.0);
    // The sun disk and its halo scale with u.sunIntensity so the slider
    // re-bakes the whole environment (and its IBL derivatives) coherently.
    col += vec3(1.7, 1.45, 1.15) * u.sunIntensity * pow(cosSun, 600.0);
    col += vec3(0.16, 0.12, 0.07) * u.sunIntensity * pow(cosSun, 10.0);
    return col;
}

// Hammersley low-discrepancy sequence, used by the GGX importance sampler.
float radicalInverse(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

vec2 hammersley(uint i, uint n) {
    return vec2(float(i) / float(n), radicalInverse(i));
}

vec3 importanceSampleGGX(vec2 xi, vec3 n, float roughness) {
    float a = roughness * roughness;
    float phi = 2.0 * PI * xi.x;
    float cosTheta = sqrt((1.0 - xi.y) / (1.0 + (a * a - 1.0) * xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    vec3 h = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    vec3 up = abs(n.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, n));
    vec3 bitangent = cross(n, tangent);
    return normalize(tangent * h.x + bitangent * h.y + n * h.z);
}
