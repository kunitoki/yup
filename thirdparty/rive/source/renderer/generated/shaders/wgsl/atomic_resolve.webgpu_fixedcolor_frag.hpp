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
struct rd{k2_:array<vec4<u32>>,}struct o0lf{k2_:array<u32>,}struct Fg{k2_:array<vec4<f32>>,}struct RB{Od:f32,Ze:f32,dh:f32,eh:f32,X5_:u32,Ka:u32,Og:u32,Qg:u32,Q8_:vec4<i32>,Yi:vec2<f32>,af:vec2<f32>,z2_:u32,cj:f32,B6_:u32,m3_:f32,bf:f32,Hg:u32,J3_:f32,K3_:f32,cf:f32,Vi:u32,Ja:u32,X8_:f32,Y8_:f32,}struct e5lf{k2_:array<u32>,}@id(8) override Jj:bool=true;@id(9) override Kj:bool=true;@id(5) override Gj:bool=true;@id(0) override Bj:bool=true;@id(1) override Cj:bool=true;@id(2) override Dj:bool=true;@group(0)@binding(2) var<storage>JB:rd;@group(2)@binding(1) var<storage,read_write>o0_:o0lf;@group(0)@binding(3) var<storage>KB:Fg;var<private>_W:vec4<f32>;@group(0)@binding(0) var<uniform>j:RB;@group(0)@binding(8) var AD:texture_2d<f32>;@group(3)@binding(8) var Z8_:sampler;@group(2)@binding(3) var<storage,read_write>e5_:e5lf;var<private>S1_:vec4<f32>;@group(3)@binding(9) var kb:sampler;@group(0)@binding(9) var BD:texture_2d<f32>;@group(1)@binding(11) var SB:texture_2d<f32>;@group(1)@binding(13) var x4_:sampler;fn _9(){var _r:bool;var _n:f32;var _C:f32;var _q:f32;var _t:f32;var _p:f32;var _i:bool;var _w:f32;var _B:f32;var _z:f32;var _o:vec2<f32>;var _E:vec4<f32>;var _v:vec4<f32>;var _D:vec3<f32>;let _m=_W;let _j=_m.xy;let _b=bitcast<vec2<u32>>(vec2<i32>(floor(_j)));let _ab=j.X5_;let _H=bitcast<i32>((((((_b.y>>bitcast<u32>(5u))*(((_ab+31u)&4294967264u)<<bitcast<u32>(5u)))+((_b.x>>bitcast<u32>(5u))<<bitcast<u32>(10u)))+(((_b.x&28u)<<bitcast<u32>(5u))+((_b.y&28u)<<bitcast<u32>(2i))))+(((_b.y&3u)<<bitcast<u32>(2i))+(_b.x&3u))));let _K=e5_.k2_[_H];let _U=((f32((_K&131071u))*0.00048828125f)+-32f);let _u=(_K>>bitcast<u32>(17u));let _a=JB.k2_[((_u*4u)+1u)];_C=_U;if ((_a.z&768u)!=0u){let _J=abs(_U);_r=Gj;if Gj{_r=((_a.z&512u)!=0u);}let _6=_r;_n=_J;if _6{_n=(1f-abs(((fract((_J*0.5f))*2f)+-1f)));}let _3=_n;_C=_3;}let _X=_C;let _h=clamp(_X,0f,1f);_p=_h;if Bj{let _P=(_a.z>>bitcast<u32>(16u));_t=_h;if (_P!=0u){let _F=o0_.k2_[_H];if (_P==(_F>>bitcast<u32>(16i))){_q=min(_h,unpack2x16float(_F).x);}else{_q=0f;}let _0=_q;_t=_0;}let _ad=_t;_p=_ad;}let _R=_p;_i=Cj;if Cj{_i=((_a.z&1024u)!=0u);}let _Y=_i;_w=_R;if _Y{let _O=(_u*8u);let _g=KB.k2_[(_O+2u)];let _G=KB.k2_[(_O+3u)];let _I=_G.zw;let _M=((abs(((mat2x2<f32>(vec2<f32>(_g.x,_g.y),vec2<f32>(_g.z,_g.w))*_j)+_G.xy))*_I)-_I);_w=min(_R,clamp((min(_M.x,_M.y)+0.5f),0f,1f));}let _4=_w;let _k=(_a.z&15u);if (_k<=1u){_v=select(unpack4x8unorm(_a.w),vec4<f32>(0f,0f,0f,0f),vec4((Bj&&(_k==0u))));}else{let _Q=(_u*8u);let _d=KB.k2_[_Q];let _e=KB.k2_[(_Q+1u)];let _s=((mat2x2<f32>(vec2<f32>(_d.x,_d.y),vec2<f32>(_d.z,_d.w))*_j)+_e.xy);let _N=f32(_k);let _5=j.X8_;let _1=j.Y8_;let _L=((_e.w*_5)+_1);switch bitcast<i32>(0u){default:{if (Kj&&(_N==4f)){let _T=normalize(_s);let _S=acos(clamp(_T.x,-1f,1f));if (_T.y>=0f){_z=_S;}else{_z=-(_S);}let _af=_z;_o=vec2<f32>((_af*0.15915494f),_L);break;}else{if (_N==2f){_B=_s.x;}else{_B=length(_s);}let _7=_B;_o=vec2<f32>(((clamp(_7,0f,1f)*select(0.001953125f,0.9980469f,(_e.z<0f)))+((max(0f,_e.z)*0.001953125f)+0.0009765625f)),_L);break;}}}let _ae=_o;let _f=textureSampleLevel(AD,Z8_,_ae,0f);_E=_f;if!((Dj&&(((_a.z>>bitcast<u32>(4i))&15u)!=0u))){let _A=(_f.xyz*_f.w);_E=vec4<f32>(_A.x,_A.y,_A.z,(_f.w*abs(bitcast<f32>(_a.w))));}let _Z=_E;_v=_Z;}let _8=_v;let _c=(_8*_4);let _V=_c.xyz;let _ag=j.J3_;let _ac=j.K3_;if (Jj&&(_c.w!=0f)){_D=(vec3(((fract((52.982918f*fract(((0.06711056f*_m.x)+(0.00583715f*_m.y)))))*_ag)+_ac))+_V);}else{_D=_V;}let _x=_D;let _l=vec4<f32>(_x.x,_c.y,_c.z,_c.w);let _y=vec4<f32>(_l.x,_x.y,_l.z,_l.w);S1_=vec4<f32>(_y.x,_y.y,_x.z,_y.w);return;}@fragment fn main(@builtin(position) _aa:vec4<f32>)->@location(0) vec4<f32>{_W=_aa;_9();let _2=S1_;return _2;})WGSL",
    .usedOverrides = {{true, true, true, false, false, true, false, false, true, true, false, false, false, false, false, false, false, false}},
    .label = "atomic_resolve.webgpu_fixedcolor_frag",
};
} // namespace wgsl
