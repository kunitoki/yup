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
struct RB{Od:f32,Ze:f32,dh:f32,eh:f32,X5_:u32,Ka:u32,Og:u32,Qg:u32,Q8_:vec4<i32>,Yi:vec2<f32>,af:vec2<f32>,z2_:u32,cj:f32,B6_:u32,m3_:f32,bf:f32,Hg:u32,J3_:f32,K3_:f32,cf:f32,Vi:u32,Ja:u32,X8_:f32,Y8_:f32,}@id(8) override Jj:bool=true;@id(9) override Kj:bool=true;@id(16) override Rj:bool=false;@id(17) override Sj:bool=false;@id(2) override Dj:bool=true;@id(4) override Fj:bool=true;var<private>S0_1:f32;var<private>N0_1:vec4<f32>;@group(0)@binding(0) var<uniform>j:RB;@group(0)@binding(8) var AD:texture_2d<f32>;@group(3)@binding(8) var Z8_:sampler;var<private>O0_1:vec3<f32>;@group(1)@binding(11) var SB:texture_2d<f32>;@group(1)@binding(13) var x4_:sampler;var<private>_H:vec4<f32>;var<private>fj:vec4<f32>;@group(3)@binding(9) var kb:sampler;@group(0)@binding(9) var BD:texture_2d<f32>;fn _X(){var _p:f32;var _t:f32;var _l:vec2<f32>;var _f:bool;var _v:vec4<f32>;var _r:vec4<f32>;var _n:vec4<f32>;var _i:bool;var _u:f32;var _k:vec4<f32>;var _A:vec4<f32>;var _y:vec3<f32>;var _m:bool;var _d:f32;var _x:vec4<f32>;let _af=S0_1;let _I=(Dj&&(u32(_af)!=0u));let _B=N0_1[3u];if (_B>=0f){let _V=N0_1;_n=_V;}else{let _a=N0_1;let _Y=j.X8_;let _Q=j.Y8_;let _K=abs(_a.z);let _J=floor(_K);let _N=(((_K-_J)*8f)+0.0009765625f);let _P=((_J*_Y)+_Q);let _L=floor(_N);let _C=_a.xy;switch bitcast<i32>(0u){default:{if (Kj&&(_L==4f)){let _O=normalize(_C);let _D=acos(clamp(_O.x,-1f,1f));if (_O.y>=0f){_t=_D;}else{_t=-(_D);}let _T=_t;_l=vec2<f32>((_T*0.15915494f),_P);break;}else{if (_L==2f){_p=_a.x;}else{_p=length(_C);}let _R=_p;_l=vec2<f32>(((clamp(_R,0f,1f)*select(0.001953125f,0.9980469f,(_a.z<0f)))+_N),_P);break;}}}let _ae=_l;let _b=textureSampleLevel(AD,Z8_,_ae,0f);switch bitcast<i32>(0u){default:{if Rj{_f=true;break;}if Sj{_f=true;break;}_f=false;break;}}let _9=_f;_v=_b;if _9{_v=vec4<f32>(_b.x,_b.y,_b.z,(_b.w*((fract(_a.w)*-2f)+1.5f)));}let _g=_v;_r=_g;if!(_I){let _s=(_g.xyz*_g.w);_r=vec4<f32>(_s.x,_s.y,_s.z,(_g.w*(ceil(_a.w)*-0.003921569f)));}let _ab=_r;_n=_ab;}let _F=_n;_i=Fj;if Fj{let _7=O0_1[2u];_i=(_7>0f);}let _0=_i;_A=_F;if _0{let _1=O0_1[2u];let _4=O0_1;let _c=textureSampleLevel(SB,x4_,_4.xy,(_1-1f));_k=_c;if _I{if (_c.w!=0f){_u=(1f/_c.w);}else{_u=0f;}let _Z=_u;let _j=(_c.xyz*_Z);_k=vec4<f32>(_j.x,_j.y,_j.z,_c.w);}let _8=_k;_A=(_F*_8);}let _e=_A;let _M=_e.xyz;let _E=_H;let _3=j.J3_;let _ad=j.K3_;if (Jj&&(_e.w!=0f)){_y=(vec3(((fract((52.982918f*fract(((0.06711056f*_E.x)+(0.00583715f*_E.y)))))*_3)+_ad))+_M);}else{_y=_M;}let _w=_y;let _h=vec4<f32>(_w.x,_e.y,_e.z,_e.w);let _z=vec4<f32>(_h.x,_w.y,_h.z,_h.w);_m=Fj;if Fj{let _S=O0_1[2u];_m=(_S<0f);}let _5=_m;_x=vec4<f32>(_z.x,_z.y,_w.z,_z.w);if _5{let _ac=O0_1;let _o=textureSampleLevel(SB,x4_,_ac.xy,0f);let _aa=O0_1[2u];let _q=u32((-1f-_aa));switch bitcast<i32>(0u){default:{let _G=dot(_o.xyz,vec3<f32>(0.3f,0.59f,0.11f));if (_q==0u){_d=_o.w;break;}if (_q==1u){_d=(1f-_o.w);break;}if (_q==2u){_d=_G;break;}_d=(1f-_G);break;}}let _6=_d;_x=vec4(mix(1f,_6,_B));}let _W=_x;fj=_W;return;}@fragment fn main(@location(1)@interpolate(flat,either) S0_:f32,@location(0) N0_:vec4<f32>,@location(2) O0_:vec3<f32>,@builtin(position) _U:vec4<f32>)->@location(0) vec4<f32>{S0_1=S0_;N0_1=N0_;O0_1=O0_;_H=_U;_X();let _2=fj;return _2;})WGSL",
    .usedOverrides = {{false, false, true, false, true, false, false, false, true, true, false, false, false, false, false, false, true, true}},
    .label = "draw_depthstencil_path.webgpu_fixedcolor_frag",
};
} // namespace wgsl
