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
struct UB{Qc:f32,Td:f32,bg:f32,cg:f32,A6_:u32,X9_:u32,Nf:u32,Of:u32,j8_:vec4<i32>,Lh:vec2<f32>,Ud:vec2<f32>,j2_:u32,Ph:f32,T4_:u32,a3_:f32,Vd:f32,Hf:u32,L3_:f32,M3_:f32,Wd:f32,Ih:u32,W9_:u32,wc:f32,xc:f32,}struct _k{@builtin(position) _c:vec4<f32>,_K:f32,_L:array<f32,1>,_M:array<f32,1>,}struct mh{k2_:array<vec4<u32>>,}struct Ef{k2_:array<vec2<u32>>,}struct Ff{k2_:array<vec4<f32>>,}struct nh{k2_:array<vec4<u32>>,}struct VertexOutput{@location(0) member:vec2<f32>,@location(1) member_1:vec4<f32>,@location(3)@interpolate(flat,either) member_2:vec4<f32>,@location(4)@interpolate(flat,either) member_3:u32,@location(5)@interpolate(flat,either) member_4:u32,@builtin(position) _c:vec4<f32>,}@id(1) override li:bool=true;var<private>_w:i32;var<private>_I:i32;var<private>YB_1:vec4<f32>;var<private>PC_1:vec2<f32>;var<private>PB_1:vec4<f32>;var<private>f2_:vec2<f32>;var<private>QC_1:vec2<f32>;var<private>HC_1:vec4<f32>;var<private>R0_:vec4<f32>;var<private>SB_1:vec4<f32>;var<private>Q1_:vec4<f32>;var<private>ZB_1:u32;var<private>I3_:u32;var<private>AC_1:u32;var<private>H1_:u32;var<private>BC_1:u32;@group(0)@binding(0) var<uniform>j:UB;var<private>_n:_k=_k(vec4<f32>(0f,0f,0f,1f),1f,array<f32,1>(),array<f32,1>());@group(0)@binding(7) var TB:texture_2d<u32>;@group(0)@binding(9) var YC:texture_2d<f32>;@group(0)@binding(2) var<storage>LB:mh;@group(0)@binding(3) var<storage>WC:Ef;@group(0)@binding(4) var<storage>JB:Ff;@group(0)@binding(5) var<storage>ZC:nh;@group(3)@binding(9) var wa:sampler;var<private>LC_1:u32;fn _A(){var _i:bool;var _e:vec4<f32>;let _a=YB_1;let _u=PC_1;let _g=PB_1;let _f=((mat2x2<f32>(vec2<f32>(_a.x,_a.y),vec2<f32>(_a.z,_a.w))*_u)+_g.xy);let _v=QC_1;let _j=HC_1;f2_=((_v*_j.zw)+_j.xy);if li{let _b=SB_1;let _q=vec2<f32>(_b.x,_b.y);let _r=vec2<f32>(_b.z,_b.w);switch bitcast<i32>(0u){default:{let _d=(abs(_q)+abs(_r));let _m=(_d.x!=0f);_i=_m;if _m{_i=(_d.y!=0f);}let _J=_i;if _J{let _h=((mat2x2<f32>(_q,_r)*_f)+_g.zw);let _o=-(_h);let _l=(vec2<f32>(1f,1f)/_d).xyxy;_e=(((vec4<f32>(_h.x,_h.y,_o.x,_o.y)*_l)+_l)+vec4<f32>(0.5f,0.5f,0.5f,0.5f));break;}else{_e=_g.zwzw;break;}}}let _F=_e;R0_=_F;}let _H=ZB_1;Q1_=unpack4x8unorm(_H);let _C=AC_1;I3_=_C;let _B=BC_1;H1_=_B;let _D=j.bg;let _p=j.cg;_n._c=vec4<f32>(((_f.x*_D)-1f),((_f.y*_p)-sign(_p)),0f,1f);return;}@vertex fn main(@builtin(vertex_index) _x:u32,@builtin(instance_index) _G:u32,@location(2) YB:vec4<f32>,@location(0) PC:vec2<f32>,@location(4) PB:vec4<f32>,@location(1) QC:vec2<f32>,@location(9) HC:vec4<f32>,@location(3) SB:vec4<f32>,@location(5) ZB:u32,@location(6) AC:u32,@location(7) BC:u32,@location(8) LC:u32)->VertexOutput{_w=i32(_x);_I=i32(_G);YB_1=YB;PC_1=PC;PB_1=PB;QC_1=QC;HC_1=HC;SB_1=SB;ZB_1=ZB;AC_1=AC;BC_1=BC;LC_1=LC;_A();let _t=f2_;let _E=R0_;let _y=Q1_;let _z=I3_;let _s=H1_;let _a=_n._c;return VertexOutput(_t,_E,_y,_z,_s,_a);})WGSL",
    .usedOverrides = {{false, true, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "atomic_draw_image_mesh.webgpu_vert",
};
} // namespace wgsl
