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
struct RB{Od:f32,Ze:f32,dh:f32,eh:f32,X5_:u32,Ka:u32,Og:u32,Qg:u32,Q8_:vec4<i32>,Yi:vec2<f32>,af:vec2<f32>,z2_:u32,cj:f32,B6_:u32,m3_:f32,bf:f32,Hg:u32,J3_:f32,K3_:f32,cf:f32,Vi:u32,Ja:u32,X8_:f32,Y8_:f32,}@id(8) override Jj:bool=true;@id(9) override Kj:bool=true;@id(2) override Dj:bool=true;@id(4) override Fj:bool=true;@group(0)@binding(0) var<uniform>j:RB;@group(0)@binding(8) var AD:texture_2d<f32>;@group(3)@binding(8) var Z8_:sampler;@group(1)@binding(11) var SB:texture_2d<f32>;@group(1)@binding(13) var x4_:sampler;@group(0)@binding(10) var ID:texture_2d<f32>;@group(3)@binding(10) var Za:sampler;var<private>X2_1:vec2<f32>;var<private>O0_1:vec3<f32>;var<private>S0_1:f32;var<private>N0_1:vec4<f32>;var<private>_E:vec4<f32>;var<private>fj:vec4<f32>;@group(3)@binding(9) var kb:sampler;@group(0)@binding(9) var BD:texture_2d<f32>;var<private>j4_1:f32;fn _P(){var _n:f32;var _q:f32;var _j:vec2<f32>;var _r:vec4<f32>;var _o:vec4<f32>;var _l:bool;var _f:bool;var _s:f32;var _k:vec4<f32>;var _p:vec4<f32>;var _v:vec4<f32>;var _u:vec3<f32>;let _X=X2_1;let _N=textureSampleLevel(ID,Za,_X,0f);let _L=S0_1;let _d=O0_1;let _a=N0_1;switch bitcast<i32>(0u){default:{let _x=(Dj&&(u32(_L)!=0u));if (_a.w>=0f){_o=_a;}else{let _J=j.X8_;let _W=j.Y8_;let _w=abs(_a.z);let _H=floor(_w);let _A=(((_w-_H)*8f)+0.0009765625f);let _I=((_H*_J)+_W);let _C=floor(_A);let _D=_a.xy;switch bitcast<i32>(0u){default:{if (Kj&&(_C==4f)){let _y=normalize(_D);let _F=acos(clamp(_y.x,-1f,1f));if (_y.y>=0f){_q=_F;}else{_q=-(_F);}let _K=_q;_j=vec2<f32>((_K*0.15915494f),_I);break;}else{if (_C==2f){_n=_a.x;}else{_n=length(_D);}let _Q=_n;_j=vec2<f32>(((clamp(_Q,0f,1f)*select(0.001953125f,0.9980469f,(_a.z<0f)))+_A),_I);break;}}}let _2=_j;let _e=textureSampleLevel(AD,Z8_,_2,0f);_r=_e;if!(_x){let _t=(_e.xyz*_e.w);_r=vec4<f32>(_t.x,_t.y,_t.z,(_e.w*(ceil(_a.w)*-0.003921569f)));}let _0=_r;_o=_0;}let _z=_o;_l=Fj;if Fj{_l=(_d.z<0f);}let _S=_l;if _S{let _M=textureSampleLevel(SB,x4_,_d.xy,0f);_v=_M;break;}_f=Fj;if Fj{_f=(_d.z>0f);}let _Y=_f;_p=_z;if _Y{let _c=textureSampleLevel(SB,x4_,_d.xy,(_d.z-1f));_k=_c;if _x{if (_c.w!=0f){_s=(1f/_c.w);}else{_s=0f;}let _O=_s;let _h=(_c.xyz*_O);_k=vec4<f32>(_h.x,_h.y,_h.z,_c.w);}let _R=_k;_p=(_z*_R);}let _Z=_p;_v=_Z;break;}}let _3=_v;let _b=(_3*clamp(_N.x,0f,1f));let _G=_b.xyz;let _B=_E;let _T=j.J3_;let _1=j.K3_;if (Jj&&(_b.w!=0f)){_u=(vec3(((fract((52.982918f*fract(((0.06711056f*_B.x)+(0.00583715f*_B.y)))))*_T)+_1))+_G);}else{_u=_G;}let _g=_u;let _i=vec4<f32>(_g.x,_b.y,_b.z,_b.w);let _m=vec4<f32>(_i.x,_g.y,_i.z,_i.w);fj=vec4<f32>(_m.x,_m.y,_g.z,_m.w);return;}@fragment fn main(@location(1) X2_:vec2<f32>,@location(9) O0_:vec3<f32>,@location(6)@interpolate(flat,either) S0_:f32,@location(0) N0_:vec4<f32>,@builtin(position) _U:vec4<f32>,@location(4)@interpolate(flat,either) j4_:f32)->@location(0) vec4<f32>{X2_1=X2_;O0_1=O0_;S0_1=S0_;N0_1=N0_;_E=_U;j4_1=j4_;_P();let _V=fj;return _V;})WGSL",
    .usedOverrides = {{false, false, true, false, true, false, false, false, true, true, false, false, false, false, false, false, false, false}},
    .label = "draw_depthstencil_atlas_blit.webgpu_fixedcolor_frag",
};
} // namespace wgsl
