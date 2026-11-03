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

inline constexpr Shader atomic_draw_interior_triangles_webgpu_vert = {
    .source = R"WGSL(
struct rd{k2_:array<vec4<u32>>,}struct RB{Od:f32,Ze:f32,dh:f32,eh:f32,X5_:u32,Ka:u32,Og:u32,Qg:u32,Q8_:vec4<i32>,Yi:vec2<f32>,af:vec2<f32>,z2_:u32,cj:f32,B6_:u32,m3_:f32,bf:f32,Hg:u32,J3_:f32,K3_:f32,cf:f32,Vi:u32,Ja:u32,X8_:f32,Y8_:f32,}struct _g{@builtin(position) _b:vec4<f32>,_u:f32,_v:array<f32,1>,_w:array<f32,1>,}struct Fg{k2_:array<vec4<f32>>,}struct Ei{k2_:array<vec4<u32>>,}struct VertexOutput{@location(0)@interpolate(flat,either) member:f32,@location(1)@interpolate(flat,either) member_1:u32,@builtin(position) _b:vec4<f32>,}@group(0)@binding(2) var<storage>JB:rd;var<private>_m:i32;var<private>_s:i32;var<private>MB_1:vec3<f32>;var<private>v1_:f32;var<private>J0_:u32;@group(0)@binding(0) var<uniform>j:RB;var<private>_h:_g=_g(vec4<f32>(0f,0f,0f,1f),1f,array<f32,1>(),array<f32,1>());@group(0)@binding(7) var WB:texture_2d<u32>;@group(0)@binding(9) var BD:texture_2d<f32>;@group(0)@binding(3) var<storage>KB:Fg;@group(0)@binding(4) var<storage>CD:Ei;@group(3)@binding(9) var kb:sampler;fn _q(){let _c=MB_1;let _d=(bitcast<u32>(_c.z)&65535u);let _e=(_d*4u);let _p=JB.k2_[(_e+1u)];let _k=JB.k2_[_e];let _a=bitcast<vec4<f32>>(_k);let _i=((mat2x2<f32>(vec2<f32>(_a.x,_a.y),vec2<f32>(_a.z,_a.w))*_c.xy)+bitcast<vec2<f32>>(_p.xy));v1_=f32((bitcast<i32>(_c.z)>>bitcast<u32>(16i)));J0_=_d;let _n=j.dh;let _f=j.eh;_h._b=vec4<f32>(((_i.x*_n)-1f),((_i.y*_f)-sign(_f)),0f,1f);return;}@vertex fn main(@builtin(vertex_index) _j:u32,@builtin(instance_index) _o:u32,@location(0) MB:vec3<f32>)->VertexOutput{_m=i32(_j);_s=i32(_o);MB_1=MB;_q();let _l=v1_;let _r=J0_;let _t=_h._b;return VertexOutput(_l,_r,_t);})WGSL",
    .usedOverrides = {{false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "atomic_draw_interior_triangles.webgpu_vert",
};
} // namespace wgsl
