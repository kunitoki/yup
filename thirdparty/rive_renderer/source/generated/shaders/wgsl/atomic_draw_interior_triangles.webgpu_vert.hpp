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
struct Rf{X1_:array<vec4<u32>>,}struct NB{Ub:f32,dd:f32,Xe:f32,Ye:f32,q5_:u32,ug:u32,Je:u32,Ke:u32,U7_:vec4<i32>,rg:vec2<f32>,ed:vec2<f32>,W1_:u32,vg:f32,Y5_:u32,P2_:f32,fd:f32,Ee:u32,y3_:f32,z3_:f32,gd:f32,og:u32,}struct _d{@builtin(position) _b:vec4<f32>,_v:f32,_w:array<f32,1>,_u:array<f32,1>,}struct Be{X1_:array<vec2<u32>>,}struct Ce{X1_:array<vec4<f32>>,}struct Sf{X1_:array<vec4<u32>>,}struct VertexOutput{@location(0)@interpolate(flat) member:f32,@location(1)@interpolate(flat) member_1:u32,@builtin(position) _b:vec4<f32>,}@group(0)@binding(3) var<storage>MB:Rf;var<private>_r:i32;var<private>_p:i32;var<private>KB_1:vec3<f32>;var<private>j1_:f32;var<private>z0_:u32;@group(0)@binding(0) var<uniform>k:NB;var<private>_e:_d=_d(vec4<f32>(0f,0f,0f,1f),1f,array<f32,1>(),array<f32,1>());@group(0)@binding(8) var DC:texture_2d<u32>;@group(0)@binding(10) var QC:texture_2d<f32>;@group(0)@binding(4) var<storage>TC:Be;@group(0)@binding(5) var<storage>PB:Ce;@group(0)@binding(6) var<storage>XC:Sf;@group(3)@binding(10) var T9_:sampler;fn _q(){let _c=KB_1;let _g=(bitcast<u32>(_c.z)&65535u);let _f=(_g*4u);let _j=MB.X1_[_f];let _a=bitcast<vec4<f32>>(_j);let _n=MB.X1_[(_f+1u)];let _i=((mat2x2<f32>(vec2<f32>(_a.x,_a.y),vec2<f32>(_a.z,_a.w))*_c.xy)+bitcast<vec2<f32>>(_n.xy));j1_=f32((bitcast<i32>(_c.z)>>bitcast<u32>(16i)));z0_=_g;let _k=k.Xe;let _h=k.Ye;_e._b=vec4<f32>(((_i.x*_k)-1f),((_i.y*_h)-sign(_h)),0f,1f);return;}@vertex fn main(@builtin(vertex_index) _t:u32,@builtin(instance_index) _l:u32,@location(0) KB:vec3<f32>)->VertexOutput{_r=i32(_t);_p=i32(_l);KB_1=KB;_q();let _m=j1_;let _o=z0_;let _s=_e._b;return VertexOutput(_m,_o,_s);})WGSL",
    .usedOverrides = {{false, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "atomic_draw_interior_triangles.webgpu_vert",
};
} // namespace wgsl
