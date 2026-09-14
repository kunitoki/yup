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

inline constexpr Shader draw_msaa_stencil_frag = {
    .source = R"WGSL(
struct NB{Ub:f32,dd:f32,Xe:f32,Ye:f32,q5_:u32,ug:u32,Je:u32,Ke:u32,U7_:vec4<i32>,rg:vec2<f32>,ed:vec2<f32>,W1_:u32,vg:f32,Y5_:u32,P2_:f32,fd:f32,Ee:u32,y3_:f32,z3_:f32,gd:f32,og:u32,}var<private>yg:vec4<f32>;@group(0)@binding(0) var<uniform>k:NB;fn _d(){let _c=vec4<f32>(0f,vec4<f32>().y,vec4<f32>().z,vec4<f32>().w);let _b=vec4<f32>(_c.x,0f,_c.z,_c.w);let _a=vec4<f32>(_b.x,_b.y,0f,_b.w);yg=vec4<f32>(_a.x,_a.y,_a.z,0f);return;}@fragment fn main()->@location(0) vec4<f32>{_d();let _e=yg;return _e;})WGSL",
    .usedOverrides = {{false, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "draw_msaa_stencil.frag",
};
} // namespace wgsl
