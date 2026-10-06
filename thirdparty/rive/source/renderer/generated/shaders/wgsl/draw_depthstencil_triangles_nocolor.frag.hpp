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
struct UB{Qc:f32,Td:f32,bg:f32,cg:f32,A6_:u32,X9_:u32,Nf:u32,Of:u32,j8_:vec4<i32>,Lh:vec2<f32>,Ud:vec2<f32>,j2_:u32,Ph:f32,T4_:u32,a3_:f32,Vd:f32,Hf:u32,L3_:f32,M3_:f32,Wd:f32,Ih:u32,W9_:u32,wc:f32,xc:f32,}var<private>Sh:vec4<f32>;@group(0)@binding(0) var<uniform>j:UB;fn _e(){let _b=vec4<f32>(0f,vec4<f32>().y,vec4<f32>().z,vec4<f32>().w);let _a=vec4<f32>(_b.x,0f,_b.z,_b.w);let _c=vec4<f32>(_a.x,_a.y,0f,_a.w);Sh=vec4<f32>(_c.x,_c.y,_c.z,0f);return;}@fragment fn main()->@location(0) vec4<f32>{_e();let _d=Sh;return _d;})WGSL",
    .usedOverrides = {{false, false, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "draw_depthstencil_triangles_nocolor.frag",
};
} // namespace wgsl
