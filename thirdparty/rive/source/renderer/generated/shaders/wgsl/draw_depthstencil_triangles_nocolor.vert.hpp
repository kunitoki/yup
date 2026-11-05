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
struct RB{Od:f32,Ze:f32,dh:f32,eh:f32,X5_:u32,Ka:u32,Og:u32,Qg:u32,Q8_:vec4<i32>,Yi:vec2<f32>,af:vec2<f32>,z2_:u32,cj:f32,B6_:u32,m3_:f32,bf:f32,Hg:u32,J3_:f32,K3_:f32,cf:f32,Vi:u32,Ja:u32,X8_:f32,Y8_:f32,}struct _b{@builtin(position) _e:vec4<f32>,_o:f32,_n:array<f32,1>,_m:array<f32,1>,}var<private>_g:i32;var<private>MB_1:vec3<f32>;@group(0)@binding(0) var<uniform>j:RB;var<private>_d:_b=_b(vec4<f32>(0f,0f,0f,1f),1f,array<f32,1>(),array<f32,1>());fn _k(){let _f=MB_1;let _j=j.dh;let _c=j.eh;let _a=vec4<f32>(((_f.x*_j)-1f),((_f.y*_c)-sign(_c)),0f,1f);let _l=MB_1[2u];_d._e=vec4<f32>(_a.x,_a.y,((f32((((bitcast<u32>(_l)&65535u)<<bitcast<u32>(8u))|255u))*0.000000059604645f)+0.000000029802322f),_a.w);return;}@vertex fn main(@builtin(vertex_index) _i:u32,@location(0) MB:vec3<f32>)->@builtin(position) vec4<f32>{_g=i32(_i);MB_1=MB;_k();let _h=_d._e;return _h;})WGSL",
    .usedOverrides = {{false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "draw_depthstencil_triangles_nocolor.vert",
};
} // namespace wgsl
