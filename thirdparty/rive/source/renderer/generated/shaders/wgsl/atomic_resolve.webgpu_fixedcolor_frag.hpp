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
struct Ef{k2_:array<vec2<u32>>,}struct m0ge{k2_:array<u32>,}struct Ff{k2_:array<vec4<f32>>,}struct UB{Qc:f32,Td:f32,bg:f32,cg:f32,A6_:u32,X9_:u32,Nf:u32,Of:u32,j8_:vec4<i32>,Lh:vec2<f32>,Ud:vec2<f32>,j2_:u32,Ph:f32,T4_:u32,a3_:f32,Vd:f32,Hf:u32,L3_:f32,M3_:f32,Wd:f32,Ih:u32,W9_:u32,wc:f32,xc:f32,}struct K4ge{k2_:array<u32>,}@id(7) override ri:bool=true;@id(4) override oi:bool=true;@id(0) override ki:bool=true;@id(1) override li:bool=true;@id(2) override mi:bool=true;@group(0)@binding(3) var<storage>WC:Ef;@group(2)@binding(1) var<storage,read_write>m0_:m0ge;@group(0)@binding(4) var<storage>JB:Ff;var<private>_F:vec4<f32>;@group(0)@binding(0) var<uniform>j:UB;@group(0)@binding(8) var ED:texture_2d<f32>;@group(3)@binding(8) var ha:sampler;@group(2)@binding(3) var<storage,read_write>K4_:K4ge;var<private>K1_:vec4<f32>;@group(3)@binding(9) var wa:sampler;@group(0)@binding(9) var YC:texture_2d<f32>;@group(1)@binding(11) var CC:texture_2d<f32>;@group(1)@binding(13) var r5_:sampler;fn _S(){var _g:bool;var _z:f32;var _n:f32;var _v:f32;var _u:f32;var _q:f32;var _o:bool;var _r:f32;var _p:f32;var _x:vec4<f32>;var _j:vec4<f32>;var _m:vec3<f32>;let _h=_F;let _s=_h.xy;let _b=bitcast<vec2<u32>>(vec2<i32>(floor(_s)));let _W=j.A6_;let _E=bitcast<i32>((((((_b.y>>bitcast<u32>(5u))*(((_W+31u)&4294967264u)<<bitcast<u32>(5u)))+((_b.x>>bitcast<u32>(5u))<<bitcast<u32>(10u)))+(((_b.x&28u)<<bitcast<u32>(5u))+((_b.y&28u)<<bitcast<u32>(2i))))+(((_b.y&3u)<<bitcast<u32>(2i))+(_b.x&3u))));let _R=K4_.k2_[_E];let _M=((f32((_R&131071u))*0.00048828125f)+-32f);let _B=(_R>>bitcast<u32>(17u));let _a=WC.k2_[_B];_n=_M;if ((_a.x&768u)!=0u){let _J=abs(_M);_g=oi;if oi{_g=((_a.x&512u)!=0u);}let _4=_g;_z=_J;if _4{_z=(1f-abs(((fract((_J*0.5f))*2f)+-1f)));}let _3=_z;_n=_3;}let _8=_n;let _k=clamp(_8,0f,1f);_q=_k;if ki{let _N=(_a.x>>bitcast<u32>(16u));_u=_k;if (_N!=0u){let _Q=m0_.k2_[_E];if (_N==(_Q>>bitcast<u32>(16i))){_v=min(_k,unpack2x16float(_Q).x);}else{_v=0f;}let _0=_v;_u=_0;}let _5=_u;_q=_5;}let _C=_q;_o=li;if li{_o=((_a.x&1024u)!=0u);}let _9=_o;_r=_C;if _9{let _O=(_B*8u);let _e=JB.k2_[(_O+2u)];let _H=JB.k2_[(_O+3u)];let _P=_H.zw;let _K=((abs(((mat2x2<f32>(vec2<f32>(_e.x,_e.y),vec2<f32>(_e.z,_e.w))*_s)+_H.xy))*_P)-_P);_r=min(_C,clamp((min(_K.x,_K.y)+0.5f),0f,1f));}let _U=_r;let _A=(_a.x&15u);if (_A<=1u){_j=select(unpack4x8unorm(_a.y),vec4<f32>(0f,0f,0f,0f),vec4((ki&&(_A==0u))));}else{let _L=(_B*8u);let _d=JB.k2_[_L];let _w=JB.k2_[(_L+1u)];let _G=((mat2x2<f32>(vec2<f32>(_d.x,_d.y),vec2<f32>(_d.z,_d.w))*_s)+_w.xy);if (_A==2u){_p=_G.x;}else{_p=length(_G);}let _X=_p;let _I=bitcast<f32>(_a.y);let _V=j.wc;let _6=j.xc;let _f=textureSampleLevel(ED,ha,vec2<f32>(((clamp(_X,0f,1f)*_w.z)+_w.w),((floor(_I)*_V)+_6)),0f);_x=_f;if!((mi&&(((_a.x>>bitcast<u32>(4i))&15u)!=0u))){let _y=(_f.xyz*_f.w);_x=vec4<f32>(_y.x,_y.y,_y.z,(_f.w*(fract(_I)*1.0039216f)));}let _7=_x;_j=_7;}let _Y=_j;let _c=(_Y*_U);let _D=_c.xyz;let _1=j.L3_;let _T=j.M3_;if (ri&&(_c.w!=0f)){_m=(vec3(((fract((52.982918f*fract(((0.06711056f*_h.x)+(0.00583715f*_h.y)))))*_1)+_T))+_D);}else{_m=_D;}let _l=_m;let _i=vec4<f32>(_l.x,_c.y,_c.z,_c.w);let _t=vec4<f32>(_i.x,_l.y,_i.z,_i.w);K1_=vec4<f32>(_t.x,_t.y,_l.z,_t.w);return;}@fragment fn main(@builtin(position) _Z:vec4<f32>)->@location(0) vec4<f32>{_F=_Z;_S();let _2=K1_;return _2;})WGSL",
    .usedOverrides = {{true, true, true, false, true, false, false, true, false, false, false, false, false, false, false}},
    .label = "atomic_resolve.webgpu_fixedcolor_frag",
};
} // namespace wgsl
