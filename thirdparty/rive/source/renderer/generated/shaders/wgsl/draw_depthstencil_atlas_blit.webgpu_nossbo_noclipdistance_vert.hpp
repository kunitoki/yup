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

inline constexpr Shader draw_depthstencil_atlas_blit_webgpu_nossbo_noclipdistance_vert = {
    .source = R"WGSL(
struct UB{Qc:f32,Td:f32,bg:f32,cg:f32,A6_:u32,X9_:u32,Nf:u32,Of:u32,j8_:vec4<i32>,Lh:vec2<f32>,Ud:vec2<f32>,j2_:u32,Ph:f32,T4_:u32,a3_:f32,Vd:f32,Hf:u32,L3_:f32,M3_:f32,Wd:f32,Ih:u32,W9_:u32,wc:f32,xc:f32,}struct _M{@builtin(position) _p:vec4<f32>,_8:f32,_6:array<f32,1>,_7:array<f32,1>,}struct VertexOutput{@location(1) member:vec2<f32>,@location(4)@interpolate(flat,either) member_1:f32,@location(6)@interpolate(flat,either) member_2:f32,@location(0) member_3:vec4<f32>,@location(9) member_4:vec3<f32>,@builtin(position) _p:vec4<f32>,}@id(0) override ki:bool=true;@id(2) override mi:bool=true;@id(8) override si:bool=true;@group(0)@binding(2) var LB:texture_2d<u32>;@group(0)@binding(0) var<uniform>j:UB;var<private>_0:i32;var<private>MB_1:vec3<f32>;var<private>J2_:vec2<f32>;@group(0)@binding(3) var WC:texture_2d<u32>;var<private>Y3_:f32;var<private>Q0_:f32;@group(0)@binding(4) var JB:texture_2d<f32>;var<private>a1_:vec4<f32>;var<private>r1_:vec3<f32>;var<private>_B:_M=_M(vec4<f32>(0f,0f,0f,1f),1f,array<f32,1>(),array<f32,1>());@group(0)@binding(7) var TB:texture_2d<u32>;@group(0)@binding(9) var YC:texture_2d<f32>;@group(0)@binding(5) var ZC:texture_2d<u32>;@group(3)@binding(9) var wa:sampler;fn _S(){var _q:u32;var _j:f32;var _h:f32;var _r:vec4<f32>;var _x:vec4<f32>;var _u:bool;var _w:f32;let _g=MB_1;let _L=bitcast<u32>(_g.z);let _f=(_L&65535u);let _D=((_f*4u)+2u);let _K=textureLoad(LB,vec2<i32>(bitcast<i32>((_D&255u)),bitcast<i32>((_D>>bitcast<u32>(8i)))),0i);let _m=_g.xy;let _J=bitcast<vec3<f32>>(_K.yzw);let _R=j.Lh;J2_=(((_m*_J.x)+_J.yz)*_R);let _a=textureLoad(WC,vec2<i32>(bitcast<i32>((_L&255u)),bitcast<i32>((_f>>bitcast<u32>(8i)))),0i);let _i=(_a.x&15u);if ki{let _z=(_i==0u);if _z{_q=_a.y;}else{_q=_a.x;}let _3=_q;let _y=(_3>>bitcast<u32>(16i));let _Q=j.T4_;if (_y==0u){_j=0f;}else{_j=unpack2x16float(((_y+1023u)*_Q)).x;}let _E=_j;_h=_E;if _z{_h=-(_E);}let _2=_h;Y3_=_2;}if mi{Q0_=f32(((_a.x>>bitcast<u32>(4i))&15u));}if (_i==1u){a1_=unpack4x8unorm(_a.y);}else{let _k=(_f*8u);let _e=textureLoad(JB,vec2<i32>(bitcast<i32>((_k&255u)),bitcast<i32>((_k>>bitcast<u32>(8i)))),0i);let _F=(_k+1u);let _l=textureLoad(JB,vec2<i32>(bitcast<i32>((_F&255u)),bitcast<i32>((_F>>bitcast<u32>(8i)))),0i);let _c=vec4<f32>(vec4<f32>().x,vec4<f32>().y,vec4<f32>().z,bitcast<f32>(_a.y));let _t=((mat2x2<f32>(vec2<f32>(_e.x,_e.y),vec2<f32>(_e.z,_e.w))*_m)+_l.xy);if (_l.z>0.9f){_r=vec4<f32>(_c.x,_c.y,2f,_c.w);}else{_r=vec4<f32>(_c.x,_c.y,_l.w,_c.w);}let _b=_r;if (f32(_i)==2f){let _s=vec4<f32>(_t.x,_b.y,_b.z,_b.w);_x=vec4<f32>(_s.x,0f,_s.z,_s.w);}else{let _n=vec4<f32>(_b.x,_b.y,-(_b.z),_b.w);let _v=vec4<f32>(_t.x,_n.y,_n.z,_n.w);_x=vec4<f32>(_v.x,_t.y,_v.z,_v.w);}let _U=_x;a1_=_U;let _W=a1_[3u];a1_[3u]=-(_W);}_u=si;if si{_u=((_a.x&2048u)!=0u);}let _1=_u;if _1{let _C=(_f*8u);let _G=(_C+4u);let _d=textureLoad(JB,vec2<i32>(bitcast<i32>((_G&255u)),bitcast<i32>((_G>>bitcast<u32>(8i)))),0i);let _N=(_C+5u);let _I=textureLoad(JB,vec2<i32>(bitcast<i32>((_N&255u)),bitcast<i32>((_N>>bitcast<u32>(8i)))),0i);let _H=((mat2x2<f32>(vec2<f32>(_d.x,_d.y),vec2<f32>(_d.z,_d.w))*_m)+_I.xy);_w=(1f+_I.z);if ((_a.x&4096u)!=0u){_w=(-1f-f32(((_a.x&24576u)>>bitcast<u32>(13u))));}let _V=_w;r1_=vec3<f32>(_H.x,_H.y,_V);}else{r1_=vec3<f32>(0f,0f,0f);}let _P=j.bg;let _A=j.cg;let _o=vec4<f32>(((_g.x*_P)-1f),((_g.y*_A)-sign(_A)),0f,1f);_B._p=vec4<f32>(_o.x,_o.y,((f32(((_K.x<<bitcast<u32>(8u))|255u))*0.000000059604645f)+0.000000029802322f),_o.w);return;}@vertex fn main(@builtin(vertex_index) _Z:u32,@location(0) MB:vec3<f32>)->VertexOutput{_0=i32(_Z);MB_1=MB;_S();let _O=J2_;let _T=Y3_;let _X=Q0_;let _Y=a1_;let _5=r1_;let _4=_B._p;return VertexOutput(_O,_T,_X,_Y,_5,_4);})WGSL",
    .usedOverrides = {{true, false, true, false, false, false, false, false, true, false, false, false, false, false, false}},
    .label = "draw_depthstencil_atlas_blit.webgpu_nossbo_noclipdistance_vert",
};
} // namespace wgsl
