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

inline constexpr Shader draw_msaa_stencil_webgpu_noclipdistance_vert = {
    .source = R"WGSL(
struct NB{Ub:f32,dd:f32,Xe:f32,Ye:f32,q5_:u32,ug:u32,Je:u32,Ke:u32,U7_:vec4<i32>,rg:vec2<f32>,ed:vec2<f32>,W1_:u32,vg:f32,Y5_:u32,P2_:f32,fd:f32,Ee:u32,y3_:f32,z3_:f32,gd:f32,og:u32,}struct _f{@builtin(position) _c:vec4<f32>,_m:f32,_o:array<f32,1>,_n:array<f32,1>,}var<private>_k:i32;var<private>KB_1:vec3<f32>;@group(0)@binding(0) var<uniform>k:NB;var<private>_d:_f=_f(vec4<f32>(0f,0f,0f,1f),1f,array<f32,1>(),array<f32,1>());fn _j(){let _e=KB_1;let _h=k.Xe;let _b=k.Ye;let _a=vec4<f32>(((_e.x*_h)-1f),((_e.y*_b)-sign(_b)),0f,1f);let _g=KB_1[2u];_d._c=vec4<f32>(_a.x,_a.y,(1f-(f32((bitcast<u32>(_g)&65535u))*0.000061035156f)),_a.w);return;}@vertex fn main(@builtin(vertex_index) _l:u32,@location(0) KB:vec3<f32>)->@builtin(position) vec4<f32>{_k=i32(_l);KB_1=KB;_j();let _i=_d._c;return _i;})WGSL",
    .usedOverrides = {{false, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "draw_msaa_stencil.webgpu_noclipdistance_vert",
};
} // namespace wgsl
