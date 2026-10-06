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

inline constexpr Shader atomic_resolve_webgpu_vert = {
    .source = R"WGSL(
struct UB{Qc:f32,Td:f32,bg:f32,cg:f32,A6_:u32,X9_:u32,Nf:u32,Of:u32,j8_:vec4<i32>,Lh:vec2<f32>,Ud:vec2<f32>,j2_:u32,Ph:f32,T4_:u32,a3_:f32,Vd:f32,Hf:u32,L3_:f32,M3_:f32,Wd:f32,Ih:u32,W9_:u32,wc:f32,xc:f32,}struct _g{@builtin(position) _e:vec4<f32>,_x:f32,_v:array<f32,1>,_w:array<f32,1>,}struct mh{k2_:array<vec4<u32>>,}struct Ef{k2_:array<vec2<u32>>,}struct Ff{k2_:array<vec4<f32>>,}struct nh{k2_:array<vec4<u32>>,}var<private>_f:i32;var<private>_n:i32;@group(0)@binding(0) var<uniform>j:UB;var<private>_i:_g=_g(vec4<f32>(0f,0f,0f,1f),1f,array<f32,1>(),array<f32,1>());@group(0)@binding(7) var TB:texture_2d<u32>;@group(0)@binding(9) var YC:texture_2d<f32>;@group(0)@binding(2) var<storage>LB:mh;@group(0)@binding(3) var<storage>WC:Ef;@group(0)@binding(4) var<storage>JB:Ff;@group(0)@binding(5) var<storage>ZC:nh;@group(3)@binding(9) var wa:sampler;fn _l(){var _b:i32;var _a:i32;let _d=_f;if ((_d&1i)==0i){let _u=j.j8_[0u];_b=_u;}else{let _j=j.j8_[2u];_b=_j;}let _s=_b;if ((_d&2i)==0i){let _p=j.j8_[1u];_a=_p;}else{let _o=j.j8_[3u];_a=_o;}let _q=_a;let _c=vec2<f32>(vec2<i32>(_s,_q));let _m=j.bg;let _h=j.cg;_i._e=vec4<f32>(((_c.x*_m)-1f),((_c.y*_h)-sign(_h)),0f,1f);return;}@vertex fn main(@builtin(vertex_index) _r:u32,@builtin(instance_index) _t:u32)->@builtin(position) vec4<f32>{_f=i32(_r);_n=i32(_t);_l();let _k=_i._e;return _k;})WGSL",
    .usedOverrides = {{false, false, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "atomic_resolve.webgpu_vert",
};
} // namespace wgsl
