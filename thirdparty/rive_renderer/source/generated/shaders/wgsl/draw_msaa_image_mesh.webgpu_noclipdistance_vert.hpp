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

inline constexpr Shader draw_msaa_image_mesh_webgpu_noclipdistance_vert = {
    .source = R"WGSL(
struct LC{r9_:vec4<f32>,c2_:vec2<f32>,x4_:f32,ki:f32,k2_:vec4<f32>,D2_:vec2<f32>,V0_:u32,n2_:u32,Z6_:u32,}struct NB{Ub:f32,dd:f32,Xe:f32,Ye:f32,q5_:u32,ug:u32,Je:u32,Ke:u32,U7_:vec4<i32>,rg:vec2<f32>,ed:vec2<f32>,W1_:u32,vg:f32,Y5_:u32,P2_:f32,fd:f32,Ee:u32,y3_:f32,z3_:f32,gd:f32,og:u32,}struct _h{@builtin(position) _b:vec4<f32>,_y:f32,_x:array<f32,1>,_w:array<f32,1>,}struct VertexOutput{@location(0) member:vec2<f32>,@location(1)@interpolate(flat) member_1:f32,@builtin(position) _b:vec4<f32>,}@id(0) override Mg:bool=true;var<private>_k:i32;@group(0)@binding(2) var<uniform>A0_:LC;var<private>GC_1:vec2<f32>;var<private>U0_:vec2<f32>;var<private>HC_1:vec2<f32>;var<private>I3_:f32;@group(0)@binding(0) var<uniform>k:NB;var<private>_e:_h=_h(vec4<f32>(0f,0f,0f,1f),1f,array<f32,1>(),array<f32,1>());fn _v(){var _d:f32;let _a=A0_.r9_;let _t=GC_1;let _s=A0_.c2_;let _f=((mat2x2<f32>(vec2<f32>(_a.x,_a.y),vec2<f32>(_a.z,_a.w))*_t)+_s);let _l=HC_1;U0_=_l;if Mg{let _i=A0_.V0_;let _p=k.Y5_;if (_i==0u){_d=0f;}else{_d=unpack2x16float(((_i+1023u)*_p)).x;}let _u=_d;I3_=_u;}let _j=k.Xe;let _g=k.Ye;let _c=vec4<f32>(((_f.x*_j)-1f),((_f.y*_g)-sign(_g)),0f,1f);let _n=A0_.Z6_;_e._b=vec4<f32>(_c.x,_c.y,(1f-(f32(_n)*0.000061035156f)),_c.w);return;}@vertex fn main(@builtin(vertex_index) _m:u32,@location(0) GC:vec2<f32>,@location(1) HC:vec2<f32>)->VertexOutput{_k=i32(_m);GC_1=GC;HC_1=HC;_v();let _o=U0_;let _q=I3_;let _r=_e._b;return VertexOutput(_o,_q,_r);})WGSL",
    .usedOverrides = {{true, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "draw_msaa_image_mesh.webgpu_noclipdistance_vert",
};
} // namespace wgsl
