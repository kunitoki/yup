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

inline constexpr Shader draw_depthstencil_triangles_nocolor_webgpu_noclipdistance_vert = {
    .source = R"WGSL(
struct UB{Qc:f32,Td:f32,bg:f32,cg:f32,A6_:u32,X9_:u32,Nf:u32,Of:u32,j8_:vec4<i32>,Lh:vec2<f32>,Ud:vec2<f32>,j2_:u32,Ph:f32,T4_:u32,a3_:f32,Vd:f32,Hf:u32,L3_:f32,M3_:f32,Wd:f32,Ih:u32,W9_:u32,wc:f32,xc:f32,}struct _d{@builtin(position) _e:vec4<f32>,_m:f32,_n:array<f32,1>,_o:array<f32,1>,}var<private>_i:i32;var<private>MB_1:vec3<f32>;@group(0)@binding(0) var<uniform>j:UB;var<private>_f:_d=_d(vec4<f32>(0f,0f,0f,1f),1f,array<f32,1>(),array<f32,1>());fn _k(){let _c=MB_1;let _j=j.bg;let _b=j.cg;let _a=vec4<f32>(((_c.x*_j)-1f),((_c.y*_b)-sign(_b)),0f,1f);let _g=MB_1[2u];_f._e=vec4<f32>(_a.x,_a.y,((f32((((bitcast<u32>(_g)&65535u)<<bitcast<u32>(8u))|255u))*0.000000059604645f)+0.000000029802322f),_a.w);return;}@vertex fn main(@builtin(vertex_index) _l:u32,@location(0) MB:vec3<f32>)->@builtin(position) vec4<f32>{_i=i32(_l);MB_1=MB;_k();let _h=_f._e;return _h;})WGSL",
    .usedOverrides = {{false, false, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "draw_depthstencil_triangles_nocolor.webgpu_noclipdistance_vert",
};
} // namespace wgsl
