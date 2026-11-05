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

inline constexpr Shader render_atlas_stroke_webgpu_frag = {
    .source = R"WGSL(
struct RB{Od:f32,Ze:f32,dh:f32,eh:f32,X5_:u32,Ka:u32,Og:u32,Qg:u32,Q8_:vec4<i32>,Yi:vec2<f32>,af:vec2<f32>,z2_:u32,cj:f32,B6_:u32,m3_:f32,bf:f32,Hg:u32,J3_:f32,K3_:f32,cf:f32,Vi:u32,Ja:u32,X8_:f32,Y8_:f32,}@group(0)@binding(9) var BD:texture_2d<f32>;@group(3)@binding(9) var kb:sampler;var<private>fj:f32;var<private>S_1:vec4<f32>;@group(0)@binding(0) var<uniform>j:RB;@group(0)@binding(8) var AD:texture_2d<f32>;@group(1)@binding(11) var SB:texture_2d<f32>;@group(3)@binding(8) var Z8_:sampler;@group(1)@binding(13) var x4_:sampler;fn _c(){let _a=S_1;let _d=textureSampleLevel(BD,kb,vec2<f32>((3f+_a.x),0f),0f);let _e=textureSampleLevel(BD,kb,vec2<f32>((1f-_a.y),0f),0f);fj=((1f-_d.x)-_e.x);return;}@fragment fn main(@location(0) S:vec4<f32>)->@location(0) f32{S_1=S;_c();let _b=fj;return _b;})WGSL",
    .usedOverrides = {{false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "render_atlas_stroke.webgpu_frag",
};
} // namespace wgsl
