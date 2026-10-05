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
struct UB{Qc:f32,Td:f32,bg:f32,cg:f32,A6_:u32,X9_:u32,Nf:u32,Of:u32,j8_:vec4<i32>,Lh:vec2<f32>,Ud:vec2<f32>,j2_:u32,Ph:f32,T4_:u32,a3_:f32,Vd:f32,Hf:u32,L3_:f32,M3_:f32,Wd:f32,Ih:u32,W9_:u32,wc:f32,xc:f32,}@group(0)@binding(9) var YC:texture_2d<f32>;@group(3)@binding(9) var wa:sampler;var<private>Sh:f32;var<private>S_1:vec4<f32>;var<private>_k:bool;@group(0)@binding(0) var<uniform>j:UB;@group(0)@binding(8) var ED:texture_2d<f32>;@group(1)@binding(11) var CC:texture_2d<f32>;@group(3)@binding(8) var ha:sampler;@group(1)@binding(13) var r5_:sampler;fn _x(){var _e:f32;var _d:f32;var _c:f32;let _a=S_1;let _p=_k;let _f=max(_a.w,0f);if (_a.z>=0f){let _w=textureSampleLevel(YC,wa,vec2<f32>(_f,0f),0f);_e=_w.x;}else{_e=0f;}let _l=_e;_d=_l;if (abs(_a.z)<1000f){let _g=(-2f-_a.y);let _j=((_g-_f)*0.5984134f);let _m=(vec4(_f)+(vec4<f32>(0.20888568f,0.62665707f,1.0444285f,1.4621998f)*_j));let _b=((_m*-(_a.z))+vec4(((_g*_a.z)+(abs(_a.x)-0.25f))));let _s=textureSampleLevel(YC,wa,vec2<f32>(_b.x,0f),0f);let _u=textureSampleLevel(YC,wa,vec2<f32>(_b.y,0f),0f);let _o=textureSampleLevel(YC,wa,vec2<f32>(_b.z,0f),0f);let _n=textureSampleLevel(YC,wa,vec2<f32>(_b.w,0f),0f);let _i=(_m*5.0959306f);_d=(_l+(dot(vec4<f32>(_s.x,_u.x,_o.x,_n.x),exp2(((vec4<f32>(2.5479653f,2.5479653f,2.5479653f,2.5479653f)-_i)*(_i+vec4<f32>(-2.5479653f,-2.5479653f,-2.5479653f,-2.5479653f)))))*_j));}let _q=_d;let _h=(_q*sign(_a.x));_c=_h;if!(_p){_c=-(_h);}let _v=_c;Sh=_v;return;}@fragment fn main(@location(0) S:vec4<f32>,@builtin(front_facing) _r:bool)->@location(0) f32{S_1=S;_k=_r;_x();let _t=Sh;return _t;})WGSL",
    .usedOverrides = {{false, false, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "render_atlas_fill.webgpu_frag",
};
} // namespace wgsl
