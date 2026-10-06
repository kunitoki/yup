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
struct UB{Qc:f32,Td:f32,bg:f32,cg:f32,A6_:u32,X9_:u32,Nf:u32,Of:u32,j8_:vec4<i32>,Lh:vec2<f32>,Ud:vec2<f32>,j2_:u32,Ph:f32,T4_:u32,a3_:f32,Vd:f32,Hf:u32,L3_:f32,M3_:f32,Wd:f32,Ih:u32,W9_:u32,wc:f32,xc:f32,}@id(7) override ri:bool=true;@group(1)@binding(11) var CC:texture_2d<f32>;@group(1)@binding(13) var r5_:sampler;var<private>V5_1:vec2<f32>;@group(0)@binding(0) var<uniform>j:UB;var<private>Q1_1:vec4<f32>;var<private>_f:vec4<f32>;var<private>Sh:vec4<f32>;var<private>Y3_1:f32;var<private>H1_1:u32;@group(0)@binding(12) var XD:texture_2d<f32>;fn _o(){var _c:vec3<f32>;let _n=V5_1;let _k=j.Vd;let _i=textureSampleBias(CC,r5_,_n,_k);let _p=Q1_1;let _a=(_i*_p);let _h=_a.xyz;let _g=_f;let _l=j.L3_;let _q=j.M3_;if (ri&&(_a.w!=0f)){_c=(vec3(((fract((52.982918f*fract(((0.06711056f*_g.x)+(0.00583715f*_g.y)))))*_l)+_q))+_h);}else{_c=_h;}let _d=_c;let _b=vec4<f32>(_d.x,_a.y,_a.z,_a.w);let _e=vec4<f32>(_b.x,_d.y,_b.z,_b.w);Sh=vec4<f32>(_e.x,_e.y,_d.z,_e.w);return;}@fragment fn main(@location(0) V5_:vec2<f32>,@location(3)@interpolate(flat,either) Q1_:vec4<f32>,@builtin(position) _m:vec4<f32>,@location(1)@interpolate(flat,either) Y3_:f32,@location(4)@interpolate(flat,either) H1_:u32)->@location(0) vec4<f32>{V5_1=V5_;Q1_1=Q1_;_f=_m;Y3_1=Y3_;H1_1=H1_;_o();let _j=Sh;return _j;})WGSL",
    .usedOverrides = {{false, false, false, false, false, false, false, true, false, false, false, false, false, false, false}},
    .label = "draw_depthstencil_image_mesh.webgpu_fixedcolor_frag",
};
} // namespace wgsl
