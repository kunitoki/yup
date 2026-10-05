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
struct Ef{k2_:array<vec2<u32>>,}struct m0ge{k2_:array<u32>,}struct Ff{k2_:array<vec4<f32>>,}struct UB{Qc:f32,Td:f32,bg:f32,cg:f32,A6_:u32,X9_:u32,Nf:u32,Of:u32,j8_:vec4<i32>,Lh:vec2<f32>,Ud:vec2<f32>,j2_:u32,Ph:f32,T4_:u32,a3_:f32,Vd:f32,Hf:u32,L3_:f32,M3_:f32,Wd:f32,Ih:u32,W9_:u32,wc:f32,xc:f32,}struct K4ge{k2_:array<u32>,}@id(7) override ri:bool=true;@id(4) override oi:bool=true;@id(0) override ki:bool=true;@id(1) override li:bool=true;@id(2) override mi:bool=true;@group(0)@binding(3) var<storage>WC:Ef;@group(2)@binding(1) var<storage,read_write>m0_:m0ge;@group(0)@binding(4) var<storage>JB:Ff;var<private>_H:vec4<f32>;@group(0)@binding(0) var<uniform>j:UB;@group(0)@binding(8) var ED:texture_2d<f32>;@group(3)@binding(8) var ha:sampler;@group(2)@binding(3) var<storage,read_write>K4_:K4ge;var<private>F0_1:u32;@group(0)@binding(10) var FD:texture_2d<f32>;@group(3)@binding(10) var ma:sampler;var<private>J2_1:vec2<f32>;var<private>K1_:vec4<f32>;@group(3)@binding(9) var wa:sampler;@group(0)@binding(9) var YC:texture_2d<f32>;@group(1)@binding(11) var CC:texture_2d<f32>;@group(1)@binding(13) var r5_:sampler;fn _aa(){var _u:bool;var _t:f32;var _x:f32;var _p:f32;var _q:f32;var _h:f32;var _k:bool;var _w:f32;var _z:u32;var _y:f32;var _v:vec4<f32>;var _E:u32;var _n:vec4<f32>;var _s:vec3<f32>;let _B=_H;let _C=_B.xy;let _b=bitcast<vec2<u32>>(vec2<i32>(floor(_C)));let _Z=j.A6_;let _g=bitcast<i32>((((((_b.y>>bitcast<u32>(5u))*(((_Z+31u)&4294967264u)<<bitcast<u32>(5u)))+((_b.x>>bitcast<u32>(5u))<<bitcast<u32>(10u)))+(((_b.x&28u)<<bitcast<u32>(5u))+((_b.y&28u)<<bitcast<u32>(2i))))+(((_b.y&3u)<<bitcast<u32>(2i))+(_b.x&3u))));let _J=K4_.k2_[_g];let _r=(_J>>bitcast<u32>(17u));let _2=F0_1;let _5=J2_1;let _X=textureSampleLevel(FD,ma,_5,0f);K4_.k2_[_g]=(((_2<<bitcast<u32>(17u))+65536u)+bitcast<u32>(i32(round((clamp(_X.x,0f,1f)*2048f)))));let _N=((f32((_J&131071u))*0.00048828125f)+-32f);let _a=WC.k2_[_r];_x=_N;if ((_a.x&768u)!=0u){let _V=abs(_N);_u=oi;if oi{_u=((_a.x&512u)!=0u);}let _3=_u;_t=_V;if _3{_t=(1f-abs(((fract((_V*0.5f))*2f)+-1f)));}let _ah=_t;_x=_ah;}let _6=_x;let _j=clamp(_6,0f,1f);_h=_j;if ki{let _L=(_a.x>>bitcast<u32>(16u));_q=_j;if (_L!=0u){let _T=m0_.k2_[_g];if (_L==(_T>>bitcast<u32>(16i))){_p=min(_j,unpack2x16float(_T).x);}else{_p=0f;}let _0=_p;_q=_0;}let _8=_q;_h=_8;}let _F=_h;_k=li;if li{_k=((_a.x&1024u)!=0u);}let _9=_k;_w=_F;if _9{let _Q=(_r*8u);let _d=JB.k2_[(_Q+2u)];let _S=JB.k2_[(_Q+3u)];let _G=_S.zw;let _W=((abs(((mat2x2<f32>(vec2<f32>(_d.x,_d.y),vec2<f32>(_d.z,_d.w))*_C)+_S.xy))*_G)-_G);_w=min(_F,clamp((min(_W.x,_W.y)+0.5f),0f,1f));}let _R=_w;let _D=(_a.x&15u);if (_D<=1u){let _U=(ki&&(_D==0u));_z=0u;if _U{_z=(_a.y|pack2x16float(vec2<f32>(_R,0f)));}let _ac=_z;_E=_ac;_n=select(unpack4x8unorm(_a.y),vec4<f32>(0f,0f,0f,0f),vec4(_U));}else{let _I=(_r*8u);let _e=JB.k2_[_I];let _A=JB.k2_[(_I+1u)];let _P=((mat2x2<f32>(vec2<f32>(_e.x,_e.y),vec2<f32>(_e.z,_e.w))*_C)+_A.xy);if (_D==2u){_y=_P.x;}else{_y=length(_P);}let _ae=_y;let _M=bitcast<f32>(_a.y);let _ad=j.wc;let _af=j.xc;let _f=textureSampleLevel(ED,ha,vec2<f32>(((clamp(_ae,0f,1f)*_A.z)+_A.w),((floor(_M)*_ad)+_af)),0f);_v=_f;if!((mi&&(((_a.x>>bitcast<u32>(4i))&15u)!=0u))){let _l=(_f.xyz*_f.w);_v=vec4<f32>(_l.x,_l.y,_l.z,(_f.w*(fract(_M)*1.0039216f)));}let _Y=_v;_E=0u;_n=_Y;}let _K=_E;let _7=_n;let _c=(_7*_R);let _O=_c.xyz;let _ab=j.L3_;let _ag=j.M3_;if (ri&&(_c.w!=0f)){_s=(vec3(((fract((52.982918f*fract(((0.06711056f*_B.x)+(0.00583715f*_B.y)))))*_ab)+_ag))+_O);}else{_s=_O;}let _i=_s;let _m=vec4<f32>(_i.x,_c.y,_c.z,_c.w);let _o=vec4<f32>(_m.x,_i.y,_m.z,_m.w);K1_=vec4<f32>(_o.x,_o.y,_i.z,_o.w);if (_K!=0u){m0_.k2_[_g]=_K;}return;}@fragment fn main(@builtin(position) _1:vec4<f32>,@location(1)@interpolate(flat,either) F0_:u32,@location(0) J2_:vec2<f32>)->@location(0) vec4<f32>{_H=_1;F0_1=F0_;J2_1=J2_;_aa();let _4=K1_;return _4;})WGSL",
    .usedOverrides = {{true, true, true, false, true, false, false, true, false, false, false, false, false, false, false}},
    .label = "atomic_draw_atlas_blit.webgpu_fixedcolor_frag",
};
} // namespace wgsl
