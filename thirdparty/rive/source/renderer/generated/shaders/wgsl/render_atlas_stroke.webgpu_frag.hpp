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
struct UB{Qc:f32,Td:f32,bg:f32,cg:f32,A6_:u32,X9_:u32,Nf:u32,Of:u32,j8_:vec4<i32>,Lh:vec2<f32>,Ud:vec2<f32>,j2_:u32,Ph:f32,T4_:u32,a3_:f32,Vd:f32,Hf:u32,L3_:f32,M3_:f32,Wd:f32,Ih:u32,W9_:u32,wc:f32,xc:f32,}@group(0)@binding(9) var YC:texture_2d<f32>;@group(3)@binding(9) var wa:sampler;var<private>Sh:f32;var<private>S_1:vec4<f32>;@group(0)@binding(0) var<uniform>j:UB;@group(0)@binding(8) var ED:texture_2d<f32>;@group(1)@binding(11) var CC:texture_2d<f32>;@group(3)@binding(8) var ha:sampler;@group(1)@binding(13) var r5_:sampler;fn _c(){let _a=S_1;let _d=textureSampleLevel(YC,wa,vec2<f32>((3f+_a.x),0f),0f);let _e=textureSampleLevel(YC,wa,vec2<f32>((1f-_a.y),0f),0f);Sh=((1f-_d.x)-_e.x);return;}@fragment fn main(@location(0) S:vec4<f32>)->@location(0) f32{S_1=S;_c();let _b=Sh;return _b;})WGSL",
    .usedOverrides = {{false, false, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "render_atlas_stroke.webgpu_frag",
};
} // namespace wgsl
