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

inline constexpr Shader atomic_draw_image_mesh_webgpu_fixedcolor_frag = {
    .source = R"WGSL(
struct Be{X1_:array<vec2<u32>>,}struct d0qd{X1_:array<u32>,}struct Ce{X1_:array<vec4<f32>>,}struct NB{Ub:f32,dd:f32,Xe:f32,Ye:f32,q5_:u32,ug:u32,Je:u32,Ke:u32,U7_:vec4<i32>,rg:vec2<f32>,ed:vec2<f32>,W1_:u32,vg:f32,Y5_:u32,P2_:f32,fd:f32,Ee:u32,y3_:f32,z3_:f32,gd:f32,og:u32,}struct p4qd{X1_:array<u32>,}struct LC{r9_:vec4<f32>,c2_:vec2<f32>,x4_:f32,ki:f32,k2_:vec4<f32>,D2_:vec2<f32>,V0_:u32,n2_:u32,Z6_:u32,}@id(7) override Tg:bool=true;@id(4) override Qg:bool=true;@id(0) override Mg:bool=true;@id(1) override Ng:bool=true;@group(0)@binding(4) var<storage>TC:Be;@group(2)@binding(1) var<storage,read_write>d0_:d0qd;@group(0)@binding(5) var<storage>PB:Ce;var<private>_N:vec4<f32>;@group(0)@binding(9) var DD:texture_2d<f32>;@group(3)@binding(9) var Bb:sampler;@group(0)@binding(0) var<uniform>k:NB;@group(1)@binding(12) var AC:texture_2d<f32>;@group(1)@binding(14) var R5_:sampler;var<private>U0_1:vec2<f32>;var<private>N0_1:vec4<f32>;@group(2)@binding(3) var<storage,read_write>p4_:p4qd;@group(0)@binding(2) var<uniform>A0_:LC;var<private>l1_:vec4<f32>;@group(3)@binding(10) var T9_:sampler;@group(0)@binding(10) var QC:texture_2d<f32>;fn _ao(){var _m:f32;var _z:bool;var _j:f32;var _s:f32;var _k:f32;var _h:f32;var _D:f32;var _E:bool;var _p:f32;var _F:u32;var _I:f32;var _q:u32;var _C:vec4<f32>;var _H:bool;var _w:u32;var _i:f32;var _G:f32;var _x:vec3<f32>;let _t=_N;let _o=_t.xy;let _b=bitcast<vec2<u32>>(vec2<i32>(floor(_o)));let _ap=k.q5_;let _c=bitcast<i32>((((((_b.y>>bitcast<u32>(5u))*(((_ap+31u)&4294967264u)<<bitcast<u32>(5u)))+((_b.x>>bitcast<u32>(5u))<<bitcast<u32>(10u)))+(((_b.x&28u)<<bitcast<u32>(5u))+((_b.y&28u)<<bitcast<u32>(2i))))+(((_b.y&3u)<<bitcast<u32>(2i))+(_b.x&3u))));let _9=U0_1;let _ab=textureSample(AC,R5_,_9);_m=1f;if Ng{let _R=N0_1;let _O=min(_R.xy,_R.zw);_m=clamp(min(_O.x,_O.y),0f,1f);}let _Q=_m;let _T=p4_.X1_[_c];let _n=(_T>>bitcast<u32>(17u));let _P=((f32((_T&131071u))*0.00048828125f)+-32f);let _a=TC.X1_[_n];_s=_P;if ((_a.x&768u)!=0u){let _Z=abs(_P);_z=Qg;if Qg{_z=((_a.x&512u)!=0u);}let _as=_z;_j=_Z;if _as{_j=(1f-abs(((fract((_Z*0.5f))*2f)+-1f)));}let _8=_j;_s=_8;}let _am=_s;let _v=clamp(_am,0f,1f);_D=_v;if Mg{let _1=(_a.x>>bitcast<u32>(16u));_h=_v;if (_1!=0u){let _5=d0_.X1_[_c];if (_1==(_5>>bitcast<u32>(16i))){_k=min(_v,unpack2x16float(_5).x);}else{_k=0f;}let _7=_k;_h=_7;}let _ag=_h;_D=_ag;}let _X=_D;_E=Ng;if Ng{_E=((_a.x&1024u)!=0u);}let _ar=_E;_p=_X;if _ar{let _S=(_n*4u);let _e=PB.X1_[(_S+2u)];let _0=PB.X1_[(_S+3u)];let _4=_0.zw;let _2=((abs(((mat2x2<f32>(vec2<f32>(_e.x,_e.y),vec2<f32>(_e.z,_e.w))*_o)+_0.xy))*_4)-_4);_p=min(_X,clamp((min(_2.x,_2.y)+0.5f),0f,1f));}let _J=_p;let _B=(_a.x&15u);if (_B<=1u){let _L=(Mg&&(_B==0u));_F=0u;if _L{_F=(_a.y|pack2x16float(vec2<f32>(_J,0f)));}let _aq=_F;_q=_aq;_C=select(unpack4x8unorm(_a.y),vec4<f32>(0f,0f,0f,0f),vec4(_L));}else{let _U=(_n*4u);let _g=PB.X1_[_U];let _l=PB.X1_[(_U+1u)];let _Y=((mat2x2<f32>(vec2<f32>(_g.x,_g.y),vec2<f32>(_g.z,_g.w))*_o)+_l.xy);if (_B==2u){_I=_Y.x;}else{_I=length(_Y);}let _6=_I;let _ai=textureSampleLevel(DD,Bb,vec2<f32>(((clamp(_6,0f,1f)*_l.z)+_l.w),bitcast<f32>(_a.y)),0f);_q=0u;_C=_ai;}let _f=_q;let _3=_C;let _V=(_3.w*_J);let _r=(_3.xyz*_V);_H=Mg;if Mg{let _aj=A0_.V0_;_H=(_aj!=0u);}let _af=_H;_G=_Q;if _af{if (_f!=0u){_w=_f;}else{let _ak=d0_.X1_[_c];_w=_ak;}let _K=_w;let _ac=A0_.V0_;if (_ac==(_K>>bitcast<u32>(16i))){_i=min(_Q,unpack2x16float(_K).x);}else{_i=0f;}let _ah=_i;_G=_ah;}let _al=_G;let _aa=A0_.x4_;let _W=(_ab*(_al*_aa));let _d=((vec4<f32>(_r.x,_r.y,_r.z,_V)*(1f-_W.w))+_W);let _M=_d.xyz;let _ae=k.y3_;let _an=k.z3_;if Tg{_x=(vec3(((fract((52.982918f*fract(((0.06711056f*_t.x)+(0.00583715f*_t.y)))))*_ae)+_an))+_M);}else{_x=_M;}let _u=_x;let _A=vec4<f32>(_u.x,_d.y,_d.z,_d.w);let _y=vec4<f32>(_A.x,_u.y,_A.z,_A.w);l1_=vec4<f32>(_y.x,_y.y,_u.z,_y.w);if (_f!=0u){d0_.X1_[_c]=_f;}p4_.X1_[_c]=65536u;return;}@fragment fn main(@builtin(position) _ad:vec4<f32>,@location(0) U0_:vec2<f32>,@location(1) N0_:vec4<f32>)->@location(0) vec4<f32>{_N=_ad;U0_1=U0_;N0_1=N0_;_ao();let _e7=l1_;return _e7;})WGSL",
    .usedOverrides = {{true, true, false, false, true, false, false, true, false, false, false, false, false, false}},
    .label = "atomic_draw_image_mesh.webgpu_fixedcolor_frag",
};
} // namespace wgsl
