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
struct rd{k2_:array<vec4<u32>>,}struct RB{Od:f32,Ze:f32,dh:f32,eh:f32,X5_:u32,Ka:u32,Og:u32,Qg:u32,Q8_:vec4<i32>,Yi:vec2<f32>,af:vec2<f32>,z2_:u32,cj:f32,B6_:u32,m3_:f32,bf:f32,Hg:u32,J3_:f32,K3_:f32,cf:f32,Vi:u32,Ja:u32,X8_:f32,Y8_:f32,}struct _e{@builtin(position) _b:vec4<f32>,_s:f32,_t:array<f32,1>,_u:array<f32,1>,}struct Fg{k2_:array<vec4<f32>>,}struct Ei{k2_:array<vec4<u32>>,}struct VertexOutput{@location(0) member:vec2<f32>,@location(1)@interpolate(flat,either) member_1:u32,@builtin(position) _b:vec4<f32>,}@group(0)@binding(2) var<storage>JB:rd;@group(0)@binding(0) var<uniform>j:RB;var<private>_q:i32;var<private>_h:i32;var<private>MB_1:vec3<f32>;var<private>X2_:vec2<f32>;var<private>J0_:u32;var<private>_g:_e=_e(vec4<f32>(0f,0f,0f,1f),1f,array<f32,1>(),array<f32,1>());@group(0)@binding(7) var WB:texture_2d<u32>;@group(0)@binding(9) var BD:texture_2d<f32>;@group(0)@binding(3) var<storage>KB:Fg;@group(0)@binding(4) var<storage>CD:Ei;@group(3)@binding(9) var kb:sampler;fn _i(){let _a=MB_1;let _d=(bitcast<u32>(_a.z)&65535u);let _l=JB.k2_[((_d*4u)+3u)];let _c=bitcast<vec3<f32>>(_l.xyz);let _m=j.Yi;X2_=(((_a.xy*_c.x)+_c.yz)*_m);J0_=_d;let _o=j.dh;let _f=j.eh;_g._b=vec4<f32>(((_a.x*_o)-1f),((_a.y*_f)-sign(_f)),0f,1f);return;}@vertex fn main(@builtin(vertex_index) _n:u32,@builtin(instance_index) _p:u32,@location(0) MB:vec3<f32>)->VertexOutput{_q=i32(_n);_h=i32(_p);MB_1=MB;_i();let _r=X2_;let _k=J0_;let _j=_g._b;return VertexOutput(_r,_k,_j);})WGSL",
    .usedOverrides = {{false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "atomic_draw_atlas_blit.webgpu_vert",
};
} // namespace wgsl
