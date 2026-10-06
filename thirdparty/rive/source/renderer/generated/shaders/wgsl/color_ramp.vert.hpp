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

inline constexpr Shader color_ramp_vert = {
    .source = R"WGSL(
struct UB{Qc:f32,Td:f32,bg:f32,cg:f32,A6_:u32,X9_:u32,Nf:u32,Of:u32,j8_:vec4<i32>,Lh:vec2<f32>,Ud:vec2<f32>,j2_:u32,Ph:f32,T4_:u32,a3_:f32,Vd:f32,Hf:u32,L3_:f32,M3_:f32,Wd:f32,Ih:u32,W9_:u32,wc:f32,xc:f32,}struct _r{@builtin(position) _f:vec4<f32>,_H:f32,_I:array<f32,1>,_J:array<f32,1>,}struct VertexOutput{@location(0) member:vec4<f32>,@builtin(position) _f:vec4<f32>,}var<private>_s:i32;var<private>JC_1:vec4<u32>;@group(0)@binding(0) var<uniform>j:UB;var<private>g7_:vec4<f32>;var<private>_l:_r=_r(vec4<f32>(0f,0f,0f,1f),1f,array<f32,1>(),array<f32,1>());fn _B(){var _e:u32;var _j:f32;var _b:f32;var _c:f32;var _k:f32;var _d:f32;var _g:u32;let _q=_s;let _h=(_q>>bitcast<u32>(1i));let _m=(_h<=1i);if _m{let _z=JC_1[0u];_e=(_z&65535u);}else{let _D=JC_1[0u];_e=(_D>>bitcast<u32>(16i));}let _t=_e;let _o=(f32(_t)*0.000015258789f);let _n=select(1f,0f,((_q&1i)==0i));let _i=j.Qc;_j=_n;if (_i<0f){_j=(1f-_n);}let _x=_j;let _a=JC_1[1u];_c=_o;if (((_a&2147483648u)!=0u)&&(_h==0i)){if ((_a&536870912u)!=0u){_b=0f;}else{_b=(_o-0.001953125f);}let _G=_b;_c=_G;}let _p=_c;_d=_p;if (((_a&1073741824u)!=0u)&&(_h==3i)){if ((_a&536870912u)!=0u){_k=1f;}else{_k=(_p+0.001953125f);}let _E=_k;_d=_E;}let _w=_d;if _m{let _A=JC_1[2u];_g=_A;}else{let _y=JC_1[3u];_g=_y;}let _v=_g;g7_=(vec4<f32>(((vec4(_v)>>bitcast<vec4<u32>>(vec4<u32>(16u,8u,0u,24u)))&vec4<u32>(255u,255u,255u,255u)))*vec4<f32>(0.003921569f,0.003921569f,0.003921569f,0.003921569f));_l._f=vec4<f32>(((_w*2f)-1f),(((f32((_a&536870911u))+_x)*_i)-sign(_i)),0f,1f);return;}@vertex fn main(@builtin(vertex_index) _C:u32,@location(0) JC:vec4<u32>)->VertexOutput{_s=i32(_C);JC_1=JC;_B();let _u=g7_;let _F=_l._f;return VertexOutput(_u,_F);})WGSL",
    .usedOverrides = {{false, false, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "color_ramp.vert",
};
} // namespace wgsl
