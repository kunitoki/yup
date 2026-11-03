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

inline constexpr Shader atomic_init_webgpu_fixedcolor_frag = {
    .source = R"WGSL(
struct RB{Od:f32,Ze:f32,dh:f32,eh:f32,X5_:u32,Ka:u32,Og:u32,Qg:u32,Q8_:vec4<i32>,Yi:vec2<f32>,af:vec2<f32>,z2_:u32,cj:f32,B6_:u32,m3_:f32,bf:f32,Hg:u32,J3_:f32,K3_:f32,cf:f32,Vi:u32,Ja:u32,X8_:f32,Y8_:f32,}struct e5lf{k2_:array<u32>,}struct o0lf{k2_:array<u32>,}struct rd{k2_:array<vec4<u32>>,}struct Fg{k2_:array<vec4<f32>>,}@id(0) override Bj:bool=true;var<private>_b:vec4<f32>;@group(0)@binding(0) var<uniform>j:RB;@group(2)@binding(3) var<storage,read_write>e5_:e5lf;@group(2)@binding(1) var<storage,read_write>o0_:o0lf;@group(3)@binding(9) var kb:sampler;@group(0)@binding(8) var AD:texture_2d<f32>;@group(0)@binding(9) var BD:texture_2d<f32>;@group(1)@binding(11) var SB:texture_2d<f32>;@group(3)@binding(8) var Z8_:sampler;@group(1)@binding(13) var x4_:sampler;@group(0)@binding(2) var<storage>JB:rd;@group(0)@binding(3) var<storage>KB:Fg;var<private>S1_:vec4<f32>;fn _g(){let _i=_b;let _a=bitcast<vec2<u32>>(vec2<i32>(floor(_i.xy)));let _h=j.X5_;let _c=bitcast<i32>((((((_a.y>>bitcast<u32>(5u))*(((_h+31u)&4294967264u)<<bitcast<u32>(5u)))+((_a.x>>bitcast<u32>(5u))<<bitcast<u32>(10u)))+(((_a.x&28u)<<bitcast<u32>(5u))+((_a.y&28u)<<bitcast<u32>(2i))))+(((_a.y&3u)<<bitcast<u32>(2i))+(_a.x&3u))));let _e=j.Qg;e5_.k2_[_c]=_e;if Bj{o0_.k2_[_c]=0u;}discard;}@fragment fn main(@builtin(position) _d:vec4<f32>)->@location(0) vec4<f32>{_b=_d;_g();let _f=S1_;return _f;})WGSL",
    .usedOverrides = {{true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "atomic_init.webgpu_fixedcolor_frag",
};
} // namespace wgsl
