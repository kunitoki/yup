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

inline constexpr Shader draw_msaa_image_mesh_webgpu_vert = {
    .source = R"WGSL(
enable clip_distances;struct _l{@builtin(position) _i:vec4<f32>,_D:f32,@builtin(clip_distances) _b:array<f32,4>,_C:array<f32,1>,}struct LC{r9_:vec4<f32>,c2_:vec2<f32>,x4_:f32,ki:f32,k2_:vec4<f32>,D2_:vec2<f32>,V0_:u32,n2_:u32,Z6_:u32,}struct NB{Ub:f32,dd:f32,Xe:f32,Ye:f32,q5_:u32,ug:u32,Je:u32,Ke:u32,U7_:vec4<i32>,rg:vec2<f32>,ed:vec2<f32>,W1_:u32,vg:f32,Y5_:u32,P2_:f32,fd:f32,Ee:u32,y3_:f32,z3_:f32,gd:f32,og:u32,}struct VertexOutput{@builtin(position) _i:vec4<f32>,@builtin(clip_distances) _b:array<f32,4>,@location(0) member:vec2<f32>,@location(1)@interpolate(flat) member_1:f32,}@id(0) override Mg:bool=true;@id(1) override Ng:bool=true;var<private>_a:_l=_l(vec4<f32>(0f,0f,0f,1f),1f,array<f32,4>(),array<f32,1>());var<private>_B:i32;@group(0)@binding(2) var<uniform>A0_:LC;var<private>GC_1:vec2<f32>;var<private>U0_:vec2<f32>;var<private>HC_1:vec2<f32>;var<private>I3_:f32;@group(0)@binding(0) var<uniform>k:NB;fn _r(){var _j:f32;let _e=A0_.r9_;let _q=GC_1;let _v=A0_.c2_;let _g=((mat2x2<f32>(vec2<f32>(_e.x,_e.y),vec2<f32>(_e.z,_e.w))*_q)+_v);let _x=HC_1;U0_=_x;if Mg{let _n=A0_.V0_;let _z=k.Y5_;if (_n==0u){_j=0f;}else{_j=unpack2x16float(((_n+1023u)*_z)).x;}let _A=_j;I3_=_A;}if Ng{let _c=A0_.k2_;let _k=A0_.D2_;if any((_c!=vec4<f32>(0f,0f,0f,0f))){let _f=((mat2x2<f32>(vec2<f32>(_c.x,_c.y),vec2<f32>(_c.z,_c.w))*_g)+_k);_a._b[0i]=(_f.x+1f);_a._b[1i]=(_f.y+1f);_a._b[2i]=(1f-_f.x);_a._b[3i]=(1f-_f.y);}else{let _d=(_k.x-0.5f);_a._b[3i]=_d;_a._b[2i]=_d;_a._b[1i]=_d;_a._b[0i]=_d;}}let _u=k.Xe;let _m=k.Ye;let _h=vec4<f32>(((_g.x*_u)-1f),((_g.y*_m)-sign(_m)),0f,1f);let _s=A0_.Z6_;_a._i=vec4<f32>(_h.x,_h.y,(1f-(f32(_s)*0.000061035156f)),_h.w);return;}@vertex fn main(@builtin(vertex_index) _t:u32,@location(0) GC:vec2<f32>,@location(1) HC:vec2<f32>)->VertexOutput{_B=i32(_t);GC_1=GC;HC_1=HC;_r();let _y=_a._i;let _o=_a._b;let _p=U0_;let _w=I3_;return VertexOutput(_y,_o,_p,_w);})WGSL",
    .usedOverrides = {{true, true, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "draw_msaa_image_mesh.webgpu_vert",
};
} // namespace wgsl
