#version 450

layout(set=0,binding=0) uniform texture2D u_tex;
layout(set=0,binding=1) uniform sampler u_samp;
layout(set=0,binding=2) uniform Params { float intensity,resX,resY,pad0,pad1,pad2,pad3,pad4; } p;
layout(location=0) out vec4 fragColor;
void main() {
    vec2 uv = gl_FragCoord.xy / vec2(p.resX, p.resY);
    vec4 col = texture(sampler2D(u_tex, u_samp), uv);
    // Scanlines
    float scanline = sin(uv.y * p.resY * 1.2) * 0.5 + 0.5;
    col.rgb *= 1.0 - (1.0 - scanline) * p.intensity * 0.6;
    // Vignette
    vec2 v = uv - 0.5;
    col.rgb *= 1.0 - dot(v, v) * p.intensity * 0.8;
    // Slight green tint
    col.rgb *= vec3(0.95, 1.05, 0.9);
    fragColor = col;
}
