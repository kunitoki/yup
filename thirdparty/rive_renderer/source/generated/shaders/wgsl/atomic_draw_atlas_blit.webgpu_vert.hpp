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

inline constexpr Shader atomic_draw_atlas_blit_webgpu_vert = {
    .source = R"WGSL(
struct Rf{X1_:array<vec4<u32>>,}struct NB{Ub:f32,dd:f32,Xe:f32,Ye:f32,q5_:u32,ug:u32,Je:u32,Ke:u32,U7_:vec4<i32>,rg:vec2<f32>,ed:vec2<f32>,W1_:u32,vg:f32,Y5_:u32,P2_:f32,fd:f32,Ee:u32,y3_:f32,z3_:f32,gd:f32,og:u32,}struct _e{@builtin(position) _b:vec4<f32>,_t:f32,_s:array<f32,1>,_u:array<f32,1>,}struct Be{X1_:array<vec2<u32>>,}struct Ce{X1_:array<vec4<f32>>,}struct Sf{X1_:array<vec4<u32>>,}struct VertexOutput{@location(0) member:vec2<f32>,@location(1)@interpolate(flat) member_1:u32,@builtin(position) _b:vec4<f32>,}@group(0)@binding(3) var<storage>MB:Rf;@group(0)@binding(0) var<uniform>k:NB;var<private>_i:i32;var<private>_p:i32;var<private>KB_1:vec3<f32>;var<private>C2_:vec2<f32>;var<private>z0_:u32;var<private>_g:_e=_e(vec4<f32>(0f,0f,0f,1f),1f,array<f32,1>(),array<f32,1>());@group(0)@binding(8) var DC:texture_2d<u32>;@group(0)@binding(10) var QC:texture_2d<f32>;@group(0)@binding(4) var<storage>TC:Be;@group(0)@binding(5) var<storage>PB:Ce;@group(0)@binding(6) var<storage>XC:Sf;@group(3)@binding(10) var T9_:sampler;fn _q(){let _a=KB_1;let _d=(bitcast<u32>(_a.z)&65535u);let _h=MB.X1_[((_d*4u)+2u)];let _c=bitcast<vec3<f32>>(_h.yzw);let _j=k.rg;C2_=(((_a.xy*_c.x)+_c.yz)*_j);z0_=_d;let _k=k.Xe;let _f=k.Ye;_g._b=vec4<f32>(((_a.x*_k)-1f),((_a.y*_f)-sign(_f)),0f,1f);return;}@vertex fn main(@builtin(vertex_index) _r:u32,@builtin(instance_index) _n:u32,@location(0) KB:vec3<f32>)->VertexOutput{_i=i32(_r);_p=i32(_n);KB_1=KB;_q();let _m=C2_;let _o=z0_;let _l=_g._b;return VertexOutput(_m,_o,_l);})WGSL",
    .usedOverrides = {{false, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "atomic_draw_atlas_blit.webgpu_vert",
};
} // namespace wgsl
