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
struct UB{Qc:f32,Td:f32,bg:f32,cg:f32,A6_:u32,X9_:u32,Nf:u32,Of:u32,j8_:vec4<i32>,Lh:vec2<f32>,Ud:vec2<f32>,j2_:u32,Ph:f32,T4_:u32,a3_:f32,Vd:f32,Hf:u32,L3_:f32,M3_:f32,Wd:f32,Ih:u32,W9_:u32,wc:f32,xc:f32,}struct _e{@builtin(position) _b:vec4<f32>,_A:f32,_C:array<f32,1>,_B:array<f32,1>,}struct VertexOutput{@location(0) member:vec2<f32>,@location(1)@interpolate(flat,either) member_1:f32,@location(3)@interpolate(flat,either) member_2:vec4<f32>,@location(4)@interpolate(flat,either) member_3:u32,@builtin(position) _b:vec4<f32>,}@id(0) override ki:bool=true;var<private>_p:i32;var<private>YB_1:vec4<f32>;var<private>PC_1:vec2<f32>;var<private>PB_1:vec4<f32>;var<private>V5_:vec2<f32>;var<private>QC_1:vec2<f32>;var<private>HC_1:vec4<f32>;var<private>Y3_:f32;var<private>AC_1:u32;@group(0)@binding(0) var<uniform>j:UB;var<private>SB_1:vec4<f32>;var<private>LC_1:u32;var<private>Q1_:vec4<f32>;var<private>ZB_1:u32;var<private>H1_:u32;var<private>BC_1:u32;var<private>_i:_e=_e(vec4<f32>(0f,0f,0f,1f),1f,array<f32,1>(),array<f32,1>());fn _r(){var _d:f32;let _a=YB_1;let _z=PC_1;let _o=PB_1;let _h=((mat2x2<f32>(vec2<f32>(_a.x,_a.y),vec2<f32>(_a.z,_a.w))*_z)+_o.xy);let _k=QC_1;let _g=HC_1;V5_=((_k*_g.zw)+_g.xy);if ki{let _j=AC_1;let _m=j.T4_;if (_j==0u){_d=0f;}else{_d=unpack2x16float(((_j+1023u)*_m)).x;}let _w=_d;Y3_=_w;}let _s=j.bg;let _f=j.cg;let _c=vec4<f32>(((_h.x*_s)-1f),((_h.y*_f)-sign(_f)),0f,1f);let _n=LC_1;let _t=ZB_1;Q1_=unpack4x8unorm(_t);let _u=BC_1;H1_=_u;_i._b=vec4<f32>(_c.x,_c.y,((f32(((_n<<bitcast<u32>(8u))|255u))*0.000000059604645f)+0.000000029802322f),_c.w);return;}@vertex fn main(@builtin(vertex_index) _y:u32,@location(2) YB:vec4<f32>,@location(0) PC:vec2<f32>,@location(4) PB:vec4<f32>,@location(1) QC:vec2<f32>,@location(9) HC:vec4<f32>,@location(6) AC:u32,@location(3) SB:vec4<f32>,@location(8) LC:u32,@location(5) ZB:u32,@location(7) BC:u32)->VertexOutput{_p=i32(_y);YB_1=YB;PC_1=PC;PB_1=PB;QC_1=QC;HC_1=HC;AC_1=AC;SB_1=SB;LC_1=LC;ZB_1=ZB;BC_1=BC;_r();let _x=V5_;let _v=Y3_;let _a=Q1_;let _l=H1_;let _q=_i._b;return VertexOutput(_x,_v,_a,_l,_q);})WGSL",
    .usedOverrides = {{true, false, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "draw_depthstencil_image_mesh.webgpu_noclipdistance_vert",
};
} // namespace wgsl
