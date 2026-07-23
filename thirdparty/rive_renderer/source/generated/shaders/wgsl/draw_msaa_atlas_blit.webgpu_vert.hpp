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

inline constexpr Shader draw_msaa_atlas_blit_webgpu_vert = {
    .source = R"WGSL(
enable clip_distances;struct _A{@builtin(position) _t:vec4<f32>,_Z:f32,@builtin(clip_distances) _b:array<f32,4>,_Y:array<f32,1>,}struct Rf{X1_:array<vec4<u32>>,}struct NB{Ub:f32,dd:f32,Xe:f32,Ye:f32,q5_:u32,ug:u32,Je:u32,Ke:u32,U7_:vec4<i32>,rg:vec2<f32>,ed:vec2<f32>,W1_:u32,vg:f32,Y5_:u32,P2_:f32,fd:f32,Ee:u32,y3_:f32,z3_:f32,gd:f32,og:u32,}struct Be{X1_:array<vec2<u32>>,}struct Ce{X1_:array<vec4<f32>>,}struct Sf{X1_:array<vec4<u32>>,}struct VertexOutput{@builtin(position) _t:vec4<f32>,@builtin(clip_distances) _b:array<f32,4>,@location(1) member:vec2<f32>,@location(4)@interpolate(flat) member_1:f32,@location(6)@interpolate(flat) member_2:f32,@location(0) member_3:vec4<f32>,}@id(0) override Mg:bool=true;@id(2) override Og:bool=true;@id(1) override Ng:bool=true;var<private>_a:_A=_A(vec4<f32>(0f,0f,0f,1f),1f,array<f32,4>(),array<f32,1>());@group(0)@binding(3) var<storage>MB:Rf;@group(0)@binding(0) var<uniform>k:NB;var<private>_P:i32;var<private>KB_1:vec3<f32>;var<private>C2_:vec2<f32>;@group(0)@binding(4) var<storage>TC:Be;var<private>I3_:f32;var<private>Z1_:f32;@group(0)@binding(5) var<storage>PB:Ce;var<private>i1_:vec4<f32>;@group(0)@binding(8) var DC:texture_2d<u32>;@group(0)@binding(10) var QC:texture_2d<f32>;@group(0)@binding(6) var<storage>XC:Sf;@group(3)@binding(10) var T9_:sampler;fn _M(){var _p:u32;var _n:f32;var _v:f32;var _s:vec4<f32>;let _l=KB_1;let _F=(bitcast<u32>(_l.z)&65535u);let _m=(_F*4u);let _C=(_m+2u);let _G=MB.X1_[_C];let _o=_l.xy;let _y=bitcast<vec3<f32>>(_G.yzw);let _N=k.rg;C2_=(((_o*_y.x)+_y.yz)*_N);let _c=TC.X1_[_F];let _h=(_c.x&15u);if Mg{let _x=(_h==0u);if _x{_p=_c.y;}else{_p=_c.x;}let _J=_p;let _H=(_J>>bitcast<u32>(16i));let _U=k.Y5_;if (_H==0u){_n=0f;}else{_n=unpack2x16float(((_H+1023u)*_U)).x;}let _E=_n;_v=_E;if _x{_v=-(_E);}let _Q=_v;I3_=_Q;}if Og{Z1_=f32(((_c.x>>bitcast<u32>(4i))&15u));}if Ng{let _e=PB.X1_[_C];let _z=PB.X1_[(_m+3u)];if any((_e!=vec4<f32>(0f,0f,0f,0f))){let _g=((mat2x2<f32>(vec2<f32>(_e.x,_e.y),vec2<f32>(_e.z,_e.w))*_o)+_z.xy);_a._b[0i]=(_g.x+1f);_a._b[1i]=(_g.y+1f);_a._b[2i]=(1f-_g.x);_a._b[3i]=(1f-_g.y);}else{let _j=(_z.x-0.5f);_a._b[3i]=_j;_a._b[2i]=_j;_a._b[1i]=_j;_a._b[0i]=_j;}}if (_h==1u){let _d=unpack4x8unorm(_c.y);if Og{_s=_d;}else{let _u=(_d.xyz*_d.w);let _w=vec4<f32>(_u.x,_d.y,_d.z,_d.w);let _r=vec4<f32>(_w.x,_u.y,_w.z,_w.w);_s=vec4<f32>(_r.x,_r.y,_u.z,_r.w);}let _S=_s;i1_=_S;}else{let _k=PB.X1_[_m];let _i=PB.X1_[(_m+1u)];let _f=((mat2x2<f32>(vec2<f32>(_k.x,_k.y),vec2<f32>(_k.z,_k.w))*_o)+_i.xy);let _B=(_h==2u);if (_B||(_h==3u)){i1_[3u]=-(bitcast<f32>(_c.y));if (_i.z>0.9f){i1_[2u]=2f;}else{i1_[2u]=_i.w;}if _B{i1_[1u]=0f;i1_[0u]=_f.x;}else{let _X=i1_[2u];i1_[2u]=-(_X);i1_[0u]=_f.x;i1_[1u]=_f.y;}}else{i1_=vec4<f32>(_f.x,_f.y,bitcast<f32>(_c.y),(-2f-_i.z));}}let _W=k.Xe;let _D=k.Ye;let _q=vec4<f32>(((_l.x*_W)-1f),((_l.y*_D)-sign(_D)),0f,1f);_a._t=vec4<f32>(_q.x,_q.y,(1f-(f32(_G.x)*0.000061035156f)),_q.w);return;}@vertex fn main(@builtin(vertex_index) _V:u32,@location(0) KB:vec3<f32>)->VertexOutput{_P=i32(_V);KB_1=KB;_M();let _R=_a._t;let _T=_a._b;let _O=C2_;let _L=I3_;let _I=Z1_;let _K=i1_;return VertexOutput(_R,_T,_O,_L,_I,_K);})WGSL",
    .usedOverrides = {{true, true, true, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "draw_msaa_atlas_blit.webgpu_vert",
};
} // namespace wgsl
