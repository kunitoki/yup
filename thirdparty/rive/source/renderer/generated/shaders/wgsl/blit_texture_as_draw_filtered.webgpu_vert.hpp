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
struct _b{@builtin(position) _a:vec4<f32>,_l:f32,_m:array<f32,1>,_n:array<f32,1>,}struct UB{Qc:f32,Td:f32,bg:f32,cg:f32,A6_:u32,X9_:u32,Nf:u32,Of:u32,j8_:vec4<i32>,Lh:vec2<f32>,Ud:vec2<f32>,j2_:u32,Ph:f32,T4_:u32,a3_:f32,Vd:f32,Hf:u32,L3_:f32,M3_:f32,Wd:f32,Ih:u32,W9_:u32,wc:f32,xc:f32,}struct VertexOutput{@location(0) member:vec2<f32>,@builtin(position) _a:vec4<f32>,}var<private>_e:i32;var<private>f2_:vec2<f32>;var<private>_f:_b=_b(vec4<f32>(0f,0f,0f,1f),1f,array<f32,1>(),array<f32,1>());@group(0)@binding(0) var<uniform>j:UB;fn _i(){let _d=_e;let _g=select(1f,-1f,((_d&1i)==0i));let _c=select(1f,-1f,((_d&2i)==0i));f2_[0u]=((_g*0.5f)+0.5f);f2_[1u]=((_c*-0.5f)+0.5f);_f._a=vec4<f32>(_g,_c,0f,1f);return;}@vertex fn main(@builtin(vertex_index) _k:u32)->VertexOutput{_e=i32(_k);_i();let _j=f2_;let _h=_f._a;return VertexOutput(_j,_h);})WGSL",
    .usedOverrides = {{false, false, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "blit_texture_as_draw_filtered.webgpu_vert",
};
} // namespace wgsl
