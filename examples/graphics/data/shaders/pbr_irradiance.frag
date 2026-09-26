#version 450
#extension GL_GOOGLE_include_directive : require

#include "pbr_bake.glsl"

layout(set = 0, binding = 1) uniform textureCube u_env;
layout(set = 0, binding = 2) uniform sampler     u_samp;

void main() {
    vec3 n = faceDirection(u.face, faceUV());

    vec3 up = abs(n.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(0.0, 0.0, 1.0);
    vec3 right = normalize(cross(up, n));
    up = normalize(cross(n, right));

    vec3 irradiance = vec3(0.0);
    float samples = 0.0;

    for (float phi = 0.0; phi < 2.0 * PI; phi += 0.15) {
        for (float theta = 0.0; theta < 0.5 * PI; theta += 0.05) {
            vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            vec3 direction = tangentSample.x * right + tangentSample.y * up + tangentSample.z * n;
            irradiance += textureLod(samplerCube(u_env, u_samp), direction, 0.0).rgb * cos(theta) * sin(theta);
            samples += 1.0;
        }
    }

    fragColor = vec4(PI * irradiance / max(samples, 1.0), 1.0);
}
