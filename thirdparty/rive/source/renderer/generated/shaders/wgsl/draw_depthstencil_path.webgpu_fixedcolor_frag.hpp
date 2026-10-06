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

inline constexpr Shader draw_depthstencil_path_webgpu_fixedcolor_frag = {
    .source = R"WGSL(
struct UB{Qc:f32,Td:f32,bg:f32,cg:f32,A6_:u32,X9_:u32,Nf:u32,Of:u32,j8_:vec4<i32>,Lh:vec2<f32>,Ud:vec2<f32>,j2_:u32,Ph:f32,T4_:u32,a3_:f32,Vd:f32,Hf:u32,L3_:f32,M3_:f32,Wd:f32,Ih:u32,W9_:u32,wc:f32,xc:f32,}@id(7) override ri:bool=true;@id(2) override mi:bool=true;@id(8) override si:bool=true;@group(0)@binding(0) var<uniform>j:UB;@group(0)@binding(8) var ED:texture_2d<f32>;@group(3)@binding(8) var ha:sampler;@group(1)@binding(11) var CC:texture_2d<f32>;@group(1)@binding(13) var r5_:sampler;var<private>r1_1:vec3<f32>;var<private>Q0_1:f32;var<private>a1_1:vec4<f32>;var<private>_v:vec4<f32>;var<private>Sh:vec4<f32>;@group(3)@binding(9) var wa:sampler;@group(0)@binding(9) var YC:texture_2d<f32>;var<private>l1_1:vec2<f32>;fn _G(){var _h:f32;var _g:f32;var _q:vec4<f32>;var _i:vec4<f32>;var _n:bool;var _j:bool;var _s:f32;var _t:vec4<f32>;var _l:vec4<f32>;var _r:vec4<f32>;var _p:vec3<f32>;let _I=Q0_1;let _c=r1_1;let _a=a1_1;switch bitcast<i32>(0u){default:{let _y=(mi&&(u32(_I)!=0u));if (_a.w>=0f){_i=_a;}else{let _z=-(_a.w);let _L=j.wc;let _H=j.xc;if (_a.z>0f){_h=_a.x;}else{_h=length(_a.xy);}let _D=_h;let _w=clamp(_D,0f,1f);let _B=abs(_a.z);if (_B>1f){_g=((0.9980469f*_w)+0.0009765625f);}else{_g=((0.001953125f*_w)+_B);}let _M=_g;let _e=textureSampleLevel(ED,ha,vec2<f32>(_M,((floor(_z)*_L)+_H)),0f);_q=_e;if!(_y){let _k=(_e.xyz*_e.w);_q=vec4<f32>(_k.x,_k.y,_k.z,(_e.w*(fract(_z)*1.0039216f)));}let _Q=_q;_i=_Q;}let _C=_i;_n=si;if si{_n=(_c.z<0f);}let _N=_n;if _N{let _R=textureSampleLevel(CC,r5_,_c.xy,0f);_r=_R;break;}_j=si;if si{_j=(_c.z>0f);}let _E=_j;_l=_C;if _E{let _d=textureSampleLevel(CC,r5_,_c.xy,(_c.z-1f));_t=_d;if _y{if (_d.w!=0f){_s=(1f/_d.w);}else{_s=0f;}let _U=_s;let _o=(_d.xyz*_U);_t=vec4<f32>(_o.x,_o.y,_o.z,_d.w);}let _S=_t;_l=(_C*_S);}let _P=_l;_r=_P;break;}}let _J=_r;let _b=(_J*1f);let _A=_b.xyz;let _x=_v;let _T=j.L3_;let _F=j.M3_;if (ri&&(_b.w!=0f)){_p=(vec3(((fract((52.982918f*fract(((0.06711056f*_x.x)+(0.00583715f*_x.y)))))*_T)+_F))+_A);}else{_p=_A;}let _u=_p;let _f=vec4<f32>(_u.x,_b.y,_b.z,_b.w);let _m=vec4<f32>(_f.x,_u.y,_f.z,_f.w);Sh=vec4<f32>(_m.x,_m.y,_u.z,_m.w);return;}@fragment fn main(@location(9) r1_:vec3<f32>,@location(6)@interpolate(flat,either) Q0_:f32,@location(0) a1_:vec4<f32>,@builtin(position) _O:vec4<f32>,@location(4)@interpolate(flat,either) l1_:vec2<f32>)->@location(0) vec4<f32>{r1_1=r1_;Q0_1=Q0_;a1_1=a1_;_v=_O;l1_1=l1_;_G();let _K=Sh;return _K;})WGSL",
    .usedOverrides = {{false, false, true, false, false, false, false, true, true, false, false, false, false, false, false}},
    .label = "draw_depthstencil_path.webgpu_fixedcolor_frag",
};
} // namespace wgsl
