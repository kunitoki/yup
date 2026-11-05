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

inline constexpr Shader draw_depthstencil_image_mesh_webgpu_vert = {
    .source = R"WGSL(
enable clip_distances;struct _m{@builtin(position) _i:vec4<f32>,_H:f32,@builtin(clip_distances) _b:array<f32,4>,_G:array<f32,1>,}struct RB{Od:f32,Ze:f32,dh:f32,eh:f32,X5_:u32,Ka:u32,Og:u32,Qg:u32,Q8_:vec4<i32>,Yi:vec2<f32>,af:vec2<f32>,z2_:u32,cj:f32,B6_:u32,m3_:f32,bf:f32,Hg:u32,J3_:f32,K3_:f32,cf:f32,Vi:u32,Ja:u32,X8_:f32,Y8_:f32,}struct VertexOutput{@builtin(position) _i:vec4<f32>,@builtin(clip_distances) _b:array<f32,4>,@location(0) member:vec2<f32>,@location(1)@interpolate(flat,either) member_1:f32,@location(3)@interpolate(flat,either) member_2:vec4<f32>,@location(4)@interpolate(flat,either) member_3:u32,}@id(0) override Bj:bool=true;@id(1) override Cj:bool=true;var<private>_a:_m=_m(vec4<f32>(0f,0f,0f,1f),1f,array<f32,4>(),array<f32,1>());var<private>_w:i32;var<private>AC_1:vec4<f32>;var<private>SC_1:vec2<f32>;var<private>QB_1:vec4<f32>;var<private>j6_:vec2<f32>;var<private>TC_1:vec2<f32>;var<private>IC_1:vec4<f32>;var<private>j4_:f32;var<private>CC_1:u32;@group(0)@binding(0) var<uniform>j:RB;var<private>VB_1:vec4<f32>;var<private>NC_1:u32;var<private>a2_:vec4<f32>;var<private>BC_1:u32;var<private>Q1_:u32;var<private>DC_1:u32;fn _s(){var _j:f32;let _f=AC_1;let _y=SC_1;let _k=QB_1;let _h=((mat2x2<f32>(vec2<f32>(_f.x,_f.y),vec2<f32>(_f.z,_f.w))*_y)+_k.xy);let _q=TC_1;let _o=IC_1;j6_=((_q*_o.zw)+_o.xy);if Bj{let _l=CC_1;let _D=j.B6_;if (_l==0u){_j=0f;}else{_j=unpack2x16float(((_l+1023u)*_D)).x;}let _r=_j;j4_=_r;}if Cj{let _c=VB_1;if any((_c!=vec4<f32>(0f,0f,0f,0f))){let _e=((mat2x2<f32>(vec2<f32>(_c.x,_c.y),vec2<f32>(_c.z,_c.w))*_h)+_k.zw);_a._b[0i]=(_e.x+1f);_a._b[1i]=(_e.y+1f);_a._b[2i]=(1f-_e.x);_a._b[3i]=(1f-_e.y);}else{let _d=(_k.z-0.5f);_a._b[3i]=_d;_a._b[2i]=_d;_a._b[1i]=_d;_a._b[0i]=_d;}}let _u=j.dh;let _n=j.eh;let _g=vec4<f32>(((_h.x*_u)-1f),((_h.y*_n)-sign(_n)),0f,1f);let _t=NC_1;let _z=BC_1;a2_=unpack4x8unorm(_z);let _B=DC_1;Q1_=_B;_a._i=vec4<f32>(_g.x,_g.y,((f32(((_t<<bitcast<u32>(8u))|255u))*0.000000059604645f)+0.000000029802322f),_g.w);return;}@vertex fn main(@builtin(vertex_index) _F:u32,@location(2) AC:vec4<f32>,@location(0) SC:vec2<f32>,@location(4) QB:vec4<f32>,@location(1) TC:vec2<f32>,@location(9) IC:vec4<f32>,@location(6) CC:u32,@location(3) VB:vec4<f32>,@location(8) NC:u32,@location(5) BC:u32,@location(7) DC:u32)->VertexOutput{_w=i32(_F);AC_1=AC;SC_1=SC;QB_1=QB;TC_1=TC;IC_1=IC;CC_1=CC;VB_1=VB;NC_1=NC;BC_1=BC;DC_1=DC;_s();let _E=_a._i;let _p=_a._b;let _x=j6_;let _A=j4_;let _C=a2_;let _v=Q1_;return VertexOutput(_E,_p,_x,_A,_C,_v);})WGSL",
    .usedOverrides = {{true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "draw_depthstencil_image_mesh.webgpu_vert",
};
} // namespace wgsl
