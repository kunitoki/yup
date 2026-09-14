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

inline constexpr Shader atomic_init_webgpu_vert = {
    .source = R"WGSL(
struct NB{Ub:f32,dd:f32,Xe:f32,Ye:f32,q5_:u32,ug:u32,Je:u32,Ke:u32,U7_:vec4<i32>,rg:vec2<f32>,ed:vec2<f32>,W1_:u32,vg:f32,Y5_:u32,P2_:f32,fd:f32,Ee:u32,y3_:f32,z3_:f32,gd:f32,og:u32,}struct _e{@builtin(position) _h:vec4<f32>,_v:f32,_x:array<f32,1>,_w:array<f32,1>,}struct Rf{X1_:array<vec4<u32>>,}struct Be{X1_:array<vec2<u32>>,}struct Ce{X1_:array<vec4<f32>>,}struct Sf{X1_:array<vec4<u32>>,}var<private>_d:i32;var<private>_s:i32;@group(0)@binding(0) var<uniform>k:NB;var<private>_c:_e=_e(vec4<f32>(0f,0f,0f,1f),1f,array<f32,1>(),array<f32,1>());@group(0)@binding(8) var DC:texture_2d<u32>;@group(0)@binding(10) var QC:texture_2d<f32>;@group(0)@binding(3) var<storage>MB:Rf;@group(0)@binding(4) var<storage>TC:Be;@group(0)@binding(5) var<storage>PB:Ce;@group(0)@binding(6) var<storage>XC:Sf;@group(3)@binding(10) var T9_:sampler;fn _n(){var _b:i32;var _a:i32;let _f=_d;if ((_f&1i)==0i){let _t=k.U7_[0u];_b=_t;}else{let _m=k.U7_[2u];_b=_m;}let _q=_b;if ((_f&2i)==0i){let _r=k.U7_[1u];_a=_r;}else{let _l=k.U7_[3u];_a=_l;}let _p=_a;let _g=vec2<f32>(vec2<i32>(_q,_p));let _k=k.Xe;let _i=k.Ye;_c._h=vec4<f32>(((_g.x*_k)-1f),((_g.y*_i)-sign(_i)),0f,1f);return;}@vertex fn main(@builtin(vertex_index) _o:u32,@builtin(instance_index) _u:u32)->@builtin(position) vec4<f32>{_d=i32(_o);_s=i32(_u);_n();let _j=_c._h;return _j;})WGSL",
    .usedOverrides = {{false, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "atomic_init.webgpu_vert",
};
} // namespace wgsl
