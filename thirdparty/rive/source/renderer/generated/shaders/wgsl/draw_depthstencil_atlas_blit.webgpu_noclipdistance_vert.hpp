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

inline constexpr Shader draw_depthstencil_atlas_blit_webgpu_noclipdistance_vert = {
    .source = R"WGSL(
struct rd{k2_:array<vec4<u32>>,}struct RB{Od:f32,Ze:f32,dh:f32,eh:f32,X5_:u32,Ka:u32,Og:u32,Qg:u32,Q8_:vec4<i32>,Yi:vec2<f32>,af:vec2<f32>,z2_:u32,cj:f32,B6_:u32,m3_:f32,bf:f32,Hg:u32,J3_:f32,K3_:f32,cf:f32,Vi:u32,Ja:u32,X8_:f32,Y8_:f32,}struct Fg{k2_:array<vec4<f32>>,}struct _A{@builtin(position) _i:vec4<f32>,_0:f32,_Y:array<f32,1>,_Z:array<f32,1>,}struct Ei{k2_:array<vec4<u32>>,}struct fl{k2_:array<u32>,}struct VertexOutput{@location(1) member:vec2<f32>,@location(4)@interpolate(flat,either) member_1:f32,@location(6)@interpolate(flat,either) member_2:f32,@location(0) member_3:vec4<f32>,@location(9) member_4:vec3<f32>,@builtin(position) _i:vec4<f32>,}@id(0) override Bj:bool=true;@id(2) override Dj:bool=true;@id(4) override Fj:bool=true;@group(0)@binding(2) var<storage>JB:rd;@group(0)@binding(0) var<uniform>j:RB;var<private>_T:i32;var<private>MB_1:vec3<f32>;var<private>X2_:vec2<f32>;var<private>j4_:f32;var<private>S0_:f32;@group(0)@binding(3) var<storage>KB:Fg;var<private>N0_:vec4<f32>;var<private>O0_:vec3<f32>;var<private>_u:_A=_A(vec4<f32>(0f,0f,0f,1f),1f,array<f32,1>(),array<f32,1>());@group(0)@binding(7) var WB:texture_2d<u32>;@group(0)@binding(9) var BD:texture_2d<f32>;@group(0)@binding(4) var<storage>CD:Ei;@group(0)@binding(6) var<storage>WE:fl;@group(3)@binding(9) var kb:sampler;fn _W(){var _h:u32;var _n:f32;var _l:f32;var _g:f32;var _j:bool;var _o:f32;let _c=MB_1;let _p=(bitcast<u32>(_c.z)&65535u);let _t=(_p*4u);let _v=(_t+1u);let _U=JB.k2_[_v][2u];let _k=_c.xy;let _Q=JB.k2_[(_t+3u)];let _x=bitcast<vec3<f32>>(_Q.xyz);let _I=j.Yi;X2_=(((_k*_x.x)+_x.yz)*_I);let _a=JB.k2_[_v];let _f=(_a.z&15u);if Bj{let _y=(_f==0u);if _y{_h=_a.w;}else{_h=_a.z;}let _L=_h;let _E=(_L>>bitcast<u32>(16i));let _N=j.B6_;if (_E==0u){_n=0f;}else{_n=unpack2x16float(((_E+1023u)*_N)).x;}let _z=_n;_l=_z;if _y{_l=-(_z);}let _X=_l;j4_=_X;}if Dj{S0_=f32(((_a.z>>bitcast<u32>(4i))&15u));}if (_f==1u){N0_=unpack4x8unorm(_a.w);}else{let _s=(_p*8u);let _e=KB.k2_[_s];let _b=KB.k2_[(_s+1u)];let _r=((mat2x2<f32>(vec2<f32>(_e.x,_e.y),vec2<f32>(_e.z,_e.w))*_k)+_b.xy);let _B=((_b.w+(f32(_f)*0.125f))+(max(_b.z,0f)*0.00024414063f));if (_b.z<0f){_g=-(_B);}else{_g=_B;}let _F=_g;N0_=vec4<f32>(_r.x,_r.y,_F,(-0.75f-round((bitcast<f32>(_a.w)*255f))));}_j=Fj;if Fj{_j=((_a.z&2048u)!=0u);}let _K=_j;if _K{let _q=(_p*8u);let _d=KB.k2_[(_q+4u)];let _C=KB.k2_[(_q+5u)];let _w=((mat2x2<f32>(vec2<f32>(_d.x,_d.y),vec2<f32>(_d.z,_d.w))*_k)+_C.xy);_o=(1f+_C.z);if ((_a.z&4096u)!=0u){_o=(-1f-f32(((_a.z&24576u)>>bitcast<u32>(13u))));}let _R=_o;O0_=vec3<f32>(_w.x,_w.y,_R);}else{O0_=vec3<f32>(0f,0f,0f);}let _S=j.dh;let _D=j.eh;let _m=vec4<f32>(((_c.x*_S)-1f),((_c.y*_D)-sign(_D)),0f,1f);_u._i=vec4<f32>(_m.x,_m.y,((f32((((_U>>bitcast<u32>(16u))<<bitcast<u32>(8u))|255u))*0.000000059604645f)+0.000000029802322f),_m.w);return;}@vertex fn main(@builtin(vertex_index) _J:u32,@location(0) MB:vec3<f32>)->VertexOutput{_T=i32(_J);MB_1=MB;_W();let _O=X2_;let _V=j4_;let _M=S0_;let _H=N0_;let _G=O0_;let _P=_u._i;return VertexOutput(_O,_V,_M,_H,_G,_P);})WGSL",
    .usedOverrides = {{true, false, true, false, true, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "draw_depthstencil_atlas_blit.webgpu_noclipdistance_vert",
};
} // namespace wgsl
