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
struct _g{@builtin(position) _a:vec4<f32>,_m:f32,_l:array<f32,1>,_n:array<f32,1>,}struct RB{Od:f32,Ze:f32,dh:f32,eh:f32,X5_:u32,Ka:u32,Og:u32,Qg:u32,Q8_:vec4<i32>,Yi:vec2<f32>,af:vec2<f32>,z2_:u32,cj:f32,B6_:u32,m3_:f32,bf:f32,Hg:u32,J3_:f32,K3_:f32,cf:f32,Vi:u32,Ja:u32,X8_:f32,Y8_:f32,}struct VertexOutput{@location(0) member:vec2<f32>,@builtin(position) _a:vec4<f32>,}var<private>_e:i32;var<private>p2_:vec2<f32>;var<private>_c:_g=_g(vec4<f32>(0f,0f,0f,1f),1f,array<f32,1>(),array<f32,1>());@group(0)@binding(0) var<uniform>j:RB;fn _i(){let _b=_e;let _d=select(1f,-1f,((_b&1i)==0i));let _f=select(1f,-1f,((_b&2i)==0i));p2_[0u]=((_d*0.5f)+0.5f);p2_[1u]=((_f*-0.5f)+0.5f);_c._a=vec4<f32>(_d,_f,0f,1f);return;}@vertex fn main(@builtin(vertex_index) _j:u32)->VertexOutput{_e=i32(_j);_i();let _h=p2_;let _k=_c._a;return VertexOutput(_h,_k);})WGSL",
    .usedOverrides = {{false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "blit_texture_as_draw_filtered.webgpu_vert",
};
} // namespace wgsl
