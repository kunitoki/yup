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

inline constexpr Shader atomic_draw_atlas_blit_webgpu_fixedcolor_frag = {
    .source = R"WGSL(
struct rd{k2_:array<vec4<u32>>,}struct o0lf{k2_:array<u32>,}struct Fg{k2_:array<vec4<f32>>,}struct RB{Od:f32,Ze:f32,dh:f32,eh:f32,X5_:u32,Ka:u32,Og:u32,Qg:u32,Q8_:vec4<i32>,Yi:vec2<f32>,af:vec2<f32>,z2_:u32,cj:f32,B6_:u32,m3_:f32,bf:f32,Hg:u32,J3_:f32,K3_:f32,cf:f32,Vi:u32,Ja:u32,X8_:f32,Y8_:f32,}struct e5lf{k2_:array<u32>,}@id(8) override Jj:bool=true;@id(9) override Kj:bool=true;@id(5) override Gj:bool=true;@id(0) override Bj:bool=true;@id(1) override Cj:bool=true;@id(2) override Dj:bool=true;@group(0)@binding(2) var<storage>JB:rd;@group(2)@binding(1) var<storage,read_write>o0_:o0lf;@group(0)@binding(3) var<storage>KB:Fg;var<private>_O:vec4<f32>;@group(0)@binding(0) var<uniform>j:RB;@group(0)@binding(8) var AD:texture_2d<f32>;@group(3)@binding(8) var Z8_:sampler;@group(2)@binding(3) var<storage,read_write>e5_:e5lf;var<private>J0_1:u32;@group(0)@binding(10) var ID:texture_2d<f32>;@group(3)@binding(10) var Za:sampler;var<private>X2_1:vec2<f32>;var<private>S1_:vec4<f32>;@group(3)@binding(9) var kb:sampler;@group(0)@binding(9) var BD:texture_2d<f32>;@group(1)@binding(11) var SB:texture_2d<f32>;@group(1)@binding(13) var x4_:sampler;fn _8(){var _k:bool;var _v:f32;var _F:f32;var _E:f32;var _w:f32;var _n:f32;var _s:bool;var _r:f32;var _q:f32;var _C:f32;var _t:vec2<f32>;var _u:vec4<f32>;var _j:u32;var _l:u32;var _A:vec4<f32>;var _x:vec3<f32>;let _D=_O;let _z=_D.xy;let _b=bitcast<vec2<u32>>(vec2<i32>(floor(_z)));let _ac=j.X5_;let _g=bitcast<i32>((((((_b.y>>bitcast<u32>(5u))*(((_ac+31u)&4294967264u)<<bitcast<u32>(5u)))+((_b.x>>bitcast<u32>(5u))<<bitcast<u32>(10u)))+(((_b.x&28u)<<bitcast<u32>(5u))+((_b.y&28u)<<bitcast<u32>(2i))))+(((_b.y&3u)<<bitcast<u32>(2i))+(_b.x&3u))));let _0=e5_.k2_[_g];let _p=(_0>>bitcast<u32>(17u));let _aa=J0_1;let _9=X2_1;let _4=textureSampleLevel(ID,Za,_9,0f);e5_.k2_[_g]=(((_aa<<bitcast<u32>(17u))+65536u)+bitcast<u32>(i32(round((clamp(_4.x,0f,1f)*2048f)))));let _S=((f32((_0&131071u))*0.00048828125f)+-32f);let _a=JB.k2_[((_p*4u)+1u)];_F=_S;if ((_a.z&768u)!=0u){let _P=abs(_S);_k=Gj;if Gj{_k=((_a.z&512u)!=0u);}let _6=_k;_v=_P;if _6{_v=(1f-abs(((fract((_P*0.5f))*2f)+-1f)));}let _an=_v;_F=_an;}let _al=_F;let _G=clamp(_al,0f,1f);_n=_G;if Bj{let _W=(_a.z>>bitcast<u32>(16u));_w=_G;if (_W!=0u){let _J=o0_.k2_[_g];if (_W==(_J>>bitcast<u32>(16i))){_E=min(_G,unpack2x16float(_J).x);}else{_E=0f;}let _2=_E;_w=_2;}let _ae=_w;_n=_ae;}let _R=_n;_s=Cj;if Cj{_s=((_a.z&1024u)!=0u);}let _ab=_s;_r=_R;if _ab{let _M=(_p*8u);let _d=KB.k2_[(_M+2u)];let _Q=KB.k2_[(_M+3u)];let _Y=_Q.zw;let _N=((abs(((mat2x2<f32>(vec2<f32>(_d.x,_d.y),vec2<f32>(_d.z,_d.w))*_z)+_Q.xy))*_Y)-_Y);_r=min(_R,clamp((min(_N.x,_N.y)+0.5f),0f,1f));}let _1=_r;let _H=(_a.z&15u);if (_H<=1u){let _L=(Bj&&(_H==0u));_j=0u;if _L{_j=(_a.w|pack2x16float(vec2<f32>(_1,0f)));}let _7=_j;_l=_7;_A=select(unpack4x8unorm(_a.w),vec4<f32>(0f,0f,0f,0f),vec4(_L));}else{let _Z=(_p*8u);let _f=KB.k2_[_Z];let _h=KB.k2_[(_Z+1u)];let _m=((mat2x2<f32>(vec2<f32>(_f.x,_f.y),vec2<f32>(_f.z,_f.w))*_z)+_h.xy);let _K=f32(_H);let _aj=j.X8_;let _ai=j.Y8_;let _V=((_h.w*_aj)+_ai);switch bitcast<i32>(0u){default:{if (Kj&&(_K==4f)){let _T=normalize(_m);let _X=acos(clamp(_T.x,-1f,1f));if (_T.y>=0f){_C=_X;}else{_C=-(_X);}let _ag=_C;_t=vec2<f32>((_ag*0.15915494f),_V);break;}else{if (_K==2f){_q=_m.x;}else{_q=length(_m);}let _3=_q;_t=vec2<f32>(((clamp(_3,0f,1f)*select(0.001953125f,0.9980469f,(_h.z<0f)))+((max(0f,_h.z)*0.001953125f)+0.0009765625f)),_V);break;}}}let _ak=_t;let _e=textureSampleLevel(AD,Z8_,_ak,0f);_u=_e;if!((Dj&&(((_a.z>>bitcast<u32>(4i))&15u)!=0u))){let _B=(_e.xyz*_e.w);_u=vec4<f32>(_B.x,_B.y,_B.z,(_e.w*abs(bitcast<f32>(_a.w))));}let _af=_u;_l=0u;_A=_af;}let _I=_l;let _5=_A;let _c=(_5*_1);let _U=_c.xyz;let _ah=j.J3_;let _ad=j.K3_;if (Jj&&(_c.w!=0f)){_x=(vec3(((fract((52.982918f*fract(((0.06711056f*_D.x)+(0.00583715f*_D.y)))))*_ah)+_ad))+_U);}else{_x=_U;}let _o=_x;let _i=vec4<f32>(_o.x,_c.y,_c.z,_c.w);let _y=vec4<f32>(_i.x,_o.y,_i.z,_i.w);S1_=vec4<f32>(_y.x,_y.y,_o.z,_y.w);if (_I!=0u){o0_.k2_[_g]=_I;}return;}@fragment fn main(@builtin(position) _am:vec4<f32>,@location(1)@interpolate(flat,either) J0_:u32,@location(0) X2_:vec2<f32>)->@location(0) vec4<f32>{_O=_am;J0_1=J0_;X2_1=X2_;_8();let _e7=S1_;return _e7;})WGSL",
    .usedOverrides = {{true, true, true, false, false, true, false, false, true, true, false, false, false, false, false, false, false, false}},
    .label = "atomic_draw_atlas_blit.webgpu_fixedcolor_frag",
};
} // namespace wgsl
