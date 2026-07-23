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

inline constexpr Shader atomic_draw_image_rect_webgpu_fixedcolor_frag = {
    .source = R"WGSL(
struct Be{X1_:array<vec2<u32>>,}struct d0qd{X1_:array<u32>,}struct Ce{X1_:array<vec4<f32>>,}struct NB{Ub:f32,dd:f32,Xe:f32,Ye:f32,q5_:u32,ug:u32,Je:u32,Ke:u32,U7_:vec4<i32>,rg:vec2<f32>,ed:vec2<f32>,W1_:u32,vg:f32,Y5_:u32,P2_:f32,fd:f32,Ee:u32,y3_:f32,z3_:f32,gd:f32,og:u32,}struct p4qd{X1_:array<u32>,}struct LC{r9_:vec4<f32>,c2_:vec2<f32>,x4_:f32,ki:f32,k2_:vec4<f32>,D2_:vec2<f32>,V0_:u32,n2_:u32,Z6_:u32,}@id(7) override Tg:bool=true;@id(4) override Qg:bool=true;@id(0) override Mg:bool=true;@id(1) override Ng:bool=true;@group(0)@binding(4) var<storage>TC:Be;@group(2)@binding(1) var<storage,read_write>d0_:d0qd;@group(0)@binding(5) var<storage>PB:Ce;var<private>_1:vec4<f32>;@group(0)@binding(9) var DD:texture_2d<f32>;@group(3)@binding(9) var Bb:sampler;@group(0)@binding(0) var<uniform>k:NB;@group(1)@binding(12) var AC:texture_2d<f32>;@group(1)@binding(14) var R5_:sampler;var<private>U0_1:vec2<f32>;var<private>S4_1:f32;var<private>N0_1:vec4<f32>;@group(2)@binding(3) var<storage,read_write>p4_:p4qd;@group(0)@binding(2) var<uniform>A0_:LC;var<private>l1_:vec4<f32>;@group(3)@binding(10) var T9_:sampler;@group(0)@binding(10) var QC:texture_2d<f32>;fn _an(){var _D:f32;var _s:bool;var _w:f32;var _n:f32;var _u:f32;var _F:f32;var _q:f32;var _I:bool;var _C:f32;var _m:u32;var _v:f32;var _B:u32;var _G:vec4<f32>;var _i:bool;var _r:u32;var _y:f32;var _l:f32;var _o:vec3<f32>;let _h=_1;let _t=_h.xy;let _b=bitcast<vec2<u32>>(vec2<i32>(floor(_t)));let _8=k.q5_;let _c=bitcast<i32>((((((_b.y>>bitcast<u32>(5u))*(((_8+31u)&4294967264u)<<bitcast<u32>(5u)))+((_b.x>>bitcast<u32>(5u))<<bitcast<u32>(10u)))+(((_b.x&28u)<<bitcast<u32>(5u))+((_b.y&28u)<<bitcast<u32>(2i))))+(((_b.y&3u)<<bitcast<u32>(2i))+(_b.x&3u))));let _7=U0_1;let _ag=textureSample(AC,R5_,_7);let _ar=S4_1;let _Q=min(_ar,1f);_D=_Q;if Ng{let _O=N0_1;let _6=min(_O.xy,_O.zw);_D=clamp(min(_6.x,_6.y),0f,_Q);}let _Z=_D;let _W=p4_.X1_[_c];let _E=(_W>>bitcast<u32>(17u));let _S=((f32((_W&131071u))*0.00048828125f)+-32f);let _a=TC.X1_[_E];_n=_S;if ((_a.x&768u)!=0u){let _J=abs(_S);_s=Qg;if Qg{_s=((_a.x&512u)!=0u);}let _af=_s;_w=_J;if _af{_w=(1f-abs(((fract((_J*0.5f))*2f)+-1f)));}let _al=_w;_n=_al;}let _ai=_n;let _x=clamp(_ai,0f,1f);_q=_x;if Mg{let _4=(_a.x>>bitcast<u32>(16u));_F=_x;if (_4!=0u){let _L=d0_.X1_[_c];if (_4==(_L>>bitcast<u32>(16i))){_u=min(_x,unpack2x16float(_L).x);}else{_u=0f;}let _ab=_u;_F=_ab;}let _ad=_F;_q=_ad;}let _P=_q;_I=Ng;if Ng{_I=((_a.x&1024u)!=0u);}let _aj=_I;_C=_P;if _aj{let _V=(_E*4u);let _g=PB.X1_[(_V+2u)];let _U=PB.X1_[(_V+3u)];let _Y=_U.zw;let _T=((abs(((mat2x2<f32>(vec2<f32>(_g.x,_g.y),vec2<f32>(_g.z,_g.w))*_t)+_U.xy))*_Y)-_Y);_C=min(_P,clamp((min(_T.x,_T.y)+0.5f),0f,1f));}let _5=_C;let _j=(_a.x&15u);if (_j<=1u){let _3=(Mg&&(_j==0u));_m=0u;if _3{_m=(_a.y|pack2x16float(vec2<f32>(_5,0f)));}let _ae=_m;_B=_ae;_G=select(unpack4x8unorm(_a.y),vec4<f32>(0f,0f,0f,0f),vec4(_3));}else{let _0=(_E*4u);let _d=PB.X1_[_0];let _A=PB.X1_[(_0+1u)];let _2=((mat2x2<f32>(vec2<f32>(_d.x,_d.y),vec2<f32>(_d.z,_d.w))*_t)+_A.xy);if (_j==2u){_v=_2.x;}else{_v=length(_2);}let _ac=_v;let _aa=textureSampleLevel(DD,Bb,vec2<f32>(((clamp(_ac,0f,1f)*_A.z)+_A.w),bitcast<f32>(_a.y)),0f);_B=0u;_G=_aa;}let _e=_B;let _M=_G;let _K=(_M.w*_5);let _p=(_M.xyz*_K);_i=Mg;if Mg{let _at=A0_.V0_;_i=(_at!=0u);}let _9=_i;_l=_Z;if _9{if (_e!=0u){_r=_e;}else{let _aq=d0_.X1_[_c];_r=_aq;}let _R=_r;let _as=A0_.V0_;if (_as==(_R>>bitcast<u32>(16i))){_y=min(_Z,unpack2x16float(_R).x);}else{_y=0f;}let _ak=_y;_l=_ak;}let _ah=_l;let _ap=A0_.x4_;let _X=(_ag*(_ah*_ap));let _f=((vec4<f32>(_p.x,_p.y,_p.z,_K)*(1f-_X.w))+_X);let _N=_f.xyz;let _ao=k.y3_;let _au=k.z3_;if Tg{_o=(vec3(((fract((52.982918f*fract(((0.06711056f*_h.x)+(0.00583715f*_h.y)))))*_ao)+_au))+_N);}else{_o=_N;}let _k=_o;let _z=vec4<f32>(_k.x,_f.y,_f.z,_f.w);let _H=vec4<f32>(_z.x,_k.y,_z.z,_z.w);l1_=vec4<f32>(_H.x,_H.y,_k.z,_H.w);if (_e!=0u){d0_.X1_[_c]=_e;}p4_.X1_[_c]=65536u;return;}@fragment fn main(@builtin(position) _am:vec4<f32>,@location(0) U0_:vec2<f32>,@location(1) S4_:f32,@location(2) N0_:vec4<f32>)->@location(0) vec4<f32>{_1=_am;U0_1=U0_;S4_1=S4_;N0_1=N0_;_an();let _e9=l1_;return _e9;})WGSL",
    .usedOverrides = {{true, true, false, false, true, false, false, true, false, false, false, false, false, false}},
    .label = "atomic_draw_image_rect.webgpu_fixedcolor_frag",
};
} // namespace wgsl
