#version 450

layout(set=0,binding=0) uniform texture2D u_tex;
layout(set=0,binding=1) uniform sampler u_samp;
layout(set=0,binding=2) uniform Params { float thr,resX,resY,pad0,pad1,pad2,pad3,pad4; } p;
layout(location=0) out vec4 fragColor;
void main() {
    vec2 uv = gl_FragCoord.xy / vec2(p.resX, p.resY);
    vec2 t = 1.0 / vec2(p.resX, p.resY);
    vec4 tl = texture(sampler2D(u_tex,u_samp), uv + vec2(-1,-1)*t);
    vec4 top = texture(sampler2D(u_tex,u_samp), uv + vec2(0,-1)*t);
    vec4 tr = texture(sampler2D(u_tex,u_samp), uv + vec2(1,-1)*t);
    vec4 lf = texture(sampler2D(u_tex,u_samp), uv + vec2(-1,0)*t);
    vec4 rt = texture(sampler2D(u_tex,u_samp), uv + vec2(1,0)*t);
    vec4 bl = texture(sampler2D(u_tex,u_samp), uv + vec2(-1,1)*t);
    vec4 bm = texture(sampler2D(u_tex,u_samp), uv + vec2(0,1)*t);
    vec4 br = texture(sampler2D(u_tex,u_samp), uv + vec2(1,1)*t);
    vec3 h = -tl.rgb - 2.0*top.rgb - tr.rgb + bl.rgb + 2.0*bm.rgb + br.rgb;
    vec3 v = -tl.rgb - 2.0*lf.rgb + tr.rgb - bl.rgb + 2.0*rt.rgb + br.rgb;
    float edge = length(h) + length(v) > p.thr ? 1.0 : 0.0;
    fragColor = vec4(vec3(edge), 1.0);
}
