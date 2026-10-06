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

inline constexpr Shader blit_texture_as_draw_filtered_webgpu_frag = {
    .source = R"WGSL(
struct UB{Qc:f32,Td:f32,bg:f32,cg:f32,A6_:u32,X9_:u32,Nf:u32,Of:u32,j8_:vec4<i32>,Lh:vec2<f32>,Ud:vec2<f32>,j2_:u32,Ph:f32,T4_:u32,a3_:f32,Vd:f32,Hf:u32,L3_:f32,M3_:f32,Wd:f32,Ih:u32,W9_:u32,wc:f32,xc:f32,}@group(1)@binding(11) var IC:texture_2d<f32>;@group(1)@binding(13) var Yf:sampler;var<private>f2_1:vec2<f32>;var<private>Sh:vec4<f32>;@group(0)@binding(0) var<uniform>j:UB;fn _d(){let _a=f2_1;let _c=textureSampleLevel(IC,Yf,_a,0f);Sh=_c;return;}@fragment fn main(@location(0) f2_:vec2<f32>)->@location(0) vec4<f32>{f2_1=f2_;_d();let _b=Sh;return _b;})WGSL",
    .usedOverrides = {{false, false, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "blit_texture_as_draw_filtered.webgpu_frag",
};
} // namespace wgsl
