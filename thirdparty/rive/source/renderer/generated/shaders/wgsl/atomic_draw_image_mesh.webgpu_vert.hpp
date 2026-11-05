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

inline constexpr Shader atomic_draw_image_mesh_webgpu_vert = {
    .source = R"WGSL(
struct RB{Od:f32,Ze:f32,dh:f32,eh:f32,X5_:u32,Ka:u32,Og:u32,Qg:u32,Q8_:vec4<i32>,Yi:vec2<f32>,af:vec2<f32>,z2_:u32,cj:f32,B6_:u32,m3_:f32,bf:f32,Hg:u32,J3_:f32,K3_:f32,cf:f32,Vi:u32,Ja:u32,X8_:f32,Y8_:f32,}struct _k{@builtin(position) _h:vec4<f32>,_K:f32,_L:array<f32,1>,_M:array<f32,1>,}struct rd{k2_:array<vec4<u32>>,}struct Fg{k2_:array<vec4<f32>>,}struct Ei{k2_:array<vec4<u32>>,}struct VertexOutput{@location(0) member:vec2<f32>,@location(1) member_1:vec4<f32>,@location(3)@interpolate(flat,either) member_2:vec4<f32>,@location(4)@interpolate(flat,either) member_3:u32,@location(5)@interpolate(flat,either) member_4:u32,@builtin(position) _h:vec4<f32>,}@id(1) override Cj:bool=true;var<private>_x:i32;var<private>_E:i32;var<private>AC_1:vec4<f32>;var<private>SC_1:vec2<f32>;var<private>QB_1:vec4<f32>;var<private>p2_:vec2<f32>;var<private>TC_1:vec2<f32>;var<private>IC_1:vec4<f32>;var<private>Y0_:vec4<f32>;var<private>VB_1:vec4<f32>;var<private>a2_:vec4<f32>;var<private>BC_1:u32;var<private>U3_:u32;var<private>CC_1:u32;var<private>Q1_:u32;var<private>DC_1:u32;@group(0)@binding(0) var<uniform>j:RB;var<private>_m:_k=_k(vec4<f32>(0f,0f,0f,1f),1f,array<f32,1>(),array<f32,1>());@group(0)@binding(7) var WB:texture_2d<u32>;@group(0)@binding(9) var BD:texture_2d<f32>;@group(0)@binding(2) var<storage>JB:rd;@group(0)@binding(3) var<storage>KB:Fg;@group(0)@binding(4) var<storage>CD:Ei;@group(3)@binding(9) var kb:sampler;var<private>NC_1:u32;fn _J(){var _g:bool;var _f:vec4<f32>;let _a=AC_1;let _D=SC_1;let _i=QB_1;let _c=((mat2x2<f32>(vec2<f32>(_a.x,_a.y),vec2<f32>(_a.z,_a.w))*_D)+_i.xy);let _H=TC_1;let _q=IC_1;p2_=((_H*_q.zw)+_q.xy);if Cj{let _b=VB_1;let _j=vec2<f32>(_b.x,_b.y);let _r=vec2<f32>(_b.z,_b.w);switch bitcast<i32>(0u){default:{let _d=(abs(_j)+abs(_r));let _l=(_d.x!=0f);_g=_l;if _l{_g=(_d.y!=0f);}let _t=_g;if _t{let _e=((mat2x2<f32>(_j,_r)*_c)+_i.zw);let _n=-(_e);let _o=(vec2<f32>(1f,1f)/_d).xyxy;_f=(((vec4<f32>(_e.x,_e.y,_n.x,_n.y)*_o)+_o)+vec4<f32>(0.5f,0.5f,0.5f,0.5f));break;}else{_f=_i.zwzw;break;}}}let _u=_f;Y0_=_u;}let _G=BC_1;a2_=unpack4x8unorm(_G);let _A=CC_1;U3_=_A;let _v=DC_1;Q1_=_v;let _I=j.dh;let _p=j.eh;_m._h=vec4<f32>(((_c.x*_I)-1f),((_c.y*_p)-sign(_p)),0f,1f);return;}@vertex fn main(@builtin(vertex_index) _w:u32,@builtin(instance_index) _B:u32,@location(2) AC:vec4<f32>,@location(0) SC:vec2<f32>,@location(4) QB:vec4<f32>,@location(1) TC:vec2<f32>,@location(9) IC:vec4<f32>,@location(3) VB:vec4<f32>,@location(5) BC:u32,@location(6) CC:u32,@location(7) DC:u32,@location(8) NC:u32)->VertexOutput{_x=i32(_w);_E=i32(_B);AC_1=AC;SC_1=SC;QB_1=QB;TC_1=TC;IC_1=IC;VB_1=VB;BC_1=BC;CC_1=CC;DC_1=DC;NC_1=NC;_J();let _s=p2_;let _C=Y0_;let _y=a2_;let _F=U3_;let _a=Q1_;let _z=_m._h;return VertexOutput(_s,_C,_y,_F,_a,_z);})WGSL",
    .usedOverrides = {{false, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "atomic_draw_image_mesh.webgpu_vert",
};
} // namespace wgsl
