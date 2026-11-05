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

inline constexpr Shader draw_depthstencil_triangles_nocolor_frag = {
    .source = R"WGSL(
struct RB{Od:f32,Ze:f32,dh:f32,eh:f32,X5_:u32,Ka:u32,Og:u32,Qg:u32,Q8_:vec4<i32>,Yi:vec2<f32>,af:vec2<f32>,z2_:u32,cj:f32,B6_:u32,m3_:f32,bf:f32,Hg:u32,J3_:f32,K3_:f32,cf:f32,Vi:u32,Ja:u32,X8_:f32,Y8_:f32,}var<private>fj:vec4<f32>;@group(0)@binding(0) var<uniform>j:RB;fn _e(){let _b=vec4<f32>(0f,vec4<f32>().y,vec4<f32>().z,vec4<f32>().w);let _c=vec4<f32>(_b.x,0f,_b.z,_b.w);let _a=vec4<f32>(_c.x,_c.y,0f,_c.w);fj=vec4<f32>(_a.x,_a.y,_a.z,0f);return;}@fragment fn main()->@location(0) vec4<f32>{_e();let _d=fj;return _d;})WGSL",
    .usedOverrides = {{false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "draw_depthstencil_triangles_nocolor.frag",
};
} // namespace wgsl
