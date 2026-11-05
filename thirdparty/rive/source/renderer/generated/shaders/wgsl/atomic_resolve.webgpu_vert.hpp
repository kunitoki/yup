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
struct RB{Od:f32,Ze:f32,dh:f32,eh:f32,X5_:u32,Ka:u32,Og:u32,Qg:u32,Q8_:vec4<i32>,Yi:vec2<f32>,af:vec2<f32>,z2_:u32,cj:f32,B6_:u32,m3_:f32,bf:f32,Hg:u32,J3_:f32,K3_:f32,cf:f32,Vi:u32,Ja:u32,X8_:f32,Y8_:f32,}struct _h{@builtin(position) _d:vec4<f32>,_v:f32,_x:array<f32,1>,_w:array<f32,1>,}struct rd{k2_:array<vec4<u32>>,}struct Fg{k2_:array<vec4<f32>>,}struct Ei{k2_:array<vec4<u32>>,}var<private>_f:i32;var<private>_r:i32;@group(0)@binding(0) var<uniform>j:RB;var<private>_e:_h=_h(vec4<f32>(0f,0f,0f,1f),1f,array<f32,1>(),array<f32,1>());@group(0)@binding(7) var WB:texture_2d<u32>;@group(0)@binding(9) var BD:texture_2d<f32>;@group(0)@binding(2) var<storage>JB:rd;@group(0)@binding(3) var<storage>KB:Fg;@group(0)@binding(4) var<storage>CD:Ei;@group(3)@binding(9) var kb:sampler;fn _u(){var _a:i32;var _b:i32;let _c=_f;if ((_c&1i)==0i){let _n=j.Q8_[0u];_a=_n;}else{let _p=j.Q8_[2u];_a=_p;}let _j=_a;if ((_c&2i)==0i){let _t=j.Q8_[1u];_b=_t;}else{let _k=j.Q8_[3u];_b=_k;}let _o=_b;let _i=vec2<f32>(vec2<i32>(_j,_o));let _l=j.dh;let _g=j.eh;_e._d=vec4<f32>(((_i.x*_l)-1f),((_i.y*_g)-sign(_g)),0f,1f);return;}@vertex fn main(@builtin(vertex_index) _s:u32,@builtin(instance_index) _m:u32)->@builtin(position) vec4<f32>{_f=i32(_s);_r=i32(_m);_u();let _q=_e._d;return _q;})WGSL",
    .usedOverrides = {{false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "atomic_resolve.webgpu_vert",
};
} // namespace wgsl
