#version 450

layout(location = 0) in vec3 v_worldPosition;
layout(location = 3) in vec3 v_cameraPosition;

layout(set = 0, binding = 0) uniform Scene {
    vec4 camera;
    vec4 material;
    vec4 light;
} u;

layout(set = 0, binding = 1) uniform textureCube u_environment;
layout(set = 0, binding = 2) uniform sampler     u_samp;

layout(location = 0) out vec4 fragColor;

vec3 acesTonemap(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    vec3 direction = normalize(v_worldPosition - v_cameraPosition);
    vec3 color = textureLod(samplerCube(u_environment, u_samp), direction, 0.0).rgb;

    color = acesTonemap(color * u.material.x);
    fragColor = vec4(pow(color, vec3(1.0 / 2.2)), 1.0);
}
