#version 450

layout(set=0,binding=0) uniform texture2D u_tex;
layout(set=0,binding=1) uniform sampler u_samp;
layout(set=0,binding=2) uniform Params { float amp,freq,time,resX,resY,pad0,pad1,pad2; } p;
layout(location=0) out vec4 fragColor;
void main() {
    vec2 uv = gl_FragCoord.xy / vec2(p.resX, p.resY);
    float aspect = p.resX / p.resY;
    vec2 center = uv - 0.5;
    float dist = length(center * vec2(aspect, 1.0));
    float offset = sin(dist * p.freq - p.time) * p.amp * 0.003;
    vec2 sampleUV = uv + normalize(center + 0.001) * offset;
    fragColor = texture(sampler2D(u_tex, u_samp), sampleUV);
}
