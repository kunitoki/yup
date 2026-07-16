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

inline constexpr Shader draw_msaa_image_mesh_webgpu_fixedcolor_frag = {
    .source = R"WGSL(
struct NB{Ub:f32,dd:f32,Xe:f32,Ye:f32,q5_:u32,ug:u32,Je:u32,Ke:u32,U7_:vec4<i32>,rg:vec2<f32>,ed:vec2<f32>,W1_:u32,vg:f32,Y5_:u32,P2_:f32,fd:f32,Ee:u32,y3_:f32,z3_:f32,gd:f32,og:u32,}struct LC{r9_:vec4<f32>,c2_:vec2<f32>,x4_:f32,ki:f32,k2_:vec4<f32>,D2_:vec2<f32>,V0_:u32,n2_:u32,Z6_:u32,}@id(7) override Tg:bool=true;@group(1)@binding(12) var AC:texture_2d<f32>;@group(1)@binding(14) var R5_:sampler;var<private>U0_1:vec2<f32>;@group(0)@binding(0) var<uniform>k:NB;@group(0)@binding(2) var<uniform>A0_:LC;var<private>_f:vec4<f32>;var<private>yg:vec4<f32>;var<private>I3_1:f32;@group(0)@binding(13) var LD:texture_2d<f32>;fn _o(){var _b:vec3<f32>;let _q=U0_1;let _l=k.fd;let _m=textureSampleBias(AC,R5_,_q,_l);let _n=A0_.x4_;let _a=(_m*_n);let _h=_a.xyz;let _g=_f;let _k=k.y3_;let _i=k.z3_;if Tg{_b=(vec3(((fract((52.982918f*fract(((0.06711056f*_g.x)+(0.00583715f*_g.y)))))*_k)+_i))+_h);}else{_b=_h;}let _d=_b;let _e=vec4<f32>(_d.x,_a.y,_a.z,_a.w);let _c=vec4<f32>(_e.x,_d.y,_e.z,_e.w);yg=vec4<f32>(_c.x,_c.y,_d.z,_c.w);return;}@fragment fn main(@location(0) U0_:vec2<f32>,@builtin(position) _j:vec4<f32>,@location(1)@interpolate(flat) I3_:f32)->@location(0) vec4<f32>{U0_1=U0_;_f=_j;I3_1=I3_;_o();let _p=yg;return _p;})WGSL",
    .usedOverrides = {{false, false, false, false, false, false, false, true, false, false, false, false, false, false}},
    .label = "draw_msaa_image_mesh.webgpu_fixedcolor_frag",
};
} // namespace wgsl
