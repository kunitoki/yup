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
struct NB{Ub:f32,dd:f32,Xe:f32,Ye:f32,q5_:u32,ug:u32,Je:u32,Ke:u32,U7_:vec4<i32>,rg:vec2<f32>,ed:vec2<f32>,W1_:u32,vg:f32,Y5_:u32,P2_:f32,fd:f32,Ee:u32,y3_:f32,z3_:f32,gd:f32,og:u32,}struct g0qd{X1_:array<u32>,}struct p4qd{X1_:array<u32>,}struct d0qd{X1_:array<u32>,}struct Be{X1_:array<vec2<u32>>,}struct Ce{X1_:array<vec4<f32>>,}@id(11) override Yg:bool=false;@id(12) override Zg:bool=false;@id(0) override Mg:bool=true;var<private>_d:vec4<f32>;@group(0)@binding(0) var<uniform>k:NB;@group(2)@binding(0) var<storage,read_write>g0_:g0qd;@group(1)@binding(12) var AC:texture_2d<f32>;@group(2)@binding(3) var<storage,read_write>p4_:p4qd;@group(2)@binding(1) var<storage,read_write>d0_:d0qd;@group(3)@binding(10) var T9_:sampler;@group(0)@binding(9) var DD:texture_2d<f32>;@group(0)@binding(10) var QC:texture_2d<f32>;@group(3)@binding(9) var Bb:sampler;@group(1)@binding(14) var R5_:sampler;@group(0)@binding(4) var<storage>TC:Be;@group(0)@binding(5) var<storage>PB:Ce;fn _e(){let _f=_d;let _c=vec2<i32>(floor(_f.xy));let _a=bitcast<vec2<u32>>(_c);let _g=k.q5_;let _b=bitcast<i32>((((((_a.y>>bitcast<u32>(5u))*(((_g+31u)&4294967264u)<<bitcast<u32>(5u)))+((_a.x>>bitcast<u32>(5u))<<bitcast<u32>(10u)))+(((_a.x&28u)<<bitcast<u32>(5u))+((_a.y&28u)<<bitcast<u32>(2i))))+(((_a.y&3u)<<bitcast<u32>(2i))+(_a.x&3u))));if Yg{let _h=k.Je;g0_.X1_[_b]=pack4x8unorm(unpack4x8unorm(_h));}if Zg{let _k=textureLoad(AC,_c,0i);g0_.X1_[_b]=pack4x8unorm(_k);}let _i=k.Ke;p4_.X1_[_b]=_i;if Mg{d0_.X1_[_b]=0u;}return;}@fragment fn main(@builtin(position) _j:vec4<f32>){_d=_j;_e();})WGSL",
    .usedOverrides = {{true, false, false, false, false, false, false, false, false, false, false, true, true, false}},
    .label = "atomic_init.webgpu_frag",
};
} // namespace wgsl
