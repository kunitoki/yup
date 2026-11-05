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
struct RB{Od:f32,Ze:f32,dh:f32,eh:f32,X5_:u32,Ka:u32,Og:u32,Qg:u32,Q8_:vec4<i32>,Yi:vec2<f32>,af:vec2<f32>,z2_:u32,cj:f32,B6_:u32,m3_:f32,bf:f32,Hg:u32,J3_:f32,K3_:f32,cf:f32,Vi:u32,Ja:u32,X8_:f32,Y8_:f32,}struct _s{@builtin(position) _b:vec4<f32>,_H:f32,_I:array<f32,1>,_J:array<f32,1>,}struct VertexOutput{@location(0) member:vec4<f32>,@builtin(position) _b:vec4<f32>,}var<private>_n:i32;var<private>KC_1:vec4<u32>;@group(0)@binding(0) var<uniform>j:RB;var<private>B7_:vec4<f32>;var<private>_q:_s=_s(vec4<f32>(0f,0f,0f,1f),1f,array<f32,1>(),array<f32,1>());fn _u(){var _c:u32;var _g:f32;var _i:f32;var _f:f32;var _h:f32;var _j:f32;var _k:u32;let _o=_n;let _e=(_o>>bitcast<u32>(1i));let _p=(_e<=1i);if _p{let _A=KC_1[0u];_c=(_A&65535u);}else{let _C=KC_1[0u];_c=(_C>>bitcast<u32>(16i));}let _D=_c;let _l=(f32(_D)*0.000015258789f);let _m=select(1f,0f,((_o&1i)==0i));let _d=j.Od;_g=_m;if (_d<0f){_g=(1f-_m);}let _E=_g;let _a=KC_1[1u];_f=_l;if (((_a&2147483648u)!=0u)&&(_e==0i)){if ((_a&536870912u)!=0u){_i=0f;}else{_i=(_l-0.001953125f);}let _B=_i;_f=_B;}let _r=_f;_j=_r;if (((_a&1073741824u)!=0u)&&(_e==3i)){if ((_a&536870912u)!=0u){_h=1f;}else{_h=(_r+0.001953125f);}let _F=_h;_j=_F;}let _G=_j;if _p{let _x=KC_1[2u];_k=_x;}else{let _y=KC_1[3u];_k=_y;}let _t=_k;B7_=(vec4<f32>(((vec4(_t)>>bitcast<vec4<u32>>(vec4<u32>(16u,8u,0u,24u)))&vec4<u32>(255u,255u,255u,255u)))*vec4<f32>(0.003921569f,0.003921569f,0.003921569f,0.003921569f));_q._b=vec4<f32>(((_G*2f)-1f),(((f32((_a&536870911u))+_E)*_d)-sign(_d)),0f,1f);return;}@vertex fn main(@builtin(vertex_index) _z:u32,@location(0) KC:vec4<u32>)->VertexOutput{_n=i32(_z);KC_1=KC;_u();let _w=B7_;let _v=_q._b;return VertexOutput(_w,_v);})WGSL",
    .usedOverrides = {{false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "color_ramp.vert",
};
} // namespace wgsl
