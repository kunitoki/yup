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

inline constexpr Shader atomic_draw_atlas_blit_webgpu_vert = {
    .source = R"WGSL(
struct mh{k2_:array<vec4<u32>>,}struct UB{Qc:f32,Td:f32,bg:f32,cg:f32,A6_:u32,X9_:u32,Nf:u32,Of:u32,j8_:vec4<i32>,Lh:vec2<f32>,Ud:vec2<f32>,j2_:u32,Ph:f32,T4_:u32,a3_:f32,Vd:f32,Hf:u32,L3_:f32,M3_:f32,Wd:f32,Ih:u32,W9_:u32,wc:f32,xc:f32,}struct _d{@builtin(position) _b:vec4<f32>,_t:f32,_s:array<f32,1>,_u:array<f32,1>,}struct Ef{k2_:array<vec2<u32>>,}struct Ff{k2_:array<vec4<f32>>,}struct nh{k2_:array<vec4<u32>>,}struct VertexOutput{@location(0) member:vec2<f32>,@location(1)@interpolate(flat,either) member_1:u32,@builtin(position) _b:vec4<f32>,}@group(0)@binding(2) var<storage>LB:mh;@group(0)@binding(0) var<uniform>j:UB;var<private>_p:i32;var<private>_q:i32;var<private>MB_1:vec3<f32>;var<private>J2_:vec2<f32>;var<private>F0_:u32;var<private>_g:_d=_d(vec4<f32>(0f,0f,0f,1f),1f,array<f32,1>(),array<f32,1>());@group(0)@binding(7) var TB:texture_2d<u32>;@group(0)@binding(9) var YC:texture_2d<f32>;@group(0)@binding(3) var<storage>WC:Ef;@group(0)@binding(4) var<storage>JB:Ff;@group(0)@binding(5) var<storage>ZC:nh;@group(3)@binding(9) var wa:sampler;fn _i(){let _a=MB_1;let _e=(bitcast<u32>(_a.z)&65535u);let _l=LB.k2_[((_e*4u)+2u)];let _c=bitcast<vec3<f32>>(_l.yzw);let _m=j.Lh;J2_=(((_a.xy*_c.x)+_c.yz)*_m);F0_=_e;let _o=j.bg;let _f=j.cg;_g._b=vec4<f32>(((_a.x*_o)-1f),((_a.y*_f)-sign(_f)),0f,1f);return;}@vertex fn main(@builtin(vertex_index) _j:u32,@builtin(instance_index) _r:u32,@location(0) MB:vec3<f32>)->VertexOutput{_p=i32(_j);_q=i32(_r);MB_1=MB;_i();let _h=J2_;let _k=F0_;let _n=_g._b;return VertexOutput(_h,_k,_n);})WGSL",
    .usedOverrides = {{false, false, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "atomic_draw_atlas_blit.webgpu_vert",
};
} // namespace wgsl
