#version 450

layout(location = 0) in vec2 v_uv;
layout(set = 0, binding = 1) uniform texture2D u_tex;
layout(set = 0, binding = 2) uniform sampler u_samp;
layout(location = 0) out vec4 fragColor;

void main() {
    fragColor = vec4(texture(sampler2D(u_tex, u_samp), v_uv).rgb, 1.0);
}
