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

inline constexpr Shader draw_depthstencil_image_mesh_webgpu_fixedcolor_frag = {
    .source = R"WGSL(
struct RB{Od:f32,Ze:f32,dh:f32,eh:f32,X5_:u32,Ka:u32,Og:u32,Qg:u32,Q8_:vec4<i32>,Yi:vec2<f32>,af:vec2<f32>,z2_:u32,cj:f32,B6_:u32,m3_:f32,bf:f32,Hg:u32,J3_:f32,K3_:f32,cf:f32,Vi:u32,Ja:u32,X8_:f32,Y8_:f32,}@id(8) override Jj:bool=true;@group(1)@binding(11) var SB:texture_2d<f32>;@group(1)@binding(13) var x4_:sampler;var<private>j6_1:vec2<f32>;@group(0)@binding(0) var<uniform>j:RB;var<private>a2_1:vec4<f32>;var<private>_f:vec4<f32>;var<private>fj:vec4<f32>;var<private>j4_1:f32;var<private>Q1_1:u32;fn _m(){var _d:vec3<f32>;let _q=j6_1;let _l=j.bf;let _o=textureSampleBias(SB,x4_,_q,_l);let _n=a2_1;let _a=(_o*_n);let _h=_a.xyz;let _g=_f;let _j=j.J3_;let _k=j.K3_;if (Jj&&(_a.w!=0f)){_d=(vec3(((fract((52.982918f*fract(((0.06711056f*_g.x)+(0.00583715f*_g.y)))))*_j)+_k))+_h);}else{_d=_h;}let _b=_d;let _c=vec4<f32>(_b.x,_a.y,_a.z,_a.w);let _e=vec4<f32>(_c.x,_b.y,_c.z,_c.w);fj=vec4<f32>(_e.x,_e.y,_b.z,_e.w);return;}@fragment fn main(@location(0) j6_:vec2<f32>,@location(3)@interpolate(flat,either) a2_:vec4<f32>,@builtin(position) _p:vec4<f32>,@location(1)@interpolate(flat,either) j4_:f32,@location(4)@interpolate(flat,either) Q1_:u32)->@location(0) vec4<f32>{j6_1=j6_;a2_1=a2_;_f=_p;j4_1=j4_;Q1_1=Q1_;_m();let _i=fj;return _i;})WGSL",
    .usedOverrides = {{false, false, false, false, false, false, false, false, true, false, false, false, false, false, false, false, false, false}},
    .label = "draw_depthstencil_image_mesh.webgpu_fixedcolor_frag",
};
} // namespace wgsl
