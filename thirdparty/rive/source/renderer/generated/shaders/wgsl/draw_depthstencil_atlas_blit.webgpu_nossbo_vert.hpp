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

inline constexpr Shader draw_depthstencil_atlas_blit_webgpu_nossbo_vert = {
    .source = R"WGSL(
enable clip_distances;struct _U{@builtin(position) _n:vec4<f32>,_ag:f32,@builtin(clip_distances) _b:array<f32,4>,_af:array<f32,1>,}struct UB{Qc:f32,Td:f32,bg:f32,cg:f32,A6_:u32,X9_:u32,Nf:u32,Of:u32,j8_:vec4<i32>,Lh:vec2<f32>,Ud:vec2<f32>,j2_:u32,Ph:f32,T4_:u32,a3_:f32,Vd:f32,Hf:u32,L3_:f32,M3_:f32,Wd:f32,Ih:u32,W9_:u32,wc:f32,xc:f32,}struct VertexOutput{@builtin(position) _n:vec4<f32>,@builtin(clip_distances) _b:array<f32,4>,@location(1) member:vec2<f32>,@location(4)@interpolate(flat,either) member_1:f32,@location(6)@interpolate(flat,either) member_2:f32,@location(0) member_3:vec4<f32>,@location(9) member_4:vec3<f32>,}@id(0) override ki:bool=true;@id(2) override mi:bool=true;@id(1) override li:bool=true;@id(8) override si:bool=true;var<private>_a:_U=_U(vec4<f32>(0f,0f,0f,1f),1f,array<f32,4>(),array<f32,1>());@group(0)@binding(2) var LB:texture_2d<u32>;@group(0)@binding(0) var<uniform>j:UB;var<private>_Z:i32;var<private>MB_1:vec3<f32>;var<private>J2_:vec2<f32>;@group(0)@binding(3) var WC:texture_2d<u32>;var<private>Y3_:f32;var<private>Q0_:f32;@group(0)@binding(4) var JB:texture_2d<f32>;var<private>a1_:vec4<f32>;var<private>r1_:vec3<f32>;@group(0)@binding(7) var TB:texture_2d<u32>;@group(0)@binding(9) var YC:texture_2d<f32>;@group(0)@binding(5) var ZC:texture_2d<u32>;@group(3)@binding(9) var wa:sampler;fn _X(){var _q:u32;var _w:f32;var _y:f32;var _A:vec4<f32>;var _s:vec4<f32>;var _p:bool;var _v:f32;let _l=MB_1;let _S=bitcast<u32>(_l.z);let _g=(_S&65535u);let _R=((_g*4u)+2u);let _K=textureLoad(LB,vec2<i32>(bitcast<i32>((_R&255u)),bitcast<i32>((_R>>bitcast<u32>(8i)))),0i);let _m=_l.xy;let _H=bitcast<vec3<f32>>(_K.yzw);let _7=j.Lh;J2_=(((_m*_H.x)+_H.yz)*_7);let _c=textureLoad(WC,vec2<i32>(bitcast<i32>((_S&255u)),bitcast<i32>((_g>>bitcast<u32>(8i)))),0i);let _o=(_c.x&15u);if ki{let _M=(_o==0u);if _M{_q=_c.y;}else{_q=_c.x;}let _ac=_q;let _V=(_ac>>bitcast<u32>(16i));let _W=j.T4_;if (_V==0u){_w=0f;}else{_w=unpack2x16float(((_V+1023u)*_W)).x;}let _T=_w;_y=_T;if _M{_y=-(_T);}let _3=_y;Y3_=_3;}if mi{Q0_=f32(((_c.x>>bitcast<u32>(4i))&15u));}if li{let _P=(_g*8u);let _D=(_P+2u);let _f=textureLoad(JB,vec2<i32>(bitcast<i32>((_D&255u)),bitcast<i32>((_D>>bitcast<u32>(8i)))),0i);let _F=(_P+3u);let _G=textureLoad(JB,vec2<i32>(bitcast<i32>((_F&255u)),bitcast<i32>((_F>>bitcast<u32>(8i)))),0i);if any((_f!=vec4<f32>(0f,0f,0f,0f))){let _i=((mat2x2<f32>(vec2<f32>(_f.x,_f.y),vec2<f32>(_f.z,_f.w))*_m)+_G.xy);_a._b[0i]=(_i.x+1f);_a._b[1i]=(_i.y+1f);_a._b[2i]=(1f-_i.x);_a._b[3i]=(1f-_i.y);}else{let _k=(_G.x-0.5f);_a._b[3i]=_k;_a._b[2i]=_k;_a._b[1i]=_k;_a._b[0i]=_k;}}if (_o==1u){a1_=unpack4x8unorm(_c.y);}else{let _x=(_g*8u);let _h=textureLoad(JB,vec2<i32>(bitcast<i32>((_x&255u)),bitcast<i32>((_x>>bitcast<u32>(8i)))),0i);let _Q=(_x+1u);let _B=textureLoad(JB,vec2<i32>(bitcast<i32>((_Q&255u)),bitcast<i32>((_Q>>bitcast<u32>(8i)))),0i);let _e=vec4<f32>(vec4<f32>().x,vec4<f32>().y,vec4<f32>().z,bitcast<f32>(_c.y));let _C=((mat2x2<f32>(vec2<f32>(_h.x,_h.y),vec2<f32>(_h.z,_h.w))*_m)+_B.xy);if (_B.z>0.9f){_A=vec4<f32>(_e.x,_e.y,2f,_e.w);}else{_A=vec4<f32>(_e.x,_e.y,_B.w,_e.w);}let _d=_A;if (f32(_o)==2f){let _r=vec4<f32>(_C.x,_d.y,_d.z,_d.w);_s=vec4<f32>(_r.x,0f,_r.z,_r.w);}else{let _t=vec4<f32>(_d.x,_d.y,-(_d.z),_d.w);let _z=vec4<f32>(_C.x,_t.y,_t.z,_t.w);_s=vec4<f32>(_z.x,_C.y,_z.z,_z.w);}let _aa=_s;a1_=_aa;let _ad=a1_[3u];a1_[3u]=-(_ad);}_p=si;if si{_p=((_c.x&2048u)!=0u);}let _8=_p;if _8{let _N=(_g*8u);let _L=(_N+4u);let _j=textureLoad(JB,vec2<i32>(bitcast<i32>((_L&255u)),bitcast<i32>((_L>>bitcast<u32>(8i)))),0i);let _I=(_N+5u);let _E=textureLoad(JB,vec2<i32>(bitcast<i32>((_I&255u)),bitcast<i32>((_I>>bitcast<u32>(8i)))),0i);let _J=((mat2x2<f32>(vec2<f32>(_j.x,_j.y),vec2<f32>(_j.z,_j.w))*_m)+_E.xy);_v=(1f+_E.z);if ((_c.x&4096u)!=0u){_v=(-1f-f32(((_c.x&24576u)>>bitcast<u32>(13u))));}let _9=_v;r1_=vec3<f32>(_J.x,_J.y,_9);}else{r1_=vec3<f32>(0f,0f,0f);}let _Y=j.bg;let _O=j.cg;let _u=vec4<f32>(((_l.x*_Y)-1f),((_l.y*_O)-sign(_O)),0f,1f);_a._n=vec4<f32>(_u.x,_u.y,((f32(((_K.x<<bitcast<u32>(8u))|255u))*0.000000059604645f)+0.000000029802322f),_u.w);return;}@vertex fn main(@builtin(vertex_index) _1:u32,@location(0) MB:vec3<f32>)->VertexOutput{_Z=i32(_1);MB_1=MB;_X();let _5=_a._n;let _ab=_a._b;let _ae=J2_;let _4=Y3_;let _2=Q0_;let _6=a1_;let _0=r1_;return VertexOutput(_5,_ab,_ae,_4,_2,_6,_0);})WGSL",
    .usedOverrides = {{true, true, true, false, false, false, false, false, true, false, false, false, false, false, false}},
    .label = "draw_depthstencil_atlas_blit.webgpu_nossbo_vert",
};
} // namespace wgsl
