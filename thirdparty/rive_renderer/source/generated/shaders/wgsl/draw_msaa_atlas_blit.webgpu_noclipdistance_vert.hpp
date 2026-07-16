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

inline constexpr Shader draw_msaa_atlas_blit_webgpu_noclipdistance_vert = {
    .source = R"WGSL(
struct Rf{X1_:array<vec4<u32>>,}struct NB{Ub:f32,dd:f32,Xe:f32,Ye:f32,q5_:u32,ug:u32,Je:u32,Ke:u32,U7_:vec4<i32>,rg:vec2<f32>,ed:vec2<f32>,W1_:u32,vg:f32,Y5_:u32,P2_:f32,fd:f32,Ee:u32,y3_:f32,z3_:f32,gd:f32,og:u32,}struct Be{X1_:array<vec2<u32>>,}struct Ce{X1_:array<vec4<f32>>,}struct _x{@builtin(position) _n:vec4<f32>,_T:f32,_S:array<f32,1>,_R:array<f32,1>,}struct Sf{X1_:array<vec4<u32>>,}struct VertexOutput{@location(1) member:vec2<f32>,@location(4)@interpolate(flat) member_1:f32,@location(6)@interpolate(flat) member_2:f32,@location(0) member_3:vec4<f32>,@builtin(position) _n:vec4<f32>,}@id(0) override Mg:bool=true;@id(2) override Og:bool=true;@group(0)@binding(3) var<storage>MB:Rf;@group(0)@binding(0) var<uniform>k:NB;var<private>_D:i32;var<private>KB_1:vec3<f32>;var<private>C2_:vec2<f32>;@group(0)@binding(4) var<storage>TC:Be;var<private>I3_:f32;var<private>Z1_:f32;@group(0)@binding(5) var<storage>PB:Ce;var<private>i1_:vec4<f32>;var<private>_v:_x=_x(vec4<f32>(0f,0f,0f,1f),1f,array<f32,1>(),array<f32,1>());@group(0)@binding(8) var DC:texture_2d<u32>;@group(0)@binding(10) var QC:texture_2d<f32>;@group(0)@binding(6) var<storage>XC:Sf;@group(3)@binding(10) var T9_:sampler;fn _I(){var _j:u32;var _h:f32;var _o:f32;var _i:vec4<f32>;let _g=KB_1;let _y=(bitcast<u32>(_g.z)&65535u);let _q=(_y*4u);let _z=MB.X1_[(_q+2u)];let _B=_g.xy;let _w=bitcast<vec3<f32>>(_z.yzw);let _L=k.rg;C2_=(((_B*_w.x)+_w.yz)*_L);let _a=TC.X1_[_y];let _f=(_a.x&15u);if Mg{let _u=(_f==0u);if _u{_j=_a.y;}else{_j=_a.x;}let _F=_j;let _s=(_F>>bitcast<u32>(16i));let _G=k.Y5_;if (_s==0u){_h=0f;}else{_h=unpack2x16float(((_s+1023u)*_G)).x;}let _t=_h;_o=_t;if _u{_o=-(_t);}let _H=_o;I3_=_H;}if Og{Z1_=f32(((_a.x>>bitcast<u32>(4i))&15u));}if (_f==1u){let _b=unpack4x8unorm(_a.y);if Og{_i=_b;}else{let _m=(_b.xyz*_b.w);let _l=vec4<f32>(_m.x,_b.y,_b.z,_b.w);let _k=vec4<f32>(_l.x,_m.y,_l.z,_l.w);_i=vec4<f32>(_k.x,_k.y,_m.z,_k.w);}let _C=_i;i1_=_C;}else{let _e=PB.X1_[_q];let _d=PB.X1_[(_q+1u)];let _c=((mat2x2<f32>(vec2<f32>(_e.x,_e.y),vec2<f32>(_e.z,_e.w))*_B)+_d.xy);let _A=(_f==2u);if (_A||(_f==3u)){i1_[3u]=-(bitcast<f32>(_a.y));if (_d.z>0.9f){i1_[2u]=2f;}else{i1_[2u]=_d.w;}if _A{i1_[1u]=0f;i1_[0u]=_c.x;}else{let _N=i1_[2u];i1_[2u]=-(_N);i1_[0u]=_c.x;i1_[1u]=_c.y;}}else{i1_=vec4<f32>(_c.x,_c.y,bitcast<f32>(_a.y),(-2f-_d.z));}}let _P=k.Xe;let _r=k.Ye;let _p=vec4<f32>(((_g.x*_P)-1f),((_g.y*_r)-sign(_r)),0f,1f);_v._n=vec4<f32>(_p.x,_p.y,(1f-(f32(_z.x)*0.000061035156f)),_p.w);return;}@vertex fn main(@builtin(vertex_index) _Q:u32,@location(0) KB:vec3<f32>)->VertexOutput{_D=i32(_Q);KB_1=KB;_I();let _J=C2_;let _M=I3_;let _E=Z1_;let _O=i1_;let _K=_v._n;return VertexOutput(_J,_M,_E,_O,_K);})WGSL",
    .usedOverrides = {{true, false, true, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "draw_msaa_atlas_blit.webgpu_noclipdistance_vert",
};
} // namespace wgsl
