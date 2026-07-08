#version 450

layout(location = 0) in vec3 v_color;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_uv;
layout(set = 0, binding = 1) uniform texture2D u_tex;
layout(set = 0, binding = 2) uniform sampler   u_samp;
layout(location = 0) out vec4 fragColor;

void main() {
    vec3  light = normalize(vec3(0.503, 0.671, -0.419));
    float ndotl = clamp(dot(normalize(v_normal), light), 0.0, 1.0);
    vec4  tex   = texture(sampler2D(u_tex, u_samp), vec2(1.0 - v_uv.x, v_uv.y));
    vec3  base  = mix(v_color, tex.rgb, tex.a);
    fragColor   = vec4(base * (0.35 + 0.65 * ndotl), 1.0);
}