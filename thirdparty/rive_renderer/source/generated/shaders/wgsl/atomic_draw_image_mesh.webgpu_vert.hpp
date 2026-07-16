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

inline constexpr Shader atomic_draw_image_mesh_webgpu_vert = {
    .source = R"WGSL(
struct LC{r9_:vec4<f32>,c2_:vec2<f32>,x4_:f32,ki:f32,k2_:vec4<f32>,D2_:vec2<f32>,V0_:u32,n2_:u32,Z6_:u32,}struct NB{Ub:f32,dd:f32,Xe:f32,Ye:f32,q5_:u32,ug:u32,Je:u32,Ke:u32,U7_:vec4<i32>,rg:vec2<f32>,ed:vec2<f32>,W1_:u32,vg:f32,Y5_:u32,P2_:f32,fd:f32,Ee:u32,y3_:f32,z3_:f32,gd:f32,og:u32,}struct _o{@builtin(position) _g:vec4<f32>,_F:f32,_G:array<f32,1>,_H:array<f32,1>,}struct Rf{X1_:array<vec4<u32>>,}struct Be{X1_:array<vec2<u32>>,}struct Ce{X1_:array<vec4<f32>>,}struct Sf{X1_:array<vec4<u32>>,}struct VertexOutput{@location(0) member:vec2<f32>,@location(1) member_1:vec4<f32>,@builtin(position) _g:vec4<f32>,}@id(1) override Ng:bool=true;var<private>_s:i32;var<private>_x:i32;@group(0)@binding(2) var<uniform>A0_:LC;var<private>GC_1:vec2<f32>;var<private>U0_:vec2<f32>;var<private>HC_1:vec2<f32>;var<private>N0_:vec4<f32>;@group(0)@binding(0) var<uniform>k:NB;var<private>_i:_o=_o(vec4<f32>(0f,0f,0f,1f),1f,array<f32,1>(),array<f32,1>());@group(0)@binding(8) var DC:texture_2d<u32>;@group(0)@binding(10) var QC:texture_2d<f32>;@group(0)@binding(3) var<storage>MB:Rf;@group(0)@binding(4) var<storage>TC:Be;@group(0)@binding(5) var<storage>PB:Ce;@group(0)@binding(6) var<storage>XC:Sf;@group(3)@binding(10) var T9_:sampler;fn _B(){var _h:bool;var _e:vec4<f32>;let _a=A0_.r9_;let _D=GC_1;let _y=A0_.c2_;let _f=((mat2x2<f32>(vec2<f32>(_a.x,_a.y),vec2<f32>(_a.z,_a.w))*_D)+_y);let _t=HC_1;U0_=_t;if Ng{let _b=A0_.k2_;let _j=vec2<f32>(_b.x,_b.y);let _p=vec2<f32>(_b.z,_b.w);let _n=A0_.D2_;switch bitcast<i32>(0u){default:{let _d=(abs(_j)+abs(_p));let _k=(_d.x!=0f);_h=_k;if _k{_h=(_d.y!=0f);}let _u=_h;if _u{let _c=((mat2x2<f32>(_j,_p)*_f)+_n);let _m=-(_c);let _l=(vec2<f32>(1f,1f)/_d).xyxy;_e=(((vec4<f32>(_c.x,_c.y,_m.x,_m.y)*_l)+_l)+vec4<f32>(0.5f,0.5f,0.5f,0.5f));break;}else{_e=_n.xyxy;break;}}}let _z=_e;N0_=_z;}let _E=k.Xe;let _q=k.Ye;_i._g=vec4<f32>(((_f.x*_E)-1f),((_f.y*_q)-sign(_q)),0f,1f);return;}@vertex fn main(@builtin(vertex_index) _r:u32,@builtin(instance_index) _v:u32,@location(0) GC:vec2<f32>,@location(1) HC:vec2<f32>)->VertexOutput{_s=i32(_r);_x=i32(_v);GC_1=GC;HC_1=HC;_B();let _w=U0_;let _A=N0_;let _C=_i._g;return VertexOutput(_w,_A,_C);})WGSL",
    .usedOverrides = {{false, true, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "atomic_draw_image_mesh.webgpu_vert",
};
} // namespace wgsl
