#version 450

layout(set=0,binding=0) uniform texture2D u_tex;
layout(set=0,binding=1) uniform sampler u_samp;
layout(set=0,binding=2) uniform Params { float s,r,rx,ry,dx,dy,pad0,pad1; } p;
layout(location=0) out vec4 fragColor;
void main() {
    vec2 uv = gl_FragCoord.xy / vec2(p.rx, p.ry);
    if (p.s <= 0.0001) { fragColor = texture(sampler2D(u_tex,u_samp), uv); return; }
    int   r = int(clamp(p.r, 1.0, 128.0));
    vec2  step = vec2(p.dx, p.dy) / vec2(p.rx, p.ry);
    float inv2s2 = 0.5 / (p.s * p.s);
    vec4  sum = texture(sampler2D(u_tex,u_samp), uv);
    float wsum = 1.0;
    for (int i = 1; i <= r; ++i) {
        float w = exp(-float(i*i) * inv2s2);
        vec2  off = step * float(i);
        sum += texture(sampler2D(u_tex,u_samp), uv + off) * w;
        sum += texture(sampler2D(u_tex,u_samp), uv - off) * w;
        wsum += 2.0 * w;
    }
    fragColor = sum / wsum;
}
