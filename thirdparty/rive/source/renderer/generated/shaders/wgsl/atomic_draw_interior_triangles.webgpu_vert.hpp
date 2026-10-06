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
struct mh{k2_:array<vec4<u32>>,}struct UB{Qc:f32,Td:f32,bg:f32,cg:f32,A6_:u32,X9_:u32,Nf:u32,Of:u32,j8_:vec4<i32>,Lh:vec2<f32>,Ud:vec2<f32>,j2_:u32,Ph:f32,T4_:u32,a3_:f32,Vd:f32,Hf:u32,L3_:f32,M3_:f32,Wd:f32,Ih:u32,W9_:u32,wc:f32,xc:f32,}struct _h{@builtin(position) _b:vec4<f32>,_w:f32,_u:array<f32,1>,_v:array<f32,1>,}struct Ef{k2_:array<vec2<u32>>,}struct Ff{k2_:array<vec4<f32>>,}struct nh{k2_:array<vec4<u32>>,}struct VertexOutput{@location(0)@interpolate(flat,either) member:f32,@location(1)@interpolate(flat,either) member_1:u32,@builtin(position) _b:vec4<f32>,}@group(0)@binding(2) var<storage>LB:mh;var<private>_t:i32;var<private>_m:i32;var<private>MB_1:vec3<f32>;var<private>m1_:f32;var<private>F0_:u32;@group(0)@binding(0) var<uniform>j:UB;var<private>_d:_h=_h(vec4<f32>(0f,0f,0f,1f),1f,array<f32,1>(),array<f32,1>());@group(0)@binding(7) var TB:texture_2d<u32>;@group(0)@binding(9) var YC:texture_2d<f32>;@group(0)@binding(3) var<storage>WC:Ef;@group(0)@binding(4) var<storage>JB:Ff;@group(0)@binding(5) var<storage>ZC:nh;@group(3)@binding(9) var wa:sampler;fn _p(){let _c=MB_1;let _f=(bitcast<u32>(_c.z)&65535u);let _g=(_f*4u);let _q=LB.k2_[_g];let _a=bitcast<vec4<f32>>(_q);let _j=LB.k2_[(_g+1u)];let _i=((mat2x2<f32>(vec2<f32>(_a.x,_a.y),vec2<f32>(_a.z,_a.w))*_c.xy)+bitcast<vec2<f32>>(_j.xy));m1_=f32((bitcast<i32>(_c.z)>>bitcast<u32>(16i)));F0_=_f;let _s=j.bg;let _e=j.cg;_d._b=vec4<f32>(((_i.x*_s)-1f),((_i.y*_e)-sign(_e)),0f,1f);return;}@vertex fn main(@builtin(vertex_index) _o:u32,@builtin(instance_index) _l:u32,@location(0) MB:vec3<f32>)->VertexOutput{_t=i32(_o);_m=i32(_l);MB_1=MB;_p();let _k=m1_;let _n=F0_;let _r=_d._b;return VertexOutput(_k,_n,_r);})WGSL",
    .usedOverrides = {{false, false, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "atomic_draw_interior_triangles.webgpu_vert",
};
} // namespace wgsl
