#pragma once

#include <array>

#include "shaders/constants.glsl"

namespace rive::gpu::wgsl
{
#ifndef RIVE_WGSL_SHADER_DEFINED
#define RIVE_WGSL_SHADER_DEFINED
struct Shader
{
    const char* source;
    std::array<bool, SPECIALIZATION_COUNT> usedOverrides;
    const char* label;
};
#endif

inline constexpr Shader draw_msaa_atlas_blit_webgpu_fixedcolor_frag = {
    .source = R"WGSL(
struct NB{Ub:f32,dd:f32,Xe:f32,Ye:f32,q5_:u32,ug:u32,Je:u32,Ke:u32,U7_:vec4<i32>,rg:vec2<f32>,ed:vec2<f32>,W1_:u32,vg:f32,Y5_:u32,P2_:f32,fd:f32,Ee:u32,y3_:f32,z3_:f32,gd:f32,og:u32,}@id(7) override Tg:bool=true;@id(2) override Og:bool=true;@group(0)@binding(9) var DD:texture_2d<f32>;@group(3)@binding(9) var Bb:sampler;@group(1)@binding(12) var AC:texture_2d<f32>;@group(1)@binding(14) var R5_:sampler;@group(0)@binding(11) var UC:texture_2d<f32>;@group(3)@binding(11) var I9_:sampler;var<private>C2_1:vec2<f32>;var<private>i1_1:vec4<f32>;var<private>_A:vec4<f32>;@group(0)@binding(0) var<uniform>k:NB;var<private>yg:vec4<f32>;@group(3)@binding(10) var T9_:sampler;@group(0)@binding(10) var QC:texture_2d<f32>;var<private>I3_1:f32;var<private>Z1_1:f32;fn _M(){var _o:vec4<f32>;var _s:f32;var _n:f32;var _q:vec4<f32>;var _f:f32;var _k:vec4<f32>;var _r:vec4<f32>;var _j:vec4<f32>;var _l:vec3<f32>;let _L=C2_1;let _N=textureSampleLevel(UC,I9_,_L,0f);let _c=clamp(_N.x,0f,1f);let _a=i1_1;if (_a.w>=0f){if Og{_o=vec4<f32>(_a.x,_a.y,_a.z,(_a.w*_c));}else{_o=(_a*_c);}let _I=_o;_j=_I;}else{if (_a.w>-1f){if (_a.z>0f){_s=_a.x;}else{_s=length(_a.xy);}let _H=_s;let _x=clamp(_H,0f,1f);let _u=abs(_a.z);if (_u>1f){_n=((0.9980469f*_x)+0.0009765625f);}else{_n=((0.001953125f*_x)+_u);}let _B=_n;let _d=textureSampleLevel(DD,Bb,vec2<f32>(_B,-(_a.w)),0f);let _p=(_d.w*_c);let _v=vec4<f32>(_d.x,_d.y,_d.z,_p);if Og{_q=_v;}else{let _t=(_v.xyz*_p);_q=vec4<f32>(_t.x,_t.y,_t.z,_p);}let _C=_q;_r=_C;}else{let _b=textureSampleLevel(AC,R5_,_a.xy,(-2f-_a.w));let _y=(_a.z*_c);if Og{if (_b.w!=0f){_f=(1f/_b.w);}else{_f=0f;}let _K=_f;let _g=(_b.xyz*_K);_k=vec4<f32>(_g.x,_g.y,_g.z,(_b.w*_y));}else{_k=(_b*_y);}let _D=_k;_r=_D;}let _J=_r;_j=_J;}let _e=_j;let _w=_e.xyz;let _z=_A;let _O=k.y3_;let _G=k.z3_;if Tg{_l=(vec3(((fract((52.982918f*fract(((0.06711056f*_z.x)+(0.00583715f*_z.y)))))*_O)+_G))+_w);}else{_l=_w;}let _i=_l;let _h=vec4<f32>(_i.x,_e.y,_e.z,_e.w);let _m=vec4<f32>(_h.x,_i.y,_h.z,_h.w);yg=vec4<f32>(_m.x,_m.y,_i.z,_m.w);return;}@fragment fn main(@location(1) C2_:vec2<f32>,@location(0) i1_:vec4<f32>,@builtin(position) _F:vec4<f32>,@location(4)@interpolate(flat) I3_:f32,@location(6)@interpolate(flat) Z1_:f32)->@location(0) vec4<f32>{C2_1=C2_;i1_1=i1_;_A=_F;I3_1=I3_;Z1_1=Z1_;_M();let _E=yg;return _E;})WGSL",
    .usedOverrides = {{false, false, true, false, false, false, false, true, false, false, false, false, false, false}},
    .label = "draw_msaa_atlas_blit.webgpu_fixedcolor_frag",
};
} // namespace wgsl
