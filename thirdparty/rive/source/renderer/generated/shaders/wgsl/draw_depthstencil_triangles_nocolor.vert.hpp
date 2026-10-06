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

inline constexpr Shader draw_depthstencil_triangles_nocolor_vert = {
    .source = R"WGSL(
struct UB{Qc:f32,Td:f32,bg:f32,cg:f32,A6_:u32,X9_:u32,Nf:u32,Of:u32,j8_:vec4<i32>,Lh:vec2<f32>,Ud:vec2<f32>,j2_:u32,Ph:f32,T4_:u32,a3_:f32,Vd:f32,Hf:u32,L3_:f32,M3_:f32,Wd:f32,Ih:u32,W9_:u32,wc:f32,xc:f32,}struct _b{@builtin(position) _d:vec4<f32>,_o:f32,_n:array<f32,1>,_m:array<f32,1>,}var<private>_i:i32;var<private>MB_1:vec3<f32>;@group(0)@binding(0) var<uniform>j:UB;var<private>_f:_b=_b(vec4<f32>(0f,0f,0f,1f),1f,array<f32,1>(),array<f32,1>());fn _k(){let _e=MB_1;let _g=j.bg;let _c=j.cg;let _a=vec4<f32>(((_e.x*_g)-1f),((_e.y*_c)-sign(_c)),0f,1f);let _h=MB_1[2u];_f._d=vec4<f32>(_a.x,_a.y,((f32((((bitcast<u32>(_h)&65535u)<<bitcast<u32>(8u))|255u))*0.000000059604645f)+0.000000029802322f),_a.w);return;}@vertex fn main(@builtin(vertex_index) _j:u32,@location(0) MB:vec3<f32>)->@builtin(position) vec4<f32>{_i=i32(_j);MB_1=MB;_k();let _l=_f._d;return _l;})WGSL",
    .usedOverrides = {{false, false, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "draw_depthstencil_triangles_nocolor.vert",
};
} // namespace wgsl
