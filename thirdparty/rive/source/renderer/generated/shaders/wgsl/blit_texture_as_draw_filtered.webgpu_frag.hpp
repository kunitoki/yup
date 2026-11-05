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
struct RB{Od:f32,Ze:f32,dh:f32,eh:f32,X5_:u32,Ka:u32,Og:u32,Qg:u32,Q8_:vec4<i32>,Yi:vec2<f32>,af:vec2<f32>,z2_:u32,cj:f32,B6_:u32,m3_:f32,bf:f32,Hg:u32,J3_:f32,K3_:f32,cf:f32,Vi:u32,Ja:u32,X8_:f32,Y8_:f32,}@group(1)@binding(11) var JC:texture_2d<f32>;@group(1)@binding(13) var ah:sampler;var<private>p2_1:vec2<f32>;var<private>fj:vec4<f32>;@group(0)@binding(0) var<uniform>j:RB;fn _a(){let _c=p2_1;let _d=textureSampleLevel(JC,ah,_c,0f);fj=_d;return;}@fragment fn main(@location(0) p2_:vec2<f32>)->@location(0) vec4<f32>{p2_1=p2_;_a();let _b=fj;return _b;})WGSL",
    .usedOverrides = {{false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "blit_texture_as_draw_filtered.webgpu_frag",
};
} // namespace wgsl
