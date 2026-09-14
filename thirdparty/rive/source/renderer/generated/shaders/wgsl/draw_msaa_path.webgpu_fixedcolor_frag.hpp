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

inline constexpr Shader draw_msaa_path_webgpu_fixedcolor_frag = {
    .source = R"WGSL(
struct NB{Ub:f32,dd:f32,Xe:f32,Ye:f32,q5_:u32,ug:u32,Je:u32,Ke:u32,U7_:vec4<i32>,rg:vec2<f32>,ed:vec2<f32>,W1_:u32,vg:f32,Y5_:u32,P2_:f32,fd:f32,Ee:u32,y3_:f32,z3_:f32,gd:f32,og:u32,}@id(7) override Tg:bool=true;@id(2) override Og:bool=true;@group(0)@binding(9) var DD:texture_2d<f32>;@group(3)@binding(9) var Bb:sampler;@group(1)@binding(12) var AC:texture_2d<f32>;@group(1)@binding(14) var R5_:sampler;var<private>i1_1:vec4<f32>;var<private>_t:vec4<f32>;@group(0)@binding(0) var<uniform>k:NB;var<private>yg:vec4<f32>;@group(3)@binding(10) var T9_:sampler;@group(0)@binding(10) var QC:texture_2d<f32>;var<private>S1_1:vec2<f32>;var<private>Z1_1:f32;fn _F(){var _m:vec4<f32>;var _q:f32;var _o:f32;var _h:vec4<f32>;var _j:f32;var _f:vec4<f32>;var _r:vec4<f32>;var _n:vec4<f32>;var _g:vec3<f32>;let _a=i1_1;if (_a.w>=0f){if Og{_m=vec4<f32>(_a.x,_a.y,_a.z,_a.w);}else{_m=(_a*1f);}let _C=_m;_n=_C;}else{if (_a.w>-1f){if (_a.z>0f){_q=_a.x;}else{_q=length(_a.xy);}let _I=_q;let _s=clamp(_I,0f,1f);let _v=abs(_a.z);if (_v>1f){_o=((0.9980469f*_s)+0.0009765625f);}else{_o=((0.001953125f*_s)+_v);}let _y=_o;let _b=textureSampleLevel(DD,Bb,vec2<f32>(_y,-(_a.w)),0f);let _w=vec4<f32>(_b.x,_b.y,_b.z,_b.w);if Og{_h=_w;}else{let _i=(_w.xyz*_b.w);_h=vec4<f32>(_i.x,_i.y,_i.z,_b.w);}let _E=_h;_r=_E;}else{let _c=textureSampleLevel(AC,R5_,_a.xy,(-2f-_a.w));if Og{if (_c.w!=0f){_j=(1f/_c.w);}else{_j=0f;}let _H=_j;let _k=(_c.xyz*_H);_f=vec4<f32>(_k.x,_k.y,_k.z,(_c.w*_a.z));}else{_f=(_c*_a.z);}let _D=_f;_r=_D;}let _B=_r;_n=_B;}let _d=_n;let _u=_d.xyz;let _x=_t;let _J=k.y3_;let _A=k.z3_;if Tg{_g=(vec3(((fract((52.982918f*fract(((0.06711056f*_x.x)+(0.00583715f*_x.y)))))*_J)+_A))+_u);}else{_g=_u;}let _p=_g;let _l=vec4<f32>(_p.x,_d.y,_d.z,_d.w);let _e=vec4<f32>(_l.x,_p.y,_l.z,_l.w);yg=vec4<f32>(_e.x,_e.y,_p.z,_e.w);return;}@fragment fn main(@location(0) i1_:vec4<f32>,@builtin(position) _z:vec4<f32>,@location(4)@interpolate(flat) S1_:vec2<f32>,@location(6)@interpolate(flat) Z1_:f32)->@location(0) vec4<f32>{i1_1=i1_;_t=_z;S1_1=S1_;Z1_1=Z1_;_F();let _G=yg;return _G;})WGSL",
    .usedOverrides = {{false, false, true, false, false, false, false, true, false, false, false, false, false, false}},
    .label = "draw_msaa_path.webgpu_fixedcolor_frag",
};
} // namespace wgsl
