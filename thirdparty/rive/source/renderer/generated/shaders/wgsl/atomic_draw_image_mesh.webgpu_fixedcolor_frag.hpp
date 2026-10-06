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
struct Ef{k2_:array<vec2<u32>>,}struct m0ge{k2_:array<u32>,}struct Ff{k2_:array<vec4<f32>>,}struct UB{Qc:f32,Td:f32,bg:f32,cg:f32,A6_:u32,X9_:u32,Nf:u32,Of:u32,j8_:vec4<i32>,Lh:vec2<f32>,Ud:vec2<f32>,j2_:u32,Ph:f32,T4_:u32,a3_:f32,Vd:f32,Hf:u32,L3_:f32,M3_:f32,Wd:f32,Ih:u32,W9_:u32,wc:f32,xc:f32,}struct K4ge{k2_:array<u32>,}@id(7) override ri:bool=true;@id(4) override oi:bool=true;@id(0) override ki:bool=true;@id(1) override li:bool=true;@id(2) override mi:bool=true;@group(0)@binding(3) var<storage>WC:Ef;@group(2)@binding(1) var<storage,read_write>m0_:m0ge;@group(0)@binding(4) var<storage>JB:Ff;var<private>_M:vec4<f32>;@group(0)@binding(0) var<uniform>j:UB;@group(0)@binding(8) var ED:texture_2d<f32>;@group(3)@binding(8) var ha:sampler;@group(1)@binding(11) var CC:texture_2d<f32>;@group(1)@binding(13) var r5_:sampler;var<private>f2_1:vec2<f32>;var<private>R0_1:vec4<f32>;@group(2)@binding(3) var<storage,read_write>K4_:K4ge;var<private>I3_1:u32;var<private>Q1_1:vec4<f32>;var<private>K1_:vec4<f32>;@group(3)@binding(9) var wa:sampler;@group(0)@binding(9) var YC:texture_2d<f32>;var<private>H1_1:u32;fn _av(){var _H:f32;var _D:bool;var _u:f32;var _I:f32;var _x:f32;var _n:f32;var _r:f32;var _E:bool;var _v:f32;var _t:u32;var _w:f32;var _C:vec4<f32>;var _j:u32;var _G:vec4<f32>;var _s:bool;var _F:u32;var _B:f32;var _A:f32;var _K:vec3<f32>;let _J=_M;let _q=_J.xy;let _b=bitcast<vec2<u32>>(vec2<i32>(floor(_q)));let _ad=j.A6_;let _c=bitcast<i32>((((((_b.y>>bitcast<u32>(5u))*(((_ad+31u)&4294967264u)<<bitcast<u32>(5u)))+((_b.x>>bitcast<u32>(5u))<<bitcast<u32>(10u)))+(((_b.x&28u)<<bitcast<u32>(5u))+((_b.y&28u)<<bitcast<u32>(2i))))+(((_b.y&3u)<<bitcast<u32>(2i))+(_b.x&3u))));let _ac=f2_1;let _ai=textureSample(CC,r5_,_ac);_H=1f;if li{let _W=R0_1;let _4=min(_W.xy,_W.zw);_H=clamp(min(_4.x,_4.y),0f,1f);}let _N=_H;let _L=K4_.k2_[_c];let _m=(_L>>bitcast<u32>(17u));let _O=((f32((_L&131071u))*0.00048828125f)+-32f);let _a=WC.k2_[_m];_I=_O;if ((_a.x&768u)!=0u){let _6=abs(_O);_D=oi;if oi{_D=((_a.x&512u)!=0u);}let _at=_D;_u=_6;if _at{_u=(1f-abs(((fract((_6*0.5f))*2f)+-1f)));}let _af=_u;_I=_af;}let _an=_I;let _i=clamp(_an,0f,1f);_r=_i;if ki{let _0=(_a.x>>bitcast<u32>(16u));_n=_i;if (_0!=0u){let _X=m0_.k2_[_c];if (_0==(_X>>bitcast<u32>(16i))){_x=min(_i,unpack2x16float(_X).x);}else{_x=0f;}let _al=_x;_n=_al;}let _ae=_n;_r=_ae;}let _Q=_r;_E=li;if li{_E=((_a.x&1024u)!=0u);}let _ab=_E;_v=_Q;if _ab{let _T=(_m*8u);let _g=JB.k2_[(_T+2u)];let _V=JB.k2_[(_T+3u)];let _2=_V.zw;let _3=((abs(((mat2x2<f32>(vec2<f32>(_g.x,_g.y),vec2<f32>(_g.z,_g.w))*_q)+_V.xy))*_2)-_2);_v=min(_Q,clamp((min(_3.x,_3.y)+0.5f),0f,1f));}let _S=_v;let _z=(_a.x&15u);if (_z<=1u){let _P=(ki&&(_z==0u));_t=0u;if _P{_t=(_a.y|pack2x16float(vec2<f32>(_S,0f)));}let _aw=_t;_j=_aw;_G=select(unpack4x8unorm(_a.y),vec4<f32>(0f,0f,0f,0f),vec4(_P));}else{let _U=(_m*8u);let _f=JB.k2_[_U];let _o=JB.k2_[(_U+1u)];let _Y=((mat2x2<f32>(vec2<f32>(_f.x,_f.y),vec2<f32>(_f.z,_f.w))*_q)+_o.xy);if (_z==2u){_w=_Y.x;}else{_w=length(_Y);}let _ak=_w;let _1=bitcast<f32>(_a.y);let _ao=j.wc;let _ar=j.xc;let _h=textureSampleLevel(ED,ha,vec2<f32>(((clamp(_ak,0f,1f)*_o.z)+_o.w),((floor(_1)*_ao)+_ar)),0f);_C=_h;if!((mi&&(((_a.x>>bitcast<u32>(4i))&15u)!=0u))){let _y=(_h.xyz*_h.w);_C=vec4<f32>(_y.x,_y.y,_y.z,(_h.w*(fract(_1)*1.0039216f)));}let _aj=_C;_j=0u;_G=_aj;}let _e=_j;let _ah=_G;_s=ki;if ki{let _aa=I3_1;_s=(_aa!=0u);}let _7=_s;_A=_N;if _7{if (_e!=0u){_F=_e;}else{let _au=m0_.k2_[_c];_F=_au;}let _5=_F;let _aq=I3_1;if (_aq==(_5>>bitcast<u32>(16i))){_B=min(_N,unpack2x16float(_5).x);}else{_B=0f;}let _9=_B;_A=_9;}let _ax=_A;let _as=Q1_1;let _R=((_ai*_as)*_ax);let _d=(((_ah*_S)*(1f-_R.w))+_R);let _Z=_d.xyz;let _ap=j.L3_;let _ag=j.M3_;if (ri&&(_d.w!=0f)){_K=(vec3(((fract((52.982918f*fract(((0.06711056f*_J.x)+(0.00583715f*_J.y)))))*_ap)+_ag))+_Z);}else{_K=_Z;}let _p=_K;let _k=vec4<f32>(_p.x,_d.y,_d.z,_d.w);let _l=vec4<f32>(_k.x,_p.y,_k.z,_k.w);K1_=vec4<f32>(_l.x,_l.y,_p.z,_l.w);if (_e!=0u){m0_.k2_[_c]=_e;}K4_.k2_[_c]=65536u;return;}@fragment fn main(@builtin(position) _am:vec4<f32>,@location(0) f2_:vec2<f32>,@location(1) R0_:vec4<f32>,@location(4)@interpolate(flat,either) I3_:u32,@location(3)@interpolate(flat,either) Q1_:vec4<f32>,@location(5)@interpolate(flat,either) H1_:u32)->@location(0) vec4<f32>{_M=_am;f2_1=f2_;R0_1=R0_;I3_1=I3_;Q1_1=Q1_;H1_1=H1_;_av();let _8=K1_;return _8;})WGSL",
    .usedOverrides = {{true, true, true, false, true, false, false, true, false, false, false, false, false, false, false}},
    .label = "atomic_draw_image_mesh.webgpu_fixedcolor_frag",
};
} // namespace wgsl
