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
struct NB{Ub:f32,dd:f32,Xe:f32,Ye:f32,q5_:u32,ug:u32,Je:u32,Ke:u32,U7_:vec4<i32>,rg:vec2<f32>,ed:vec2<f32>,W1_:u32,vg:f32,Y5_:u32,P2_:f32,fd:f32,Ee:u32,y3_:f32,z3_:f32,gd:f32,og:u32,}struct _p{@builtin(position) _f:vec4<f32>,_H:f32,_I:array<f32,1>,_J:array<f32,1>,}struct VertexOutput{@location(0) member:vec4<f32>,@builtin(position) _f:vec4<f32>,}var<private>_m:i32;var<private>CC_1:vec4<u32>;@group(0)@binding(0) var<uniform>k:NB;var<private>R6_:vec4<f32>;var<private>_s:_p=_p(vec4<f32>(0f,0f,0f,1f),1f,array<f32,1>(),array<f32,1>());fn _x(){var _i:u32;var _j:f32;var _g:f32;var _d:f32;var _e:f32;var _c:f32;var _k:u32;let _n=_m;let _b=(_n>>bitcast<u32>(1i));let _r=(_b<=1i);if _r{let _v=CC_1[0u];_i=(_v&65535u);}else{let _A=CC_1[0u];_i=(_A>>bitcast<u32>(16i));}let _u=_i;let _l=(f32(_u)*0.000015258789f);let _q=select(1f,0f,((_n&1i)==0i));let _h=k.Ub;_j=_q;if (_h<0f){_j=(1f-_q);}let _B=_j;let _a=CC_1[1u];_d=_l;if (((_a&2147483648u)!=0u)&&(_b==0i)){if ((_a&536870912u)!=0u){_g=0f;}else{_g=(_l-0.001953125f);}let _G=_g;_d=_G;}let _o=_d;_c=_o;if (((_a&1073741824u)!=0u)&&(_b==3i)){if ((_a&536870912u)!=0u){_e=1f;}else{_e=(_o+0.001953125f);}let _F=_e;_c=_F;}let _w=_c;if _r{let _E=CC_1[2u];_k=_E;}else{let _C=CC_1[3u];_k=_C;}let _t=_k;R6_=(vec4<f32>(((vec4(_t)>>bitcast<vec4<u32>>(vec4<u32>(16u,8u,0u,24u)))&vec4<u32>(255u,255u,255u,255u)))*vec4<f32>(0.003921569f,0.003921569f,0.003921569f,0.003921569f));_s._f=vec4<f32>(((_w*2f)-1f),(((f32((_a&536870911u))+_B)*_h)-sign(_h)),0f,1f);return;}@vertex fn main(@builtin(vertex_index) _y:u32,@location(0) CC:vec4<u32>)->VertexOutput{_m=i32(_y);CC_1=CC;_x();let _z=R6_;let _D=_s._f;return VertexOutput(_z,_D);})WGSL",
    .usedOverrides = {{false, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "color_ramp.vert",
};
} // namespace wgsl
