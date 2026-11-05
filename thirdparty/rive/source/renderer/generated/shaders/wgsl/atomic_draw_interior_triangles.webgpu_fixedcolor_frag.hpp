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

inline constexpr Shader atomic_draw_interior_triangles_webgpu_fixedcolor_frag = {
    .source = R"WGSL(
struct rd{k2_:array<vec4<u32>>,}struct o0lf{k2_:array<u32>,}struct Fg{k2_:array<vec4<f32>>,}struct RB{Od:f32,Ze:f32,dh:f32,eh:f32,X5_:u32,Ka:u32,Og:u32,Qg:u32,Q8_:vec4<i32>,Yi:vec2<f32>,af:vec2<f32>,z2_:u32,cj:f32,B6_:u32,m3_:f32,bf:f32,Hg:u32,J3_:f32,K3_:f32,cf:f32,Vi:u32,Ja:u32,X8_:f32,Y8_:f32,}struct e5lf{k2_:array<u32>,}@id(8) override Jj:bool=true;@id(9) override Kj:bool=true;@id(5) override Gj:bool=true;@id(0) override Bj:bool=true;@id(1) override Cj:bool=true;@id(2) override Dj:bool=true;@group(0)@binding(2) var<storage>JB:rd;@group(2)@binding(1) var<storage,read_write>o0_:o0lf;@group(0)@binding(3) var<storage>KB:Fg;var<private>_U:vec4<f32>;@group(0)@binding(0) var<uniform>j:RB;@group(0)@binding(8) var AD:texture_2d<f32>;@group(3)@binding(8) var Z8_:sampler;@group(2)@binding(3) var<storage,read_write>e5_:e5lf;var<private>J0_1:u32;var<private>v1_1:f32;var<private>S1_:vec4<f32>;@group(3)@binding(9) var kb:sampler;@group(0)@binding(9) var BD:texture_2d<f32>;@group(1)@binding(11) var SB:texture_2d<f32>;@group(1)@binding(13) var x4_:sampler;fn _as(){var _H:u32;var _L:bool;var _B:f32;var _q:f32;var _M:f32;var _o:f32;var _A:f32;var _D:bool;var _y:f32;var _G:f32;var _v:f32;var _s:vec2<f32>;var _p:vec4<f32>;var _K:u32;var _w:u32;var _m:vec4<f32>;var _j:u32;var _F:vec4<f32>;var _n:vec3<f32>;let _l=_U;let _I=_l.xy;let _b=bitcast<vec2<u32>>(vec2<i32>(floor(_I)));let _ai=j.X5_;let _i=bitcast<i32>((((((_b.y>>bitcast<u32>(5u))*(((_ai+31u)&4294967264u)<<bitcast<u32>(5u)))+((_b.x>>bitcast<u32>(5u))<<bitcast<u32>(10u)))+(((_b.x&28u)<<bitcast<u32>(5u))+((_b.y&28u)<<bitcast<u32>(2i))))+(((_b.y&3u)<<bitcast<u32>(2i))+(_b.x&3u))));let _k=e5_.k2_[_i];let _d=(_k>>bitcast<u32>(17u));let _E=J0_1;if (_d==_E){_H=_k;}else{_H=((_E<<bitcast<u32>(17u))+65536u);}let _ac=_H;let _ab=v1_1;e5_.k2_[_i]=(_ac+bitcast<u32>(i32(round((_ab*2048f)))));_j=0u;_F=vec4<f32>(0f,0f,0f,0f);if (_d!=_E){let _T=((f32((_k&131071u))*0.00048828125f)+-32f);let _a=JB.k2_[((_d*4u)+1u)];_q=_T;if ((_a.z&768u)!=0u){let _R=abs(_T);_L=Gj;if Gj{_L=((_a.z&512u)!=0u);}let _aq=_L;_B=_R;if _aq{_B=(1f-abs(((fract((_R*0.5f))*2f)+-1f)));}let _an=_B;_q=_an;}let _ae=_q;let _t=clamp(_ae,0f,1f);_A=_t;if Bj{let _P=(_a.z>>bitcast<u32>(16u));_o=_t;if (_P!=0u){let _2=o0_.k2_[_i];if (_P==(_2>>bitcast<u32>(16i))){_M=min(_t,unpack2x16float(_2).x);}else{_M=0f;}let _al=_M;_o=_al;}let _aa=_o;_A=_aa;}let _N=_A;_D=Cj;if Cj{_D=((_a.z&1024u)!=0u);}let _ag=_D;_y=_N;if _ag{let _0=(_d*8u);let _h=KB.k2_[(_0+2u)];let _X=KB.k2_[(_0+3u)];let _S=_X.zw;let _1=((abs(((mat2x2<f32>(vec2<f32>(_h.x,_h.y),vec2<f32>(_h.z,_h.w))*_I)+_X.xy))*_S)-_S);_y=min(_N,clamp((min(_1.x,_1.y)+0.5f),0f,1f));}let _W=_y;let _J=(_a.z&15u);if (_J<=1u){let _5=(Bj&&(_J==0u));_K=0u;if _5{_K=(_a.w|pack2x16float(vec2<f32>(_W,0f)));}let _6=_K;_w=_6;_m=select(unpack4x8unorm(_a.w),vec4<f32>(0f,0f,0f,0f),vec4(_5));}else{let _O=(_d*8u);let _e=KB.k2_[_O];let _f=KB.k2_[(_O+1u)];let _z=((mat2x2<f32>(vec2<f32>(_e.x,_e.y),vec2<f32>(_e.z,_e.w))*_I)+_f.xy);let _4=f32(_J);let _9=j.X8_;let _ao=j.Y8_;let _Q=((_f.w*_9)+_ao);switch bitcast<i32>(0u){default:{if (Kj&&(_4==4f)){let _V=normalize(_z);let _Y=acos(clamp(_V.x,-1f,1f));if (_V.y>=0f){_v=_Y;}else{_v=-(_Y);}let _am=_v;_s=vec2<f32>((_am*0.15915494f),_Q);break;}else{if (_4==2f){_G=_z.x;}else{_G=length(_z);}let _ah=_G;_s=vec2<f32>(((clamp(_ah,0f,1f)*select(0.001953125f,0.9980469f,(_f.z<0f)))+((max(0f,_f.z)*0.001953125f)+0.0009765625f)),_Q);break;}}}let _ap=_s;let _g=textureSampleLevel(AD,Z8_,_ap,0f);_p=_g;if!((Dj&&(((_a.z>>bitcast<u32>(4i))&15u)!=0u))){let _u=(_g.xyz*_g.w);_p=vec4<f32>(_u.x,_u.y,_u.z,(_g.w*abs(bitcast<f32>(_a.w))));}let _ak=_p;_w=0u;_m=_ak;}let _aj=_w;let _ar=_m;_j=_aj;_F=(_ar*_W);}let _Z=_j;let _c=_F;let _3=_c.xyz;let _ad=j.J3_;let _af=j.K3_;if (Jj&&(_c.w!=0f)){_n=(vec3(((fract((52.982918f*fract(((0.06711056f*_l.x)+(0.00583715f*_l.y)))))*_ad)+_af))+_3);}else{_n=_3;}let _x=_n;let _r=vec4<f32>(_x.x,_c.y,_c.z,_c.w);let _C=vec4<f32>(_r.x,_x.y,_r.z,_r.w);S1_=vec4<f32>(_C.x,_C.y,_x.z,_C.w);if (_Z!=0u){o0_.k2_[_i]=_Z;}return;}@fragment fn main(@builtin(position) _7:vec4<f32>,@location(1)@interpolate(flat,either) J0_:u32,@location(0)@interpolate(flat,either) v1_:f32)->@location(0) vec4<f32>{_U=_7;J0_1=J0_;v1_1=v1_;_as();let _8=S1_;return _8;})WGSL",
    .usedOverrides = {{true, true, true, false, false, true, false, false, true, true, false, false, false, false, false, false, false, false}},
    .label = "atomic_draw_interior_triangles.webgpu_fixedcolor_frag",
};
} // namespace wgsl
