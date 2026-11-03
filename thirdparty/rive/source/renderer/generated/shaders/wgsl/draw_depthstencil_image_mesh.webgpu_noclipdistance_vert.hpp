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

inline constexpr Shader draw_depthstencil_image_mesh_webgpu_noclipdistance_vert = {
    .source = R"WGSL(
struct RB{Od:f32,Ze:f32,dh:f32,eh:f32,X5_:u32,Ka:u32,Og:u32,Qg:u32,Q8_:vec4<i32>,Yi:vec2<f32>,af:vec2<f32>,z2_:u32,cj:f32,B6_:u32,m3_:f32,bf:f32,Hg:u32,J3_:f32,K3_:f32,cf:f32,Vi:u32,Ja:u32,X8_:f32,Y8_:f32,}struct _f{@builtin(position) _d:vec4<f32>,_A:f32,_B:array<f32,1>,_C:array<f32,1>,}struct VertexOutput{@location(0) member:vec2<f32>,@location(1)@interpolate(flat,either) member_1:f32,@location(3)@interpolate(flat,either) member_2:vec4<f32>,@location(4)@interpolate(flat,either) member_3:u32,@builtin(position) _d:vec4<f32>,}@id(0) override Bj:bool=true;var<private>_w:i32;var<private>AC_1:vec4<f32>;var<private>SC_1:vec2<f32>;var<private>QB_1:vec4<f32>;var<private>j6_:vec2<f32>;var<private>TC_1:vec2<f32>;var<private>IC_1:vec4<f32>;var<private>j4_:f32;var<private>CC_1:u32;@group(0)@binding(0) var<uniform>j:RB;var<private>VB_1:vec4<f32>;var<private>NC_1:u32;var<private>a2_:vec4<f32>;var<private>BC_1:u32;var<private>Q1_:u32;var<private>DC_1:u32;var<private>_e:_f=_f(vec4<f32>(0f,0f,0f,1f),1f,array<f32,1>(),array<f32,1>());fn _v(){var _b:f32;let _a=AC_1;let _m=SC_1;let _x=QB_1;let _g=((mat2x2<f32>(vec2<f32>(_a.x,_a.y),vec2<f32>(_a.z,_a.w))*_m)+_x.xy);let _l=TC_1;let _i=IC_1;j6_=((_l*_i.zw)+_i.xy);if Bj{let _h=CC_1;let _q=j.B6_;if (_h==0u){_b=0f;}else{_b=unpack2x16float(((_h+1023u)*_q)).x;}let _z=_b;j4_=_z;}let _r=j.dh;let _j=j.eh;let _c=vec4<f32>(((_g.x*_r)-1f),((_g.y*_j)-sign(_j)),0f,1f);let _o=NC_1;let _n=BC_1;a2_=unpack4x8unorm(_n);let _u=DC_1;Q1_=_u;_e._d=vec4<f32>(_c.x,_c.y,((f32(((_o<<bitcast<u32>(8u))|255u))*0.000000059604645f)+0.000000029802322f),_c.w);return;}@vertex fn main(@builtin(vertex_index) _k:u32,@location(2) AC:vec4<f32>,@location(0) SC:vec2<f32>,@location(4) QB:vec4<f32>,@location(1) TC:vec2<f32>,@location(9) IC:vec4<f32>,@location(6) CC:u32,@location(3) VB:vec4<f32>,@location(8) NC:u32,@location(5) BC:u32,@location(7) DC:u32)->VertexOutput{_w=i32(_k);AC_1=AC;SC_1=SC;QB_1=QB;TC_1=TC;IC_1=IC;CC_1=CC;VB_1=VB;NC_1=NC;BC_1=BC;DC_1=DC;_v();let _p=j6_;let _y=j4_;let _a=a2_;let _s=Q1_;let _t=_e._d;return VertexOutput(_p,_y,_a,_s,_t);})WGSL",
    .usedOverrides = {{true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "draw_depthstencil_image_mesh.webgpu_noclipdistance_vert",
};
} // namespace wgsl
