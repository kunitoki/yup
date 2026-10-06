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

inline constexpr Shader draw_depthstencil_atlas_blit_webgpu_fixedcolor_frag = {
    .source = R"WGSL(
struct UB{Qc:f32,Td:f32,bg:f32,cg:f32,A6_:u32,X9_:u32,Nf:u32,Of:u32,j8_:vec4<i32>,Lh:vec2<f32>,Ud:vec2<f32>,j2_:u32,Ph:f32,T4_:u32,a3_:f32,Vd:f32,Hf:u32,L3_:f32,M3_:f32,Wd:f32,Ih:u32,W9_:u32,wc:f32,xc:f32,}@id(7) override ri:bool=true;@id(2) override mi:bool=true;@id(8) override si:bool=true;@group(0)@binding(0) var<uniform>j:UB;@group(0)@binding(8) var ED:texture_2d<f32>;@group(3)@binding(8) var ha:sampler;@group(1)@binding(11) var CC:texture_2d<f32>;@group(1)@binding(13) var r5_:sampler;@group(0)@binding(10) var FD:texture_2d<f32>;@group(3)@binding(10) var ma:sampler;var<private>J2_1:vec2<f32>;var<private>r1_1:vec3<f32>;var<private>Q0_1:f32;var<private>a1_1:vec4<f32>;var<private>_B:vec4<f32>;var<private>Sh:vec4<f32>;@group(3)@binding(9) var wa:sampler;@group(0)@binding(9) var YC:texture_2d<f32>;var<private>Y3_1:f32;fn _W(){var _s:f32;var _k:f32;var _o:vec4<f32>;var _t:vec4<f32>;var _j:bool;var _p:bool;var _g:f32;var _n:vec4<f32>;var _f:vec4<f32>;var _u:vec4<f32>;var _l:vec3<f32>;let _R=J2_1;let _Q=textureSampleLevel(FD,ma,_R,0f);let _G=Q0_1;let _b=r1_1;let _a=a1_1;switch bitcast<i32>(0u){default:{let _z=(mi&&(u32(_G)!=0u));if (_a.w>=0f){_t=_a;}else{let _v=-(_a.w);let _U=j.wc;let _F=j.xc;if (_a.z>0f){_s=_a.x;}else{_s=length(_a.xy);}let _S=_s;let _x=clamp(_S,0f,1f);let _w=abs(_a.z);if (_w>1f){_k=((0.9980469f*_x)+0.0009765625f);}else{_k=((0.001953125f*_x)+_w);}let _O=_k;let _e=textureSampleLevel(ED,ha,vec2<f32>(_O,((floor(_v)*_U)+_F)),0f);_o=_e;if!(_z){let _m=(_e.xyz*_e.w);_o=vec4<f32>(_m.x,_m.y,_m.z,(_e.w*(fract(_v)*1.0039216f)));}let _D=_o;_t=_D;}let _A=_t;_j=si;if si{_j=(_b.z<0f);}let _I=_j;if _I{let _J=textureSampleLevel(CC,r5_,_b.xy,0f);_u=_J;break;}_p=si;if si{_p=(_b.z>0f);}let _M=_p;_f=_A;if _M{let _c=textureSampleLevel(CC,r5_,_b.xy,(_b.z-1f));_n=_c;if _z{if (_c.w!=0f){_g=(1f/_c.w);}else{_g=0f;}let _H=_g;let _i=(_c.xyz*_H);_n=vec4<f32>(_i.x,_i.y,_i.z,_c.w);}let _K=_n;_f=(_A*_K);}let _L=_f;_u=_L;break;}}let _T=_u;let _d=(_T*clamp(_Q.x,0f,1f));let _y=_d.xyz;let _C=_B;let _P=j.L3_;let _E=j.M3_;if (ri&&(_d.w!=0f)){_l=(vec3(((fract((52.982918f*fract(((0.06711056f*_C.x)+(0.00583715f*_C.y)))))*_P)+_E))+_y);}else{_l=_y;}let _q=_l;let _r=vec4<f32>(_q.x,_d.y,_d.z,_d.w);let _h=vec4<f32>(_r.x,_q.y,_r.z,_r.w);Sh=vec4<f32>(_h.x,_h.y,_q.z,_h.w);return;}@fragment fn main(@location(1) J2_:vec2<f32>,@location(9) r1_:vec3<f32>,@location(6)@interpolate(flat,either) Q0_:f32,@location(0) a1_:vec4<f32>,@builtin(position) _N:vec4<f32>,@location(4)@interpolate(flat,either) Y3_:f32)->@location(0) vec4<f32>{J2_1=J2_;r1_1=r1_;Q0_1=Q0_;a1_1=a1_;_B=_N;Y3_1=Y3_;_W();let _V=Sh;return _V;})WGSL",
    .usedOverrides = {{false, false, true, false, false, false, false, true, true, false, false, false, false, false, false}},
    .label = "draw_depthstencil_atlas_blit.webgpu_fixedcolor_frag",
};
} // namespace wgsl
