#version 450

layout(set=0,binding=0) uniform texture2D u_tex;
layout(set=0,binding=1) uniform sampler u_samp;
layout(set=0,binding=2) uniform Params { float bs,resX,resY,pad0,pad1,pad2,pad3,pad4; } p;
layout(location=0) out vec4 fragColor;
void main() {
    vec2 uv = gl_FragCoord.xy / vec2(p.resX, p.resY);
    float bs = max(1.0, p.bs);
    vec2 block = floor(uv * vec2(p.resX, p.resY) / bs) * bs;
    vec2 sampleUV = (block + 0.5 * bs) / vec2(p.resX, p.resY);
    fragColor = texture(sampler2D(u_tex, u_samp), sampleUV);
}
