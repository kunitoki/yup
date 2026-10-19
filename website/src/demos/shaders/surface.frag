#version 450
#extension GL_GOOGLE_include_directive : require

// Flat, the panel texture is drawn exactly as the Component painted it; lifted, it is lit like
// a glossy card. Faces seen from behind show a plain dark blue back instead of the panel.
layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec3 v_world;
layout(location = 2) in vec3 v_normal;
layout(location = 3) in vec3 v_eye;
layout(location = 4) in float v_lift;

layout(set = 0, binding = 1) uniform texture2D u_panel;
layout(set = 0, binding = 2) uniform sampler u_panelSampler;

layout(location = 0) out vec4 fragColor;

#include "light.glsl"

void main()
{
    vec4 panel = texture(sampler2D(u_panel, u_panelSampler), v_uv);
    vec3 toEye = normalize(v_eye - v_world);
    vec3 normal = normalize(v_normal);

    if (dot(normal, toEye) < 0.0)
    {
        normal = -normal;
        panel = vec4(0.043, 0.133, 0.325, 1.0);
    }

    vec3 albedo = panel.a > 0.0 ? panel.rgb / panel.a : vec3(0.0);
    vec3 lit = shade(albedo, normal, toEye, 0.55) * panel.a;

    fragColor = vec4(mix(panel.rgb, lit, v_lift), panel.a);
}
