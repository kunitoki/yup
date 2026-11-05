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

inline constexpr Shader draw_depthstencil_atlas_blit_webgpu_nossbo_noclipdistance_vert = {
    .source = R"WGSL(
struct RB{Od:f32,Ze:f32,dh:f32,eh:f32,X5_:u32,Ka:u32,Og:u32,Qg:u32,Q8_:vec4<i32>,Yi:vec2<f32>,af:vec2<f32>,z2_:u32,cj:f32,B6_:u32,m3_:f32,bf:f32,Hg:u32,J3_:f32,K3_:f32,cf:f32,Vi:u32,Ja:u32,X8_:f32,Y8_:f32,}struct _E{@builtin(position) _k:vec4<f32>,_3:f32,_5:array<f32,1>,_4:array<f32,1>,}struct VertexOutput{@location(1) member:vec2<f32>,@location(4)@interpolate(flat,either) member_1:f32,@location(6)@interpolate(flat,either) member_2:f32,@location(0) member_3:vec4<f32>,@location(9) member_4:vec3<f32>,@builtin(position) _k:vec4<f32>,}@id(0) override Bj:bool=true;@id(2) override Dj:bool=true;@id(4) override Fj:bool=true;@group(0)@binding(2) var JB:texture_2d<u32>;@group(0)@binding(0) var<uniform>j:RB;var<private>_R:i32;var<private>MB_1:vec3<f32>;var<private>X2_:vec2<f32>;var<private>j4_:f32;var<private>S0_:f32;@group(0)@binding(3) var KB:texture_2d<f32>;var<private>N0_:vec4<f32>;var<private>O0_:vec3<f32>;var<private>_w:_E=_E(vec4<f32>(0f,0f,0f,1f),1f,array<f32,1>(),array<f32,1>());@group(0)@binding(7) var WB:texture_2d<u32>;@group(0)@binding(9) var BD:texture_2d<f32>;@group(0)@binding(4) var CD:texture_2d<u32>;@group(0)@binding(6) var WE:texture_2d<u32>;@group(3)@binding(9) var kb:sampler;fn _K(){var _m:u32;var _f:f32;var _l:f32;var _g:f32;var _n:bool;var _j:f32;let _e=MB_1;let _o=(bitcast<u32>(_e.z)&65535u);let _H=(_o*4u);let _t=(_H+1u);let _C=vec2<i32>(bitcast<i32>((_t&255u)),bitcast<i32>((_t>>bitcast<u32>(8i))));let _W=textureLoad(JB,_C,0i);let _i=_e.xy;let _v=(_H+3u);let _S=textureLoad(JB,vec2<i32>(bitcast<i32>((_v&255u)),bitcast<i32>((_v>>bitcast<u32>(8i)))),0i);let _x=bitcast<vec3<f32>>(_S.xyz);let _V=j.Yi;X2_=(((_i*_x.x)+_x.yz)*_V);let _a=textureLoad(JB,_C,0i);let _h=(_a.z&15u);if Bj{let _D=(_h==0u);if _D{_m=_a.w;}else{_m=_a.z;}let _Y=_m;let _F=(_Y>>bitcast<u32>(16i));let _0=j.B6_;if (_F==0u){_f=0f;}else{_f=unpack2x16float(((_F+1023u)*_0)).x;}let _r=_f;_l=_r;if _D{_l=-(_r);}let _X=_l;j4_=_X;}if Dj{S0_=f32(((_a.z>>bitcast<u32>(4i))&15u));}if (_h==1u){N0_=unpack4x8unorm(_a.w);}else{let _p=(_o*8u);let _c=textureLoad(KB,vec2<i32>(bitcast<i32>((_p&255u)),bitcast<i32>((_p>>bitcast<u32>(8i)))),0i);let _J=(_p+1u);let _d=textureLoad(KB,vec2<i32>(bitcast<i32>((_J&255u)),bitcast<i32>((_J>>bitcast<u32>(8i)))),0i);let _u=((mat2x2<f32>(vec2<f32>(_c.x,_c.y),vec2<f32>(_c.z,_c.w))*_i)+_d.xy);let _y=((_d.w+(f32(_h)*0.125f))+(max(_d.z,0f)*0.00024414063f));if (_d.z<0f){_g=-(_y);}else{_g=_y;}let _Z=_g;N0_=vec4<f32>(_u.x,_u.y,_Z,(-0.75f-round((bitcast<f32>(_a.w)*255f))));}_n=Fj;if Fj{_n=((_a.z&2048u)!=0u);}let _L=_n;if _L{let _s=(_o*8u);let _B=(_s+4u);let _b=textureLoad(KB,vec2<i32>(bitcast<i32>((_B&255u)),bitcast<i32>((_B>>bitcast<u32>(8i)))),0i);let _I=(_s+5u);let _G=textureLoad(KB,vec2<i32>(bitcast<i32>((_I&255u)),bitcast<i32>((_I>>bitcast<u32>(8i)))),0i);let _A=((mat2x2<f32>(vec2<f32>(_b.x,_b.y),vec2<f32>(_b.z,_b.w))*_i)+_G.xy);_j=(1f+_G.z);if ((_a.z&4096u)!=0u){_j=(-1f-f32(((_a.z&24576u)>>bitcast<u32>(13u))));}let _1=_j;O0_=vec3<f32>(_A.x,_A.y,_1);}else{O0_=vec3<f32>(0f,0f,0f);}let _O=j.dh;let _z=j.eh;let _q=vec4<f32>(((_e.x*_O)-1f),((_e.y*_z)-sign(_z)),0f,1f);_w._k=vec4<f32>(_q.x,_q.y,((f32((((_W.z>>bitcast<u32>(16u))<<bitcast<u32>(8u))|255u))*0.000000059604645f)+0.000000029802322f),_q.w);return;}@vertex fn main(@builtin(vertex_index) _T:u32,@location(0) MB:vec3<f32>)->VertexOutput{_R=i32(_T);MB_1=MB;_K();let _2=X2_;let _M=j4_;let _P=S0_;let _U=N0_;let _N=O0_;let _Q=_w._k;return VertexOutput(_2,_M,_P,_U,_N,_Q);})WGSL",
    .usedOverrides = {{true, false, true, false, true, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "draw_depthstencil_atlas_blit.webgpu_nossbo_noclipdistance_vert",
};
} // namespace wgsl
