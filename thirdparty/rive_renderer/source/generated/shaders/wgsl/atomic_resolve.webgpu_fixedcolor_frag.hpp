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

inline constexpr Shader atomic_resolve_webgpu_fixedcolor_frag = {
    .source = R"WGSL(
struct Be{X1_:array<vec2<u32>>,}struct d0qd{X1_:array<u32>,}struct Ce{X1_:array<vec4<f32>>,}struct NB{Ub:f32,dd:f32,Xe:f32,Ye:f32,q5_:u32,ug:u32,Je:u32,Ke:u32,U7_:vec4<i32>,rg:vec2<f32>,ed:vec2<f32>,W1_:u32,vg:f32,Y5_:u32,P2_:f32,fd:f32,Ee:u32,y3_:f32,z3_:f32,gd:f32,og:u32,}struct p4qd{X1_:array<u32>,}@id(7) override Tg:bool=true;@id(4) override Qg:bool=true;@id(0) override Mg:bool=true;@id(1) override Ng:bool=true;@group(0)@binding(4) var<storage>TC:Be;@group(2)@binding(1) var<storage,read_write>d0_:d0qd;@group(0)@binding(5) var<storage>PB:Ce;var<private>_M:vec4<f32>;@group(0)@binding(9) var DD:texture_2d<f32>;@group(3)@binding(9) var Bb:sampler;@group(0)@binding(0) var<uniform>k:NB;@group(2)@binding(3) var<storage,read_write>p4_:p4qd;var<private>l1_:vec4<f32>;@group(3)@binding(10) var T9_:sampler;@group(0)@binding(10) var QC:texture_2d<f32>;@group(1)@binding(12) var AC:texture_2d<f32>;@group(1)@binding(14) var R5_:sampler;fn _2(){var _k:bool;var _m:f32;var _g:f32;var _l:f32;var _v:f32;var _t:f32;var _o:bool;var _p:f32;var _y:f32;var _h:vec4<f32>;var _u:vec3<f32>;let _f=_M;let _j=_f.xy;let _b=bitcast<vec2<u32>>(vec2<i32>(floor(_j)));let _5=k.q5_;let _K=bitcast<i32>((((((_b.y>>bitcast<u32>(5u))*(((_5+31u)&4294967264u)<<bitcast<u32>(5u)))+((_b.x>>bitcast<u32>(5u))<<bitcast<u32>(10u)))+(((_b.x&28u)<<bitcast<u32>(5u))+((_b.y&28u)<<bitcast<u32>(2i))))+(((_b.y&3u)<<bitcast<u32>(2i))+(_b.x&3u))));let _G=p4_.X1_[_K];let _N=((f32((_G&131071u))*0.00048828125f)+-32f);let _x=(_G>>bitcast<u32>(17u));let _a=TC.X1_[_x];_g=_N;if ((_a.x&768u)!=0u){let _E=abs(_N);_k=Qg;if Qg{_k=((_a.x&512u)!=0u);}let _3=_k;_m=_E;if _3{_m=(1f-abs(((fract((_E*0.5f))*2f)+-1f)));}let _0=_m;_g=_0;}let _X=_g;let _w=clamp(_X,0f,1f);_t=_w;if Mg{let _O=(_a.x>>bitcast<u32>(16u));_v=_w;if (_O!=0u){let _F=d0_.X1_[_K];if (_O==(_F>>bitcast<u32>(16i))){_l=min(_w,unpack2x16float(_F).x);}else{_l=0f;}let _R=_l;_v=_R;}let _U=_v;_t=_U;}let _B=_t;_o=Ng;if Ng{_o=((_a.x&1024u)!=0u);}let _V=_o;_p=_B;if _V{let _Q=(_x*4u);let _d=PB.X1_[(_Q+2u)];let _H=PB.X1_[(_Q+3u)];let _P=_H.zw;let _J=((abs(((mat2x2<f32>(vec2<f32>(_d.x,_d.y),vec2<f32>(_d.z,_d.w))*_j)+_H.xy))*_P)-_P);_p=min(_B,clamp((min(_J.x,_J.y)+0.5f),0f,1f));}let _W=_p;let _r=(_a.x&15u);if (_r<=1u){_h=select(unpack4x8unorm(_a.y),vec4<f32>(0f,0f,0f,0f),vec4((Mg&&(_r==0u))));}else{let _D=(_x*4u);let _e=PB.X1_[_D];let _z=PB.X1_[(_D+1u)];let _I=((mat2x2<f32>(vec2<f32>(_e.x,_e.y),vec2<f32>(_e.z,_e.w))*_j)+_z.xy);if (_r==2u){_y=_I.x;}else{_y=length(_I);}let _Z=_y;let _S=textureSampleLevel(DD,Bb,vec2<f32>(((clamp(_Z,0f,1f)*_z.z)+_z.w),bitcast<f32>(_a.y)),0f);_h=_S;}let _A=_h;let _C=(_A.w*_W);let _n=(_A.xyz*_C);let _c=vec4<f32>(_n.x,_n.y,_n.z,_C);let _L=_c.xyz;let _T=k.y3_;let _1=k.z3_;if Tg{_u=(vec3(((fract((52.982918f*fract(((0.06711056f*_f.x)+(0.00583715f*_f.y)))))*_T)+_1))+_L);}else{_u=_L;}let _q=_u;let _i=vec4<f32>(_q.x,_c.y,_c.z,_c.w);let _s=vec4<f32>(_i.x,_q.y,_i.z,_i.w);l1_=vec4<f32>(_s.x,_s.y,_q.z,_s.w);return;}@fragment fn main(@builtin(position) _4:vec4<f32>)->@location(0) vec4<f32>{_M=_4;_2();let _Y=l1_;return _Y;})WGSL",
    .usedOverrides = {{true, true, false, false, true, false, false, true, false, false, false, false, false, false}},
    .label = "atomic_resolve.webgpu_fixedcolor_frag",
};
} // namespace wgsl
