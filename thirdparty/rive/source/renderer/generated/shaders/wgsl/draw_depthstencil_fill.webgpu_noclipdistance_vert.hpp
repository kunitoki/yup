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

inline constexpr Shader draw_depthstencil_fill_webgpu_noclipdistance_vert = {
    .source = R"WGSL(
struct nh{k2_:array<vec4<u32>>,}struct mh{k2_:array<vec4<u32>>,}struct Ef{k2_:array<vec2<u32>>,}struct UB{Qc:f32,Td:f32,bg:f32,cg:f32,A6_:u32,X9_:u32,Nf:u32,Of:u32,j8_:vec4<i32>,Lh:vec2<f32>,Ud:vec2<f32>,j2_:u32,Ph:f32,T4_:u32,a3_:f32,Vd:f32,Hf:u32,L3_:f32,M3_:f32,Wd:f32,Ih:u32,W9_:u32,wc:f32,xc:f32,}struct Ff{k2_:array<vec4<f32>>,}struct _N{@builtin(position) _l:vec4<f32>,_as:f32,_at:array<f32,1>,_ar:array<f32,1>,}struct VertexOutput{@location(4)@interpolate(flat,either) member:vec2<f32>,@location(6)@interpolate(flat,either) member_1:f32,@location(0) member_2:vec4<f32>,@location(9) member_3:vec3<f32>,@builtin(position) _l:vec4<f32>,}@id(0) override ki:bool=true;@id(2) override mi:bool=true;@id(8) override si:bool=true;var<private>_U:i32;@group(0)@binding(7) var TB:texture_2d<u32>;@group(0)@binding(5) var<storage>ZC:nh;@group(0)@binding(2) var<storage>LB:mh;@group(0)@binding(3) var<storage>WC:Ef;@group(0)@binding(0) var<uniform>j:UB;var<private>l1_:vec2<f32>;var<private>Q0_:f32;@group(0)@binding(4) var<storage>JB:Ff;var<private>a1_:vec4<f32>;var<private>r1_:vec3<f32>;var<private>_1:_N=_N(vec4<f32>(0f,0f,0f,1f),1f,array<f32,1>(),array<f32,1>());@group(0)@binding(9) var YC:texture_2d<f32>;@group(3)@binding(9) var wa:sampler;fn _af(){var _y:i32;var _x:vec4<u32>;var _t:vec4<u32>;var _A:vec2<f32>;var _o:u32;var _G:f32;var _H:f32;var _J:f32;var _I:vec4<f32>;var _n:vec4<f32>;var _k:bool;let _s=_U;let _p=((_s&1073741824i)!=0i);let _Y=(_s&536870911i);let _S=select(4i,5i,_p);let _K=(_Y&((1i<<bitcast<u32>(_S))-1i));let _O=select(8i,17i,_p);let _W=!(_p);let _q=(_W&&(_K==9i));let _u=select(_K,0i,_q);let _w=min(_u,(_O-1i));let _v=(((_Y>>bitcast<u32>(_S))*_O)+_w);let _j=textureLoad(TB,vec2<i32>((_v&2047i),(_v>>bitcast<u32>(11i))),0i);let _B=ZC.k2_[(max((_j.w&65535u),1u)-1u)];let _f=(_B.z&65535u);let _z=(_f*4u);let _al=LB.k2_[_z];let _h=bitcast<vec4<f32>>(_al);let _ab=LB.k2_[(_z+1u)];_y=_u;if ((((_j.w&8388608u)!=0u)&&_W)&&!(_q)){_y=(_u-1i);}let _0=_y;_t=_j;if (_0!=_w){let _X=((_v+_0)-_w);let _4=textureLoad(TB,vec2<i32>((_X&2047i),(_X>>bitcast<u32>(11i))),0i);if ((_4.w&8454143u)!=(_j.w&8454143u)){let _L=bitcast<i32>(_B.w);let _an=textureLoad(TB,vec2<i32>((_L&2047i),(_L>>bitcast<u32>(11i))),0i);_x=_an;}else{_x=_4;}let _9=_x;_t=_9;}let _am=_t;if _q{_A=bitcast<vec2<f32>>(_B.xy);}else{_A=bitcast<vec2<f32>>(_am.xy);}let _7=_A;let _i=((mat2x2<f32>(vec2<f32>(_h.x,_h.y),vec2<f32>(_h.z,_h.w))*_7)+bitcast<vec2<f32>>(_ab.xy));let _a=WC.k2_[_f];let _d=(_a.x&15u);if ki{let _T=(_d==0u);if _T{_o=_a.y;}else{_o=_a.x;}let _ao=_o;let _Z=(_ao>>bitcast<u32>(16i));let _8=j.T4_;if (_Z==0u){_G=0f;}else{_G=unpack2x16float(((_Z+1023u)*_8)).x;}let _P=_G;_H=_P;if _T{_H=-(_P);}let _ad=_H;l1_[0u]=_ad;}if mi{Q0_=f32(((_a.x>>bitcast<u32>(4i))&15u));}if (_d==1u){a1_=unpack4x8unorm(_a.y);}else{if (ki&&(_d==0u)){let _Q=(_a.x>>bitcast<u32>(16i));let _ac=j.T4_;if (_Q==0u){_J=0f;}else{_J=unpack2x16float(((_Q+1023u)*_ac)).x;}let _ag=_J;l1_[1u]=_ag;}else{let _M=(_f*8u);let _e=JB.k2_[_M];let _C=JB.k2_[(_M+1u)];let _c=vec4<f32>(vec4<f32>().x,vec4<f32>().y,vec4<f32>().z,bitcast<f32>(_a.y));let _F=((mat2x2<f32>(vec2<f32>(_e.x,_e.y),vec2<f32>(_e.z,_e.w))*_i)+_C.xy);if (_C.z>0.9f){_I=vec4<f32>(_c.x,_c.y,2f,_c.w);}else{_I=vec4<f32>(_c.x,_c.y,_C.w,_c.w);}let _b=_I;if (f32(_d)==2f){let _r=vec4<f32>(_F.x,_b.y,_b.z,_b.w);_n=vec4<f32>(_r.x,0f,_r.z,_r.w);}else{let _D=vec4<f32>(_b.x,_b.y,-(_b.z),_b.w);let _E=vec4<f32>(_F.x,_D.y,_D.z,_D.w);_n=vec4<f32>(_E.x,_F.y,_E.z,_E.w);}let _5=_n;a1_=_5;let _ai=a1_[3u];a1_[3u]=-(_ai);}}if ((_s&536870912i)!=0i){a1_=vec4<f32>(0f,0f,0f,0f);}_k=si;if si{_k=((_a.x&2048u)!=0u);}let _aj=_k;if _aj{let _3=(_f*8u);let _g=JB.k2_[(_3+4u)];let _R=JB.k2_[(_3+5u)];let _2=((mat2x2<f32>(vec2<f32>(_g.x,_g.y),vec2<f32>(_g.z,_g.w))*_i)+_R.xy);r1_=vec3<f32>(_2.x,_2.y,(1f+_R.z));}else{r1_=vec3<f32>(0f,0f,0f);}let _ak=j.bg;let _V=j.cg;let _m=vec4<f32>(((_i.x*_ak)-1f),((_i.y*_V)-sign(_V)),0f,1f);let _ap=LB.k2_[(_z+2u)];_1._l=vec4<f32>(_m.x,_m.y,((f32(((_ap.x<<bitcast<u32>(8u))|255u))*0.000000059604645f)+0.000000029802322f),_m.w);return;}@vertex fn main(@builtin(vertex_index) _ae:u32)->VertexOutput{_U=i32(_ae);_af();let _e9=l1_;let _ah=Q0_;let _aa=a1_;let _6=r1_;let _aq=_1._l;return VertexOutput(_e9,_ah,_aa,_6,_aq);})WGSL",
    .usedOverrides = {{true, false, true, false, false, false, false, false, true, false, false, false, false, false, false}},
    .label = "draw_depthstencil_fill.webgpu_noclipdistance_vert",
};
} // namespace wgsl
