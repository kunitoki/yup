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

inline constexpr Shader draw_depthstencil_atlas_blit_webgpu_vert = {
    .source = R"WGSL(
enable clip_distances;struct _x{@builtin(position) _s:vec4<f32>,_5:f32,@builtin(clip_distances) _b:array<f32,4>,_6:array<f32,1>,}struct rd{k2_:array<vec4<u32>>,}struct RB{Od:f32,Ze:f32,dh:f32,eh:f32,X5_:u32,Ka:u32,Og:u32,Qg:u32,Q8_:vec4<i32>,Yi:vec2<f32>,af:vec2<f32>,z2_:u32,cj:f32,B6_:u32,m3_:f32,bf:f32,Hg:u32,J3_:f32,K3_:f32,cf:f32,Vi:u32,Ja:u32,X8_:f32,Y8_:f32,}struct Fg{k2_:array<vec4<f32>>,}struct Ei{k2_:array<vec4<u32>>,}struct fl{k2_:array<u32>,}struct VertexOutput{@builtin(position) _s:vec4<f32>,@builtin(clip_distances) _b:array<f32,4>,@location(1) member:vec2<f32>,@location(4)@interpolate(flat,either) member_1:f32,@location(6)@interpolate(flat,either) member_2:f32,@location(0) member_3:vec4<f32>,@location(9) member_4:vec3<f32>,}@id(0) override Bj:bool=true;@id(2) override Dj:bool=true;@id(1) override Cj:bool=true;@id(4) override Fj:bool=true;var<private>_a:_x=_x(vec4<f32>(0f,0f,0f,1f),1f,array<f32,4>(),array<f32,1>());@group(0)@binding(2) var<storage>JB:rd;@group(0)@binding(0) var<uniform>j:RB;var<private>_W:i32;var<private>MB_1:vec3<f32>;var<private>X2_:vec2<f32>;var<private>j4_:f32;var<private>S0_:f32;@group(0)@binding(3) var<storage>KB:Fg;var<private>N0_:vec4<f32>;var<private>O0_:vec3<f32>;@group(0)@binding(7) var WB:texture_2d<u32>;@group(0)@binding(9) var BD:texture_2d<f32>;@group(0)@binding(4) var<storage>CD:Ei;@group(0)@binding(6) var<storage>WE:fl;@group(3)@binding(9) var kb:sampler;fn _V(){var _t:u32;var _o:f32;var _m:f32;var _n:f32;var _r:bool;var _u:f32;let _i=MB_1;let _e=(bitcast<u32>(_i.z)&65535u);let _C=(_e*4u);let _y=(_C+1u);let _3=JB.k2_[_y][2u];let _h=_i.xy;let _P=JB.k2_[(_C+3u)];let _v=bitcast<vec3<f32>>(_P.xyz);let _X=j.Yi;X2_=(((_h*_v.x)+_v.yz)*_X);let _c=JB.k2_[_y];let _q=(_c.z&15u);if Bj{let _D=(_q==0u);if _D{_t=_c.w;}else{_t=_c.z;}let _O=_t;let _H=(_O>>bitcast<u32>(16i));let _M=j.B6_;if (_H==0u){_o=0f;}else{_o=unpack2x16float(((_H+1023u)*_M)).x;}let _G=_o;_m=_G;if _D{_m=-(_G);}let _1=_m;j4_=_1;}if Dj{S0_=f32(((_c.z>>bitcast<u32>(4i))&15u));}if Cj{let _J=(_e*8u);let _d=KB.k2_[(_J+2u)];let _B=KB.k2_[(_J+3u)];if any((_d!=vec4<f32>(0f,0f,0f,0f))){let _j=((mat2x2<f32>(vec2<f32>(_d.x,_d.y),vec2<f32>(_d.z,_d.w))*_h)+_B.xy);_a._b[0i]=(_j.x+1f);_a._b[1i]=(_j.y+1f);_a._b[2i]=(1f-_j.x);_a._b[3i]=(1f-_j.y);}else{let _k=(_B.x-0.5f);_a._b[3i]=_k;_a._b[2i]=_k;_a._b[1i]=_k;_a._b[0i]=_k;}}if (_q==1u){N0_=unpack4x8unorm(_c.w);}else{let _z=(_e*8u);let _g=KB.k2_[_z];let _l=KB.k2_[(_z+1u)];let _F=((mat2x2<f32>(vec2<f32>(_g.x,_g.y),vec2<f32>(_g.z,_g.w))*_h)+_l.xy);let _I=((_l.w+(f32(_q)*0.125f))+(max(_l.z,0f)*0.00024414063f));if (_l.z<0f){_n=-(_I);}else{_n=_I;}let _2=_n;N0_=vec4<f32>(_F.x,_F.y,_2,(-0.75f-round((bitcast<f32>(_c.w)*255f))));}_r=Fj;if Fj{_r=((_c.z&2048u)!=0u);}let _U=_r;if _U{let _A=(_e*8u);let _f=KB.k2_[(_A+4u)];let _E=KB.k2_[(_A+5u)];let _w=((mat2x2<f32>(vec2<f32>(_f.x,_f.y),vec2<f32>(_f.z,_f.w))*_h)+_E.xy);_u=(1f+_E.z);if ((_c.z&4096u)!=0u){_u=(-1f-f32(((_c.z&24576u)>>bitcast<u32>(13u))));}let _Q=_u;O0_=vec3<f32>(_w.x,_w.y,_Q);}else{O0_=vec3<f32>(0f,0f,0f);}let _N=j.dh;let _K=j.eh;let _p=vec4<f32>(((_i.x*_N)-1f),((_i.y*_K)-sign(_K)),0f,1f);_a._s=vec4<f32>(_p.x,_p.y,((f32((((_3>>bitcast<u32>(16u))<<bitcast<u32>(8u))|255u))*0.000000059604645f)+0.000000029802322f),_p.w);return;}@vertex fn main(@builtin(vertex_index) _T:u32,@location(0) MB:vec3<f32>)->VertexOutput{_W=i32(_T);MB_1=MB;_V();let _S=_a._s;let _Y=_a._b;let _L=X2_;let _0=j4_;let _R=S0_;let _4=N0_;let _Z=O0_;return VertexOutput(_S,_Y,_L,_0,_R,_4,_Z);})WGSL",
    .usedOverrides = {{true, true, true, false, true, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "draw_depthstencil_atlas_blit.webgpu_vert",
};
} // namespace wgsl
