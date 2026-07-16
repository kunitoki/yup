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

inline constexpr Shader blit_texture_as_draw_filtered_webgpu_vert = {
    .source = R"WGSL(
struct _c{@builtin(position) _a:vec4<f32>,_m:f32,_l:array<f32,1>,_n:array<f32,1>,}struct NB{Ub:f32,dd:f32,Xe:f32,Ye:f32,q5_:u32,ug:u32,Je:u32,Ke:u32,U7_:vec4<i32>,rg:vec2<f32>,ed:vec2<f32>,W1_:u32,vg:f32,Y5_:u32,P2_:f32,fd:f32,Ee:u32,y3_:f32,z3_:f32,gd:f32,og:u32,}struct VertexOutput{@location(0) member:vec2<f32>,@builtin(position) _a:vec4<f32>,}var<private>_f:i32;var<private>U0_:vec2<f32>;var<private>_b:_c=_c(vec4<f32>(0f,0f,0f,1f),1f,array<f32,1>(),array<f32,1>());@group(0)@binding(0) var<uniform>k:NB;fn _k(){let _e=_f;let _d=select(1f,-1f,((_e&1i)==0i));let _g=select(1f,-1f,((_e&2i)==0i));U0_[0u]=((_d*0.5f)+0.5f);U0_[1u]=((_g*-0.5f)+0.5f);_b._a=vec4<f32>(_d,_g,0f,1f);return;}@vertex fn main(@builtin(vertex_index) _j:u32)->VertexOutput{_f=i32(_j);_k();let _h=U0_;let _i=_b._a;return VertexOutput(_h,_i);})WGSL",
    .usedOverrides = {{false, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "blit_texture_as_draw_filtered.webgpu_vert",
};
} // namespace wgsl
