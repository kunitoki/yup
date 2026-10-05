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

inline constexpr Shader atomic_draw_interior_triangles_webgpu_fixedcolor_frag = {
    .source = R"WGSL(
struct Ef{k2_:array<vec2<u32>>,}struct m0ge{k2_:array<u32>,}struct Ff{k2_:array<vec4<f32>>,}struct UB{Qc:f32,Td:f32,bg:f32,cg:f32,A6_:u32,X9_:u32,Nf:u32,Of:u32,j8_:vec4<i32>,Lh:vec2<f32>,Ud:vec2<f32>,j2_:u32,Ph:f32,T4_:u32,a3_:f32,Vd:f32,Hf:u32,L3_:f32,M3_:f32,Wd:f32,Ih:u32,W9_:u32,wc:f32,xc:f32,}struct K4ge{k2_:array<u32>,}@id(7) override ri:bool=true;@id(4) override oi:bool=true;@id(0) override ki:bool=true;@id(1) override li:bool=true;@id(2) override mi:bool=true;@group(0)@binding(3) var<storage>WC:Ef;@group(2)@binding(1) var<storage,read_write>m0_:m0ge;@group(0)@binding(4) var<storage>JB:Ff;var<private>_Z:vec4<f32>;@group(0)@binding(0) var<uniform>j:UB;@group(0)@binding(8) var ED:texture_2d<f32>;@group(3)@binding(8) var ha:sampler;@group(2)@binding(3) var<storage,read_write>K4_:K4ge;var<private>F0_1:u32;var<private>m1_1:f32;var<private>K1_:vec4<f32>;@group(3)@binding(9) var wa:sampler;@group(0)@binding(9) var YC:texture_2d<f32>;@group(1)@binding(11) var CC:texture_2d<f32>;@group(1)@binding(13) var r5_:sampler;fn _ae(){var _v:u32;var _n:bool;var _i:f32;var _s:f32;var _l:f32;var _I:f32;var _u:f32;var _x:bool;var _k:f32;var _C:u32;var _G:f32;var _p:vec4<f32>;var _B:u32;var _H:vec4<f32>;var _F:u32;var _E:vec4<f32>;var _w:vec3<f32>;let _D=_Z;let _j=_D.xy;let _b=bitcast<vec2<u32>>(vec2<i32>(floor(_j)));let _ad=j.A6_;let _f=bitcast<i32>((((((_b.y>>bitcast<u32>(5u))*(((_ad+31u)&4294967264u)<<bitcast<u32>(5u)))+((_b.x>>bitcast<u32>(5u))<<bitcast<u32>(10u)))+(((_b.x&28u)<<bitcast<u32>(5u))+((_b.y&28u)<<bitcast<u32>(2i))))+(((_b.y&3u)<<bitcast<u32>(2i))+(_b.x&3u))));let _m=K4_.k2_[_f];let _c=(_m>>bitcast<u32>(17u));let _y=F0_1;if (_c==_y){_v=_m;}else{_v=((_y<<bitcast<u32>(17u))+65536u);}let _9=_v;let _af=m1_1;K4_.k2_[_f]=(_9+bitcast<u32>(i32(round((_af*2048f)))));_F=0u;_E=vec4<f32>(0f,0f,0f,0f);if (_c!=_y){let _O=((f32((_m&131071u))*0.00048828125f)+-32f);let _a=WC.k2_[_c];_s=_O;if ((_a.x&768u)!=0u){let _S=abs(_O);_n=oi;if oi{_n=((_a.x&512u)!=0u);}let _5=_n;_i=_S;if _5{_i=(1f-abs(((fract((_S*0.5f))*2f)+-1f)));}let _8=_i;_s=_8;}let _aa=_s;let _r=clamp(_aa,0f,1f);_u=_r;if ki{let _Y=(_a.x>>bitcast<u32>(16u));_I=_r;if (_Y!=0u){let _Q=m0_.k2_[_f];if (_Y==(_Q>>bitcast<u32>(16i))){_l=min(_r,unpack2x16float(_Q).x);}else{_l=0f;}let _1=_l;_I=_1;}let _aj=_I;_u=_aj;}let _P=_u;_x=li;if li{_x=((_a.x&1024u)!=0u);}let _ac=_x;_k=_P;if _ac{let _L=(_c*8u);let _g=JB.k2_[(_L+2u)];let _M=JB.k2_[(_L+3u)];let _0=_M.zw;let _X=((abs(((mat2x2<f32>(vec2<f32>(_g.x,_g.y),vec2<f32>(_g.z,_g.w))*_j)+_M.xy))*_0)-_0);_k=min(_P,clamp((min(_X.x,_X.y)+0.5f),0f,1f));}let _N=_k;let _z=(_a.x&15u);if (_z<=1u){let _T=(ki&&(_z==0u));_C=0u;if _T{_C=(_a.y|pack2x16float(vec2<f32>(_N,0f)));}let _al=_C;_B=_al;_H=select(unpack4x8unorm(_a.y),vec4<f32>(0f,0f,0f,0f),vec4(_T));}else{let _K=(_c*8u);let _h=JB.k2_[_K];let _o=JB.k2_[(_K+1u)];let _W=((mat2x2<f32>(vec2<f32>(_h.x,_h.y),vec2<f32>(_h.z,_h.w))*_j)+_o.xy);if (_z==2u){_G=_W.x;}else{_G=length(_W);}let _ab=_G;let _V=bitcast<f32>(_a.y);let _ag=j.wc;let _ai=j.xc;let _e=textureSampleLevel(ED,ha,vec2<f32>(((clamp(_ab,0f,1f)*_o.z)+_o.w),((floor(_V)*_ag)+_ai)),0f);_p=_e;if!((mi&&(((_a.x>>bitcast<u32>(4i))&15u)!=0u))){let _t=(_e.xyz*_e.w);_p=vec4<f32>(_t.x,_t.y,_t.z,(_e.w*(fract(_V)*1.0039216f)));}let _6=_p;_B=0u;_H=_6;}let _3=_B;let _ah=_H;_F=_3;_E=(_ah*_N);}let _U=_F;let _d=_E;let _R=_d.xyz;let _7=j.L3_;let _2=j.M3_;if (ri&&(_d.w!=0f)){_w=(vec3(((fract((52.982918f*fract(((0.06711056f*_D.x)+(0.00583715f*_D.y)))))*_7)+_2))+_R);}else{_w=_R;}let _J=_w;let _A=vec4<f32>(_J.x,_d.y,_d.z,_d.w);let _q=vec4<f32>(_A.x,_J.y,_A.z,_A.w);K1_=vec4<f32>(_q.x,_q.y,_J.z,_q.w);if (_U!=0u){m0_.k2_[_f]=_U;}return;}@fragment fn main(@builtin(position) _ak:vec4<f32>,@location(1)@interpolate(flat,either) F0_:u32,@location(0)@interpolate(flat,either) m1_:f32)->@location(0) vec4<f32>{_Z=_ak;F0_1=F0_;m1_1=m1_;_ae();let _4=K1_;return _4;})WGSL",
    .usedOverrides = {{true, true, true, false, true, false, false, true, false, false, false, false, false, false, false}},
    .label = "atomic_draw_interior_triangles.webgpu_fixedcolor_frag",
};
} // namespace wgsl
