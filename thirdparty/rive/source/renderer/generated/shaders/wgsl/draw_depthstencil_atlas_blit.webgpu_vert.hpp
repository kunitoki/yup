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

inline constexpr Shader draw_depthstencil_atlas_blit_webgpu_vert = {
    .source = R"WGSL(
enable clip_distances;struct _D{@builtin(position) _z:vec4<f32>,_8:f32,@builtin(clip_distances) _b:array<f32,4>,_9:array<f32,1>,}struct mh{k2_:array<vec4<u32>>,}struct UB{Qc:f32,Td:f32,bg:f32,cg:f32,A6_:u32,X9_:u32,Nf:u32,Of:u32,j8_:vec4<i32>,Lh:vec2<f32>,Ud:vec2<f32>,j2_:u32,Ph:f32,T4_:u32,a3_:f32,Vd:f32,Hf:u32,L3_:f32,M3_:f32,Wd:f32,Ih:u32,W9_:u32,wc:f32,xc:f32,}struct Ef{k2_:array<vec2<u32>>,}struct Ff{k2_:array<vec4<f32>>,}struct nh{k2_:array<vec4<u32>>,}struct VertexOutput{@builtin(position) _z:vec4<f32>,@builtin(clip_distances) _b:array<f32,4>,@location(1) member:vec2<f32>,@location(4)@interpolate(flat,either) member_1:f32,@location(6)@interpolate(flat,either) member_2:f32,@location(0) member_3:vec4<f32>,@location(9) member_4:vec3<f32>,}@id(0) override ki:bool=true;@id(2) override mi:bool=true;@id(1) override li:bool=true;@id(8) override si:bool=true;var<private>_a:_D=_D(vec4<f32>(0f,0f,0f,1f),1f,array<f32,4>(),array<f32,1>());@group(0)@binding(2) var<storage>LB:mh;@group(0)@binding(0) var<uniform>j:UB;var<private>_1:i32;var<private>MB_1:vec3<f32>;var<private>J2_:vec2<f32>;@group(0)@binding(3) var<storage>WC:Ef;var<private>Y3_:f32;var<private>Q0_:f32;@group(0)@binding(4) var<storage>JB:Ff;var<private>a1_:vec4<f32>;var<private>r1_:vec3<f32>;@group(0)@binding(7) var TB:texture_2d<u32>;@group(0)@binding(9) var YC:texture_2d<f32>;@group(0)@binding(5) var<storage>ZC:nh;@group(3)@binding(9) var wa:sampler;fn _P(){var _A:u32;var _p:f32;var _r:f32;var _o:vec4<f32>;var _B:vec4<f32>;var _q:bool;var _u:f32;let _h=MB_1;let _f=(bitcast<u32>(_h.z)&65535u);let _F=LB.k2_[((_f*4u)+2u)];let _k=_h.xy;let _I=bitcast<vec3<f32>>(_F.yzw);let _W=j.Lh;J2_=(((_k*_I.x)+_I.yz)*_W);let _c=WC.k2_[_f];let _w=(_c.x&15u);if ki{let _N=(_w==0u);if _N{_A=_c.y;}else{_A=_c.x;}let _5=_A;let _L=(_5>>bitcast<u32>(16i));let _7=j.T4_;if (_L==0u){_p=0f;}else{_p=unpack2x16float(((_L+1023u)*_7)).x;}let _M=_p;_r=_M;if _N{_r=-(_M);}let _V=_r;Y3_=_V;}if mi{Q0_=f32(((_c.x>>bitcast<u32>(4i))&15u));}if li{let _E=(_f*8u);let _g=JB.k2_[(_E+2u)];let _J=JB.k2_[(_E+3u)];if any((_g!=vec4<f32>(0f,0f,0f,0f))){let _l=((mat2x2<f32>(vec2<f32>(_g.x,_g.y),vec2<f32>(_g.z,_g.w))*_k)+_J.xy);_a._b[0i]=(_l.x+1f);_a._b[1i]=(_l.y+1f);_a._b[2i]=(1f-_l.x);_a._b[3i]=(1f-_l.y);}else{let _j=(_J.x-0.5f);_a._b[3i]=_j;_a._b[2i]=_j;_a._b[1i]=_j;_a._b[0i]=_j;}}if (_w==1u){a1_=unpack4x8unorm(_c.y);}else{let _G=(_f*8u);let _i=JB.k2_[_G];let _t=JB.k2_[(_G+1u)];let _e=vec4<f32>(vec4<f32>().x,vec4<f32>().y,vec4<f32>().z,bitcast<f32>(_c.y));let _y=((mat2x2<f32>(vec2<f32>(_i.x,_i.y),vec2<f32>(_i.z,_i.w))*_k)+_t.xy);if (_t.z>0.9f){_o=vec4<f32>(_e.x,_e.y,2f,_e.w);}else{_o=vec4<f32>(_e.x,_e.y,_t.w,_e.w);}let _d=_o;if (f32(_w)==2f){let _x=vec4<f32>(_y.x,_d.y,_d.z,_d.w);_B=vec4<f32>(_x.x,0f,_x.z,_x.w);}else{let _n=vec4<f32>(_d.x,_d.y,-(_d.z),_d.w);let _v=vec4<f32>(_y.x,_n.y,_n.z,_n.w);_B=vec4<f32>(_v.x,_y.y,_v.z,_v.w);}let _S=_B;a1_=_S;let _T=a1_[3u];a1_[3u]=-(_T);}_q=si;if si{_q=((_c.x&2048u)!=0u);}let _U=_q;if _U{let _H=(_f*8u);let _m=JB.k2_[(_H+4u)];let _K=JB.k2_[(_H+5u)];let _O=((mat2x2<f32>(vec2<f32>(_m.x,_m.y),vec2<f32>(_m.z,_m.w))*_k)+_K.xy);_u=(1f+_K.z);if ((_c.x&4096u)!=0u){_u=(-1f-f32(((_c.x&24576u)>>bitcast<u32>(13u))));}let _R=_u;r1_=vec3<f32>(_O.x,_O.y,_R);}else{r1_=vec3<f32>(0f,0f,0f);}let _Q=j.bg;let _C=j.cg;let _s=vec4<f32>(((_h.x*_Q)-1f),((_h.y*_C)-sign(_C)),0f,1f);_a._z=vec4<f32>(_s.x,_s.y,((f32(((_F.x<<bitcast<u32>(8u))|255u))*0.000000059604645f)+0.000000029802322f),_s.w);return;}@vertex fn main(@builtin(vertex_index) _Y:u32,@location(0) MB:vec3<f32>)->VertexOutput{_1=i32(_Y);MB_1=MB;_P();let _X=_a._z;let _6=_a._b;let _2=J2_;let _3=Y3_;let _Z=Q0_;let _0=a1_;let _4=r1_;return VertexOutput(_X,_6,_2,_3,_Z,_0,_4);})WGSL",
    .usedOverrides = {{true, true, true, false, false, false, false, false, true, false, false, false, false, false, false}},
    .label = "draw_depthstencil_atlas_blit.webgpu_vert",
};
} // namespace wgsl
