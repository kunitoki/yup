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
enable clip_distances;struct _n{@builtin(position) _i:vec4<f32>,_H:f32,@builtin(clip_distances) _b:array<f32,4>,_G:array<f32,1>,}struct UB{Qc:f32,Td:f32,bg:f32,cg:f32,A6_:u32,X9_:u32,Nf:u32,Of:u32,j8_:vec4<i32>,Lh:vec2<f32>,Ud:vec2<f32>,j2_:u32,Ph:f32,T4_:u32,a3_:f32,Vd:f32,Hf:u32,L3_:f32,M3_:f32,Wd:f32,Ih:u32,W9_:u32,wc:f32,xc:f32,}struct VertexOutput{@builtin(position) _i:vec4<f32>,@builtin(clip_distances) _b:array<f32,4>,@location(0) member:vec2<f32>,@location(1)@interpolate(flat,either) member_1:f32,@location(3)@interpolate(flat,either) member_2:vec4<f32>,@location(4)@interpolate(flat,either) member_3:u32,}@id(0) override ki:bool=true;@id(1) override li:bool=true;var<private>_a:_n=_n(vec4<f32>(0f,0f,0f,1f),1f,array<f32,4>(),array<f32,1>());var<private>_p:i32;var<private>YB_1:vec4<f32>;var<private>PC_1:vec2<f32>;var<private>PB_1:vec4<f32>;var<private>V5_:vec2<f32>;var<private>QC_1:vec2<f32>;var<private>HC_1:vec4<f32>;var<private>Y3_:f32;var<private>AC_1:u32;@group(0)@binding(0) var<uniform>j:UB;var<private>SB_1:vec4<f32>;var<private>LC_1:u32;var<private>Q1_:vec4<f32>;var<private>ZB_1:u32;var<private>H1_:u32;var<private>BC_1:u32;fn _A(){var _k:f32;let _e=YB_1;let _C=PC_1;let _h=PB_1;let _g=((mat2x2<f32>(vec2<f32>(_e.x,_e.y),vec2<f32>(_e.z,_e.w))*_C)+_h.xy);let _s=QC_1;let _m=HC_1;V5_=((_s*_m.zw)+_m.xy);if ki{let _l=AC_1;let _u=j.T4_;if (_l==0u){_k=0f;}else{_k=unpack2x16float(((_l+1023u)*_u)).x;}let _v=_k;Y3_=_v;}if li{let _c=SB_1;if any((_c!=vec4<f32>(0f,0f,0f,0f))){let _d=((mat2x2<f32>(vec2<f32>(_c.x,_c.y),vec2<f32>(_c.z,_c.w))*_g)+_h.zw);_a._b[0i]=(_d.x+1f);_a._b[1i]=(_d.y+1f);_a._b[2i]=(1f-_d.x);_a._b[3i]=(1f-_d.y);}else{let _f=(_h.z-0.5f);_a._b[3i]=_f;_a._b[2i]=_f;_a._b[1i]=_f;_a._b[0i]=_f;}}let _q=j.bg;let _o=j.cg;let _j=vec4<f32>(((_g.x*_q)-1f),((_g.y*_o)-sign(_o)),0f,1f);let _w=LC_1;let _x=ZB_1;Q1_=unpack4x8unorm(_x);let _E=BC_1;H1_=_E;_a._i=vec4<f32>(_j.x,_j.y,((f32(((_w<<bitcast<u32>(8u))|255u))*0.000000059604645f)+0.000000029802322f),_j.w);return;}@vertex fn main(@builtin(vertex_index) _z:u32,@location(2) YB:vec4<f32>,@location(0) PC:vec2<f32>,@location(4) PB:vec4<f32>,@location(1) QC:vec2<f32>,@location(9) HC:vec4<f32>,@location(6) AC:u32,@location(3) SB:vec4<f32>,@location(8) LC:u32,@location(5) ZB:u32,@location(7) BC:u32)->VertexOutput{_p=i32(_z);YB_1=YB;PC_1=PC;PB_1=PB;QC_1=QC;HC_1=HC;AC_1=AC;SB_1=SB;LC_1=LC;ZB_1=ZB;BC_1=BC;_A();let _D=_a._i;let _y=_a._b;let _r=V5_;let _B=Y3_;let _F=Q1_;let _t=H1_;return VertexOutput(_D,_y,_r,_B,_F,_t);})WGSL",
    .usedOverrides = {{true, true, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "draw_depthstencil_image_mesh.webgpu_vert",
};
} // namespace wgsl
