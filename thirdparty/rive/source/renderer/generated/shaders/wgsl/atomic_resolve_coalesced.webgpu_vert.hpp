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

inline constexpr Shader atomic_resolve_coalesced_webgpu_vert = {
    .source = R"WGSL(
struct RB{Od:f32,Ze:f32,dh:f32,eh:f32,X5_:u32,Ka:u32,Og:u32,Qg:u32,Q8_:vec4<i32>,Yi:vec2<f32>,af:vec2<f32>,z2_:u32,cj:f32,B6_:u32,m3_:f32,bf:f32,Hg:u32,J3_:f32,K3_:f32,cf:f32,Vi:u32,Ja:u32,X8_:f32,Y8_:f32,}struct _f{@builtin(position) _i:vec4<f32>,_w:f32,_v:array<f32,1>,_x:array<f32,1>,}struct rd{k2_:array<vec4<u32>>,}struct Fg{k2_:array<vec4<f32>>,}struct Ei{k2_:array<vec4<u32>>,}var<private>_e:i32;var<private>_l:i32;@group(0)@binding(0) var<uniform>j:RB;var<private>_d:_f=_f(vec4<f32>(0f,0f,0f,1f),1f,array<f32,1>(),array<f32,1>());@group(0)@binding(7) var WB:texture_2d<u32>;@group(0)@binding(9) var BD:texture_2d<f32>;@group(0)@binding(2) var<storage>JB:rd;@group(0)@binding(3) var<storage>KB:Fg;@group(0)@binding(4) var<storage>CD:Ei;@group(3)@binding(9) var kb:sampler;fn _u(){var _b:i32;var _a:i32;let _c=_e;if ((_c&1i)==0i){let _p=j.Q8_[0u];_b=_p;}else{let _k=j.Q8_[2u];_b=_k;}let _s=_b;if ((_c&2i)==0i){let _o=j.Q8_[1u];_a=_o;}else{let _q=j.Q8_[3u];_a=_q;}let _r=_a;let _g=vec2<f32>(vec2<i32>(_s,_r));let _j=j.dh;let _h=j.eh;_d._i=vec4<f32>(((_g.x*_j)-1f),((_g.y*_h)-sign(_h)),0f,1f);return;}@vertex fn main(@builtin(vertex_index) _m:u32,@builtin(instance_index) _n:u32)->@builtin(position) vec4<f32>{_e=i32(_m);_l=i32(_n);_u();let _t=_d._i;return _t;})WGSL",
    .usedOverrides = {{false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "atomic_resolve_coalesced.webgpu_vert",
};
} // namespace wgsl
