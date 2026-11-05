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

inline constexpr Shader draw_depthstencil_triangles_nocolor_webgpu_vert = {
    .source = R"WGSL(
struct RB{Od:f32,Ze:f32,dh:f32,eh:f32,X5_:u32,Ka:u32,Og:u32,Qg:u32,Q8_:vec4<i32>,Yi:vec2<f32>,af:vec2<f32>,z2_:u32,cj:f32,B6_:u32,m3_:f32,bf:f32,Hg:u32,J3_:f32,K3_:f32,cf:f32,Vi:u32,Ja:u32,X8_:f32,Y8_:f32,}struct _b{@builtin(position) _c:vec4<f32>,_o:f32,_m:array<f32,1>,_n:array<f32,1>,}var<private>_g:i32;var<private>MB_1:vec3<f32>;@group(0)@binding(0) var<uniform>j:RB;var<private>_f:_b=_b(vec4<f32>(0f,0f,0f,1f),1f,array<f32,1>(),array<f32,1>());fn _h(){let _d=MB_1;let _l=j.dh;let _e=j.eh;let _a=vec4<f32>(((_d.x*_l)-1f),((_d.y*_e)-sign(_e)),0f,1f);let _i=MB_1[2u];_f._c=vec4<f32>(_a.x,_a.y,((f32((((bitcast<u32>(_i)&65535u)<<bitcast<u32>(8u))|255u))*0.000000059604645f)+0.000000029802322f),_a.w);return;}@vertex fn main(@builtin(vertex_index) _k:u32,@location(0) MB:vec3<f32>)->@builtin(position) vec4<f32>{_g=i32(_k);MB_1=MB;_h();let _j=_f._c;return _j;})WGSL",
    .usedOverrides = {{false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "draw_depthstencil_triangles_nocolor.webgpu_vert",
};
} // namespace wgsl
