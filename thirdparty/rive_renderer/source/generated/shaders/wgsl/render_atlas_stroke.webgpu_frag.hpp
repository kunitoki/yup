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
struct NB{Ub:f32,dd:f32,Xe:f32,Ye:f32,q5_:u32,ug:u32,Je:u32,Ke:u32,U7_:vec4<i32>,rg:vec2<f32>,ed:vec2<f32>,W1_:u32,vg:f32,Y5_:u32,P2_:f32,fd:f32,Ee:u32,y3_:f32,z3_:f32,gd:f32,og:u32,}@group(0)@binding(10) var QC:texture_2d<f32>;@group(3)@binding(10) var T9_:sampler;var<private>yg:f32;var<private>I_1:vec4<f32>;@group(0)@binding(0) var<uniform>k:NB;@group(0)@binding(9) var DD:texture_2d<f32>;@group(1)@binding(12) var AC:texture_2d<f32>;@group(3)@binding(9) var Bb:sampler;@group(1)@binding(14) var R5_:sampler;fn _b(){let _a=I_1;let _d=textureSampleLevel(QC,T9_,vec2<f32>((3f+_a.x),0f),0f);let _e=textureSampleLevel(QC,T9_,vec2<f32>((1f-_a.y),0f),0f);yg=((1f-_d.x)-_e.x);return;}@fragment fn main(@location(0) I:vec4<f32>)->@location(0) f32{I_1=I;_b();let _c=yg;return _c;})WGSL",
    .usedOverrides = {{false, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "render_atlas_stroke.webgpu_frag",
};
} // namespace wgsl
