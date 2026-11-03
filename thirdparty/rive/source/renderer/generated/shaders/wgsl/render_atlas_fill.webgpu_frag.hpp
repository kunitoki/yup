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

inline constexpr Shader render_atlas_fill_webgpu_frag = {
    .source = R"WGSL(
struct RB{Od:f32,Ze:f32,dh:f32,eh:f32,X5_:u32,Ka:u32,Og:u32,Qg:u32,Q8_:vec4<i32>,Yi:vec2<f32>,af:vec2<f32>,z2_:u32,cj:f32,B6_:u32,m3_:f32,bf:f32,Hg:u32,J3_:f32,K3_:f32,cf:f32,Vi:u32,Ja:u32,X8_:f32,Y8_:f32,}@group(0)@binding(9) var BD:texture_2d<f32>;@group(3)@binding(9) var kb:sampler;var<private>fj:f32;var<private>S_1:vec4<f32>;var<private>_g:bool;@group(0)@binding(0) var<uniform>j:RB;@group(0)@binding(8) var AD:texture_2d<f32>;@group(1)@binding(11) var SB:texture_2d<f32>;@group(3)@binding(8) var Z8_:sampler;@group(1)@binding(13) var x4_:sampler;fn _n(){var _d:f32;var _f:f32;var _c:f32;let _a=S_1;let _o=_g;let _e=max(_a.w,0f);if (_a.z>=0f){let _s=textureSampleLevel(BD,kb,vec2<f32>(_e,0f),0f);_d=_s.x;}else{_d=0f;}let _l=_d;_f=_l;if (abs(_a.z)<1000f){let _j=(-2f-_a.y);let _i=((_j-_e)*0.5984134f);let _h=(vec4(_e)+(vec4<f32>(0.20888568f,0.62665707f,1.0444285f,1.4621998f)*_i));let _b=((_h*-(_a.z))+vec4(((_j*_a.z)+(abs(_a.x)-0.25f))));let _q=textureSampleLevel(BD,kb,vec2<f32>(_b.x,0f),0f);let _t=textureSampleLevel(BD,kb,vec2<f32>(_b.y,0f),0f);let _w=textureSampleLevel(BD,kb,vec2<f32>(_b.z,0f),0f);let _p=textureSampleLevel(BD,kb,vec2<f32>(_b.w,0f),0f);let _m=(_h*5.0959306f);_f=(_l+(dot(vec4<f32>(_q.x,_t.x,_w.x,_p.x),exp2(((vec4<f32>(2.5479653f,2.5479653f,2.5479653f,2.5479653f)-_m)*(_m+vec4<f32>(-2.5479653f,-2.5479653f,-2.5479653f,-2.5479653f)))))*_i));}let _v=_f;let _k=(_v*sign(_a.x));_c=_k;if!(_o){_c=-(_k);}let _r=_c;fj=_r;return;}@fragment fn main(@location(0) S:vec4<f32>,@builtin(front_facing) _x:bool)->@location(0) f32{S_1=S;_g=_x;_n();let _u=fj;return _u;})WGSL",
    .usedOverrides = {{false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "render_atlas_fill.webgpu_frag",
};
} // namespace wgsl
