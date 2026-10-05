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

inline constexpr Shader atomic_init_webgpu_frag = {
    .source = R"WGSL(
struct UB{Qc:f32,Td:f32,bg:f32,cg:f32,A6_:u32,X9_:u32,Nf:u32,Of:u32,j8_:vec4<i32>,Lh:vec2<f32>,Ud:vec2<f32>,j2_:u32,Ph:f32,T4_:u32,a3_:f32,Vd:f32,Hf:u32,L3_:f32,M3_:f32,Wd:f32,Ih:u32,W9_:u32,wc:f32,xc:f32,}struct n0ge{k2_:array<u32>,}struct K4ge{k2_:array<u32>,}struct m0ge{k2_:array<u32>,}struct Ef{k2_:array<vec2<u32>>,}struct Ff{k2_:array<vec4<f32>>,}@id(12) override wi:bool=false;@id(13) override xi:bool=false;@id(0) override ki:bool=true;var<private>_d:vec4<f32>;@group(0)@binding(0) var<uniform>j:UB;@group(2)@binding(0) var<storage,read_write>n0_:n0ge;@group(1)@binding(11) var CC:texture_2d<f32>;@group(2)@binding(3) var<storage,read_write>K4_:K4ge;@group(2)@binding(1) var<storage,read_write>m0_:m0ge;@group(3)@binding(9) var wa:sampler;@group(0)@binding(8) var ED:texture_2d<f32>;@group(0)@binding(9) var YC:texture_2d<f32>;@group(3)@binding(8) var ha:sampler;@group(1)@binding(13) var r5_:sampler;@group(0)@binding(3) var<storage>WC:Ef;@group(0)@binding(4) var<storage>JB:Ff;fn _j(){let _h=_d;let _c=vec2<i32>(floor(_h.xy));let _a=bitcast<vec2<u32>>(_c);let _k=j.A6_;let _b=bitcast<i32>((((((_a.y>>bitcast<u32>(5u))*(((_k+31u)&4294967264u)<<bitcast<u32>(5u)))+((_a.x>>bitcast<u32>(5u))<<bitcast<u32>(10u)))+(((_a.x&28u)<<bitcast<u32>(5u))+((_a.y&28u)<<bitcast<u32>(2i))))+(((_a.y&3u)<<bitcast<u32>(2i))+(_a.x&3u))));if wi{let _e=j.Nf;n0_.k2_[_b]=pack4x8unorm(unpack4x8unorm(_e));}if xi{let _f=textureLoad(CC,_c,0i);n0_.k2_[_b]=pack4x8unorm(_f);}let _g=j.Of;K4_.k2_[_b]=_g;if ki{m0_.k2_[_b]=0u;}return;}@fragment fn main(@builtin(position) _i:vec4<f32>){_d=_i;_j();})WGSL",
    .usedOverrides = {{true, false, false, false, false, false, false, false, false, false, false, false, true, true, false}},
    .label = "atomic_init.webgpu_frag",
};
} // namespace wgsl
