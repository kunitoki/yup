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

inline constexpr Shader atomic_draw_atlas_blit_webgpu_fixedcolor_frag = {
    .source = R"WGSL(
struct Be{X1_:array<vec2<u32>>,}struct d0qd{X1_:array<u32>,}struct Ce{X1_:array<vec4<f32>>,}struct NB{Ub:f32,dd:f32,Xe:f32,Ye:f32,q5_:u32,ug:u32,Je:u32,Ke:u32,U7_:vec4<i32>,rg:vec2<f32>,ed:vec2<f32>,W1_:u32,vg:f32,Y5_:u32,P2_:f32,fd:f32,Ee:u32,y3_:f32,z3_:f32,gd:f32,og:u32,}struct p4qd{X1_:array<u32>,}@id(7) override Tg:bool=true;@id(4) override Qg:bool=true;@id(0) override Mg:bool=true;@id(1) override Ng:bool=true;@group(0)@binding(4) var<storage>TC:Be;@group(2)@binding(1) var<storage,read_write>d0_:d0qd;@group(0)@binding(5) var<storage>PB:Ce;var<private>_P:vec4<f32>;@group(0)@binding(9) var DD:texture_2d<f32>;@group(3)@binding(9) var Bb:sampler;@group(0)@binding(0) var<uniform>k:NB;@group(2)@binding(3) var<storage,read_write>p4_:p4qd;var<private>z0_1:u32;@group(0)@binding(11) var UC:texture_2d<f32>;@group(3)@binding(11) var I9_:sampler;var<private>C2_1:vec2<f32>;var<private>l1_:vec4<f32>;@group(3)@binding(10) var T9_:sampler;@group(0)@binding(10) var QC:texture_2d<f32>;@group(1)@binding(12) var AC:texture_2d<f32>;@group(1)@binding(14) var R5_:sampler;fn _9(){var _x:bool;var _p:f32;var _l:f32;var _o:f32;var _q:f32;var _t:f32;var _v:bool;var _z:f32;var _r:u32;var _g:f32;var _u:u32;var _j:vec4<f32>;var _A:vec3<f32>;let _i=_P;let _B=_i.xy;let _b=bitcast<vec2<u32>>(vec2<i32>(floor(_B)));let _ad=k.q5_;let _f=bitcast<i32>((((((_b.y>>bitcast<u32>(5u))*(((_ad+31u)&4294967264u)<<bitcast<u32>(5u)))+((_b.x>>bitcast<u32>(5u))<<bitcast<u32>(10u)))+(((_b.x&28u)<<bitcast<u32>(5u))+((_b.y&28u)<<bitcast<u32>(2i))))+(((_b.y&3u)<<bitcast<u32>(2i))+(_b.x&3u))));let _V=p4_.X1_[_f];let _y=(_V>>bitcast<u32>(17u));let _ab=z0_1;let _8=C2_1;let _1=textureSampleLevel(UC,I9_,_8,0f);p4_.X1_[_f]=(((_ab<<bitcast<u32>(17u))+65536u)+bitcast<u32>(i32(round((clamp(_1.x,0f,1f)*2048f)))));let _T=((f32((_V&131071u))*0.00048828125f)+-32f);let _a=TC.X1_[_y];_l=_T;if ((_a.x&768u)!=0u){let _J=abs(_T);_x=Qg;if Qg{_x=((_a.x&512u)!=0u);}let _ac=_x;_p=_J;if _ac{_p=(1f-abs(((fract((_J*0.5f))*2f)+-1f)));}let _aa=_p;_l=_aa;}let _2=_l;let _k=clamp(_2,0f,1f);_t=_k;if Mg{let _L=(_a.x>>bitcast<u32>(16u));_q=_k;if (_L!=0u){let _I=d0_.X1_[_f];if (_L==(_I>>bitcast<u32>(16i))){_o=min(_k,unpack2x16float(_I).x);}else{_o=0f;}let _3=_o;_q=_3;}let _X=_q;_t=_X;}let _M=_t;_v=Ng;if Ng{_v=((_a.x&1024u)!=0u);}let _4=_v;_z=_M;if _4{let _N=(_y*4u);let _d=PB.X1_[(_N+2u)];let _U=PB.X1_[(_N+3u)];let _E=_U.zw;let _Q=((abs(((mat2x2<f32>(vec2<f32>(_d.x,_d.y),vec2<f32>(_d.z,_d.w))*_B)+_U.xy))*_E)-_E);_z=min(_M,clamp((min(_Q.x,_Q.y)+0.5f),0f,1f));}let _G=_z;let _m=(_a.x&15u);if (_m<=1u){let _O=(Mg&&(_m==0u));_r=0u;if _O{_r=(_a.y|pack2x16float(vec2<f32>(_G,0f)));}let _6=_r;_u=_6;_j=select(unpack4x8unorm(_a.y),vec4<f32>(0f,0f,0f,0f),vec4(_O));}else{let _R=(_y*4u);let _e=PB.X1_[_R];let _h=PB.X1_[(_R+1u)];let _S=((mat2x2<f32>(vec2<f32>(_e.x,_e.y),vec2<f32>(_e.z,_e.w))*_B)+_h.xy);if (_m==2u){_g=_S.x;}else{_g=length(_S);}let _W=_g;let _Y=textureSampleLevel(DD,Bb,vec2<f32>(((clamp(_W,0f,1f)*_h.z)+_h.w),bitcast<f32>(_a.y)),0f);_u=0u;_j=_Y;}let _D=_u;let _F=_j;let _H=(_F.w*_G);let _n=(_F.xyz*_H);let _c=vec4<f32>(_n.x,_n.y,_n.z,_H);let _K=_c.xyz;let _7=k.y3_;let _Z=k.z3_;if Tg{_A=(vec3(((fract((52.982918f*fract(((0.06711056f*_i.x)+(0.00583715f*_i.y)))))*_7)+_Z))+_K);}else{_A=_K;}let _s=_A;let _C=vec4<f32>(_s.x,_c.y,_c.z,_c.w);let _w=vec4<f32>(_C.x,_s.y,_C.z,_C.w);l1_=vec4<f32>(_w.x,_w.y,_s.z,_w.w);if (_D!=0u){d0_.X1_[_f]=_D;}return;}@fragment fn main(@builtin(position) _0:vec4<f32>,@location(1)@interpolate(flat) z0_:u32,@location(0) C2_:vec2<f32>)->@location(0) vec4<f32>{_P=_0;z0_1=z0_;C2_1=C2_;_9();let _5=l1_;return _5;})WGSL",
    .usedOverrides = {{true, true, false, false, true, false, false, true, false, false, false, false, false, false}},
    .label = "atomic_draw_atlas_blit.webgpu_fixedcolor_frag",
};
} // namespace wgsl
