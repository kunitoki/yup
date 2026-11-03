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

inline constexpr Shader draw_depthstencil_atlas_blit_webgpu_nossbo_vert = {
    .source = R"WGSL(
enable clip_distances;struct _I{@builtin(position) _t:vec4<f32>,_ac:f32,@builtin(clip_distances) _b:array<f32,4>,_ad:array<f32,1>,}struct RB{Od:f32,Ze:f32,dh:f32,eh:f32,X5_:u32,Ka:u32,Og:u32,Qg:u32,Q8_:vec4<i32>,Yi:vec2<f32>,af:vec2<f32>,z2_:u32,cj:f32,B6_:u32,m3_:f32,bf:f32,Hg:u32,J3_:f32,K3_:f32,cf:f32,Vi:u32,Ja:u32,X8_:f32,Y8_:f32,}struct VertexOutput{@builtin(position) _t:vec4<f32>,@builtin(clip_distances) _b:array<f32,4>,@location(1) member:vec2<f32>,@location(4)@interpolate(flat,either) member_1:f32,@location(6)@interpolate(flat,either) member_2:f32,@location(0) member_3:vec4<f32>,@location(9) member_4:vec3<f32>,}@id(0) override Bj:bool=true;@id(2) override Dj:bool=true;@id(1) override Cj:bool=true;@id(4) override Fj:bool=true;var<private>_a:_I=_I(vec4<f32>(0f,0f,0f,1f),1f,array<f32,4>(),array<f32,1>());@group(0)@binding(2) var JB:texture_2d<u32>;@group(0)@binding(0) var<uniform>j:RB;var<private>_0:i32;var<private>MB_1:vec3<f32>;var<private>X2_:vec2<f32>;var<private>j4_:f32;var<private>S0_:f32;@group(0)@binding(3) var KB:texture_2d<f32>;var<private>N0_:vec4<f32>;var<private>O0_:vec3<f32>;@group(0)@binding(7) var WB:texture_2d<u32>;@group(0)@binding(9) var BD:texture_2d<f32>;@group(0)@binding(4) var CD:texture_2d<u32>;@group(0)@binding(6) var WE:texture_2d<u32>;@group(3)@binding(9) var kb:sampler;fn _X(){var _n:u32;var _r:f32;var _o:f32;var _s:f32;var _v:bool;var _q:f32;let _j=MB_1;let _l=(bitcast<u32>(_j.z)&65535u);let _M=(_l*4u);let _F=(_M+1u);let _D=vec2<i32>(bitcast<i32>((_F&255u)),bitcast<i32>((_F>>bitcast<u32>(8i))));let _2=textureLoad(JB,_D,0i);let _g=_j.xy;let _O=(_M+3u);let _U=textureLoad(JB,vec2<i32>(bitcast<i32>((_O&255u)),bitcast<i32>((_O>>bitcast<u32>(8i)))),0i);let _G=bitcast<vec3<f32>>(_U.xyz);let _aa=j.Yi;X2_=(((_g*_G.x)+_G.yz)*_aa);let _c=textureLoad(JB,_D,0i);let _u=(_c.z&15u);if Bj{let _w=(_u==0u);if _w{_n=_c.w;}else{_n=_c.z;}let _V=_n;let _y=(_V>>bitcast<u32>(16i));let _8=j.B6_;if (_y==0u){_r=0f;}else{_r=unpack2x16float(((_y+1023u)*_8)).x;}let _L=_r;_o=_L;if _w{_o=-(_L);}let _S=_o;j4_=_S;}if Dj{S0_=f32(((_c.z>>bitcast<u32>(4i))&15u));}if Cj{let _P=(_l*8u);let _B=(_P+2u);let _d=textureLoad(KB,vec2<i32>(bitcast<i32>((_B&255u)),bitcast<i32>((_B>>bitcast<u32>(8i)))),0i);let _C=(_P+3u);let _J=textureLoad(KB,vec2<i32>(bitcast<i32>((_C&255u)),bitcast<i32>((_C>>bitcast<u32>(8i)))),0i);if any((_d!=vec4<f32>(0f,0f,0f,0f))){let _i=((mat2x2<f32>(vec2<f32>(_d.x,_d.y),vec2<f32>(_d.z,_d.w))*_g)+_J.xy);_a._b[0i]=(_i.x+1f);_a._b[1i]=(_i.y+1f);_a._b[2i]=(1f-_i.x);_a._b[3i]=(1f-_i.y);}else{let _k=(_J.x-0.5f);_a._b[3i]=_k;_a._b[2i]=_k;_a._b[1i]=_k;_a._b[0i]=_k;}}if (_u==1u){N0_=unpack4x8unorm(_c.w);}else{let _p=(_l*8u);let _h=textureLoad(KB,vec2<i32>(bitcast<i32>((_p&255u)),bitcast<i32>((_p>>bitcast<u32>(8i)))),0i);let _x=(_p+1u);let _f=textureLoad(KB,vec2<i32>(bitcast<i32>((_x&255u)),bitcast<i32>((_x>>bitcast<u32>(8i)))),0i);let _Q=((mat2x2<f32>(vec2<f32>(_h.x,_h.y),vec2<f32>(_h.z,_h.w))*_g)+_f.xy);let _R=((_f.w+(f32(_u)*0.125f))+(max(_f.z,0f)*0.00024414063f));if (_f.z<0f){_s=-(_R);}else{_s=_R;}let _3=_s;N0_=vec4<f32>(_Q.x,_Q.y,_3,(-0.75f-round((bitcast<f32>(_c.w)*255f))));}_v=Fj;if Fj{_v=((_c.z&2048u)!=0u);}let _1=_v;if _1{let _N=(_l*8u);let _E=(_N+4u);let _e=textureLoad(KB,vec2<i32>(bitcast<i32>((_E&255u)),bitcast<i32>((_E>>bitcast<u32>(8i)))),0i);let _K=(_N+5u);let _H=textureLoad(KB,vec2<i32>(bitcast<i32>((_K&255u)),bitcast<i32>((_K>>bitcast<u32>(8i)))),0i);let _z=((mat2x2<f32>(vec2<f32>(_e.x,_e.y),vec2<f32>(_e.z,_e.w))*_g)+_H.xy);_q=(1f+_H.z);if ((_c.z&4096u)!=0u){_q=(-1f-f32(((_c.z&24576u)>>bitcast<u32>(13u))));}let _9=_q;O0_=vec3<f32>(_z.x,_z.y,_9);}else{O0_=vec3<f32>(0f,0f,0f);}let _7=j.dh;let _A=j.eh;let _m=vec4<f32>(((_j.x*_7)-1f),((_j.y*_A)-sign(_A)),0f,1f);_a._t=vec4<f32>(_m.x,_m.y,((f32((((_2.z>>bitcast<u32>(16u))<<bitcast<u32>(8u))|255u))*0.000000059604645f)+0.000000029802322f),_m.w);return;}@vertex fn main(@builtin(vertex_index) _ab:u32,@location(0) MB:vec3<f32>)->VertexOutput{_0=i32(_ab);MB_1=MB;_X();let _4=_a._t;let _5=_a._b;let _T=X2_;let _W=j4_;let _6=S0_;let _Y=N0_;let _Z=O0_;return VertexOutput(_4,_5,_T,_W,_6,_Y,_Z);})WGSL",
    .usedOverrides = {{true, true, true, false, true, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "draw_depthstencil_atlas_blit.webgpu_nossbo_vert",
};
} // namespace wgsl
