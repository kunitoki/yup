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
struct NB{Ub:f32,dd:f32,Xe:f32,Ye:f32,q5_:u32,ug:u32,Je:u32,Ke:u32,U7_:vec4<i32>,rg:vec2<f32>,ed:vec2<f32>,W1_:u32,vg:f32,Y5_:u32,P2_:f32,fd:f32,Ee:u32,y3_:f32,z3_:f32,gd:f32,og:u32,}@group(0)@binding(10) var QC:texture_2d<f32>;@group(3)@binding(10) var T9_:sampler;var<private>yg:f32;var<private>I_1:vec4<f32>;var<private>_k:bool;@group(0)@binding(0) var<uniform>k:NB;@group(0)@binding(9) var DD:texture_2d<f32>;@group(1)@binding(12) var AC:texture_2d<f32>;@group(3)@binding(9) var Bb:sampler;@group(1)@binding(14) var R5_:sampler;fn _w(){var _f:f32;var _d:f32;var _c:f32;let _a=I_1;let _x=_k;let _e=max(_a.w,0f);if (_a.z>=0f){let _s=textureSampleLevel(QC,T9_,vec2<f32>(_e,0f),0f);_f=_s.x;}else{_f=0f;}let _l=_f;_d=_l;if (abs(_a.z)<1000f){let _h=(-2f-_a.y);let _g=((_h-_e)*0.5984134f);let _m=(vec4(_e)+(vec4<f32>(0.20888568f,0.62665707f,1.0444285f,1.4621998f)*_g));let _b=((_m*-(_a.z))+vec4(((_h*_a.z)+(abs(_a.x)-0.25f))));let _n=textureSampleLevel(QC,T9_,vec2<f32>(_b.x,0f),0f);let _v=textureSampleLevel(QC,T9_,vec2<f32>(_b.y,0f),0f);let _o=textureSampleLevel(QC,T9_,vec2<f32>(_b.z,0f),0f);let _u=textureSampleLevel(QC,T9_,vec2<f32>(_b.w,0f),0f);let _i=(_m*5.0959306f);_d=(_l+(dot(vec4<f32>(_n.x,_v.x,_o.x,_u.x),exp2(((vec4<f32>(2.5479653f,2.5479653f,2.5479653f,2.5479653f)-_i)*(_i+vec4<f32>(-2.5479653f,-2.5479653f,-2.5479653f,-2.5479653f)))))*_g));}let _q=_d;let _j=(_q*sign(_a.x));_c=_j;if!(_x){_c=-(_j);}let _t=_c;yg=_t;return;}@fragment fn main(@location(0) I:vec4<f32>,@builtin(front_facing) _r:bool)->@location(0) f32{I_1=I;_k=_r;_w();let _p=yg;return _p;})WGSL",
    .usedOverrides = {{false, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "render_atlas_fill.webgpu_frag",
};
} // namespace wgsl
