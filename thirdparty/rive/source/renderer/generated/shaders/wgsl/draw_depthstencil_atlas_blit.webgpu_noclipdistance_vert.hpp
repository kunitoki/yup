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

inline constexpr Shader draw_depthstencil_atlas_blit_webgpu_noclipdistance_vert = {
    .source = R"WGSL(
struct mh{k2_:array<vec4<u32>>,}struct UB{Qc:f32,Td:f32,bg:f32,cg:f32,A6_:u32,X9_:u32,Nf:u32,Of:u32,j8_:vec4<i32>,Lh:vec2<f32>,Ud:vec2<f32>,j2_:u32,Ph:f32,T4_:u32,a3_:f32,Vd:f32,Hf:u32,L3_:f32,M3_:f32,Wd:f32,Ih:u32,W9_:u32,wc:f32,xc:f32,}struct Ef{k2_:array<vec2<u32>>,}struct Ff{k2_:array<vec4<f32>>,}struct _E{@builtin(position) _t:vec4<f32>,_3:f32,_1:array<f32,1>,_2:array<f32,1>,}struct nh{k2_:array<vec4<u32>>,}struct VertexOutput{@location(1) member:vec2<f32>,@location(4)@interpolate(flat,either) member_1:f32,@location(6)@interpolate(flat,either) member_2:f32,@location(0) member_3:vec4<f32>,@location(9) member_4:vec3<f32>,@builtin(position) _t:vec4<f32>,}@id(0) override ki:bool=true;@id(2) override mi:bool=true;@id(8) override si:bool=true;@group(0)@binding(2) var<storage>LB:mh;@group(0)@binding(0) var<uniform>j:UB;var<private>_0:i32;var<private>MB_1:vec3<f32>;var<private>J2_:vec2<f32>;@group(0)@binding(3) var<storage>WC:Ef;var<private>Y3_:f32;var<private>Q0_:f32;@group(0)@binding(4) var<storage>JB:Ff;var<private>a1_:vec4<f32>;var<private>r1_:vec3<f32>;var<private>_y:_E=_E(vec4<f32>(0f,0f,0f,1f),1f,array<f32,1>(),array<f32,1>());@group(0)@binding(7) var TB:texture_2d<u32>;@group(0)@binding(9) var YC:texture_2d<f32>;@group(0)@binding(5) var<storage>ZC:nh;@group(3)@binding(9) var wa:sampler;fn _W(){var _q:u32;var _s:f32;var _j:f32;var _v:vec4<f32>;var _w:vec4<f32>;var _m:bool;var _o:f32;let _e=MB_1;let _f=(bitcast<u32>(_e.z)&65535u);let _x=LB.k2_[((_f*4u)+2u)];let _i=_e.xy;let _G=bitcast<vec3<f32>>(_x.yzw);let _L=j.Lh;J2_=(((_i*_G.x)+_G.yz)*_L);let _a=WC.k2_[_f];let _u=(_a.x&15u);if ki{let _D=(_u==0u);if _D{_q=_a.y;}else{_q=_a.x;}let _U=_q;let _B=(_U>>bitcast<u32>(16i));let _S=j.T4_;if (_B==0u){_s=0f;}else{_s=unpack2x16float(((_B+1023u)*_S)).x;}let _I=_s;_j=_I;if _D{_j=-(_I);}let _K=_j;Y3_=_K;}if mi{Q0_=f32(((_a.x>>bitcast<u32>(4i))&15u));}if (_u==1u){a1_=unpack4x8unorm(_a.y);}else{let _H=(_f*8u);let _g=JB.k2_[_H];let _n=JB.k2_[(_H+1u)];let _c=vec4<f32>(vec4<f32>().x,vec4<f32>().y,vec4<f32>().z,bitcast<f32>(_a.y));let _p=((mat2x2<f32>(vec2<f32>(_g.x,_g.y),vec2<f32>(_g.z,_g.w))*_i)+_n.xy);if (_n.z>0.9f){_v=vec4<f32>(_c.x,_c.y,2f,_c.w);}else{_v=vec4<f32>(_c.x,_c.y,_n.w,_c.w);}let _b=_v;if (f32(_u)==2f){let _r=vec4<f32>(_p.x,_b.y,_b.z,_b.w);_w=vec4<f32>(_r.x,0f,_r.z,_r.w);}else{let _k=vec4<f32>(_b.x,_b.y,-(_b.z),_b.w);let _h=vec4<f32>(_p.x,_k.y,_k.z,_k.w);_w=vec4<f32>(_h.x,_p.y,_h.z,_h.w);}let _Z=_w;a1_=_Z;let _Q=a1_[3u];a1_[3u]=-(_Q);}_m=si;if si{_m=((_a.x&2048u)!=0u);}let _V=_m;if _V{let _C=(_f*8u);let _d=JB.k2_[(_C+4u)];let _A=JB.k2_[(_C+5u)];let _F=((mat2x2<f32>(vec2<f32>(_d.x,_d.y),vec2<f32>(_d.z,_d.w))*_i)+_A.xy);_o=(1f+_A.z);if ((_a.x&4096u)!=0u){_o=(-1f-f32(((_a.x&24576u)>>bitcast<u32>(13u))));}let _R=_o;r1_=vec3<f32>(_F.x,_F.y,_R);}else{r1_=vec3<f32>(0f,0f,0f);}let _T=j.bg;let _z=j.cg;let _l=vec4<f32>(((_e.x*_T)-1f),((_e.y*_z)-sign(_z)),0f,1f);_y._t=vec4<f32>(_l.x,_l.y,((f32(((_x.x<<bitcast<u32>(8u))|255u))*0.000000059604645f)+0.000000029802322f),_l.w);return;}@vertex fn main(@builtin(vertex_index) _Y:u32,@location(0) MB:vec3<f32>)->VertexOutput{_0=i32(_Y);MB_1=MB;_W();let _O=J2_;let _X=Y3_;let _J=Q0_;let _M=a1_;let _N=r1_;let _P=_y._t;return VertexOutput(_O,_X,_J,_M,_N,_P);})WGSL",
    .usedOverrides = {{true, false, true, false, false, false, false, false, true, false, false, false, false, false, false}},
    .label = "draw_depthstencil_atlas_blit.webgpu_noclipdistance_vert",
};
} // namespace wgsl
