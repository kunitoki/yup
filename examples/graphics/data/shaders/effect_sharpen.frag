#version 450

layout(set=0,binding=0) uniform texture2D u_tex;
layout(set=0,binding=1) uniform sampler u_samp;
layout(set=0,binding=2) uniform Params { float str,resX,resY,pad0,pad1,pad2,pad3,pad4; } p;
layout(location=0) out vec4 fragColor;
void main() {
    vec2 uv = gl_FragCoord.xy / vec2(p.resX, p.resY);
    vec2 t = 1.0 / vec2(p.resX, p.resY);
    vec4 c  = texture(sampler2D(u_tex,u_samp), uv);
    vec4 bl = c - 0.25 * (
        texture(sampler2D(u_tex,u_samp), uv + vec2(-1,-1)*t) +
        texture(sampler2D(u_tex,u_samp), uv + vec2( 0,-1)*t) +
        texture(sampler2D(u_tex,u_samp), uv + vec2( 1,-1)*t) +
        texture(sampler2D(u_tex,u_samp), uv + vec2(-1, 0)*t) +
        texture(sampler2D(u_tex,u_samp), uv + vec2( 1, 0)*t) +
        texture(sampler2D(u_tex,u_samp), uv + vec2(-1, 1)*t) +
        texture(sampler2D(u_tex,u_samp), uv + vec2( 0, 1)*t) +
        texture(sampler2D(u_tex,u_samp), uv + vec2( 1, 1)*t)) * 0.125;
    fragColor = mix(c, c + bl * p.str, 0.8);
}
