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
struct Be{X1_:array<vec2<u32>>,}struct d0qd{X1_:array<u32>,}struct Ce{X1_:array<vec4<f32>>,}struct NB{Ub:f32,dd:f32,Xe:f32,Ye:f32,q5_:u32,ug:u32,Je:u32,Ke:u32,U7_:vec4<i32>,rg:vec2<f32>,ed:vec2<f32>,W1_:u32,vg:f32,Y5_:u32,P2_:f32,fd:f32,Ee:u32,y3_:f32,z3_:f32,gd:f32,og:u32,}struct p4qd{X1_:array<u32>,}@id(7) override Tg:bool=true;@id(4) override Qg:bool=true;@id(0) override Mg:bool=true;@id(1) override Ng:bool=true;@group(0)@binding(4) var<storage>TC:Be;@group(2)@binding(1) var<storage,read_write>d0_:d0qd;@group(0)@binding(5) var<storage>PB:Ce;var<private>_O:vec4<f32>;@group(0)@binding(9) var DD:texture_2d<f32>;@group(3)@binding(9) var Bb:sampler;@group(0)@binding(0) var<uniform>k:NB;@group(2)@binding(3) var<storage,read_write>p4_:p4qd;var<private>z0_1:u32;var<private>j1_1:f32;var<private>l1_:vec4<f32>;@group(3)@binding(10) var T9_:sampler;@group(0)@binding(10) var QC:texture_2d<f32>;@group(1)@binding(12) var AC:texture_2d<f32>;@group(1)@binding(14) var R5_:sampler;fn _5(){var _F:u32;var _k:bool;var _r:f32;var _A:f32;var _B:f32;var _l:f32;var _u:f32;var _E:bool;var _w:f32;var _D:u32;var _G:f32;var _v:u32;var _C:vec4<f32>;var _p:u32;var _x:vec4<f32>;var _h:vec3<f32>;let _i=_O;let _n=_i.xy;let _b=bitcast<vec2<u32>>(vec2<i32>(floor(_n)));let _3=k.q5_;let _f=bitcast<i32>((((((_b.y>>bitcast<u32>(5u))*(((_3+31u)&4294967264u)<<bitcast<u32>(5u)))+((_b.x>>bitcast<u32>(5u))<<bitcast<u32>(10u)))+(((_b.x&28u)<<bitcast<u32>(5u))+((_b.y&28u)<<bitcast<u32>(2i))))+(((_b.y&3u)<<bitcast<u32>(2i))+(_b.x&3u))));let _m=p4_.X1_[_f];let _c=(_m>>bitcast<u32>(17u));let _o=z0_1;if (_c==_o){_F=_m;}else{_F=((_o<<bitcast<u32>(17u))+65536u);}let _ae=_F;let _ag=j1_1;p4_.X1_[_f]=(_ae+bitcast<u32>(i32(round((_ag*2048f)))));_p=0u;_x=vec4<f32>(0f,0f,0f,0f);if (_c!=_o){let _R=((f32((_m&131071u))*0.00048828125f)+-32f);let _a=TC.X1_[_c];_A=_R;if ((_a.x&768u)!=0u){let _L=abs(_R);_k=Qg;if Qg{_k=((_a.x&512u)!=0u);}let _aa=_k;_r=_L;if _aa{_r=(1f-abs(((fract((_L*0.5f))*2f)+-1f)));}let _6=_r;_A=_6;}let _9=_A;let _H=clamp(_9,0f,1f);_u=_H;if Mg{let _U=(_a.x>>bitcast<u32>(16u));_l=_H;if (_U!=0u){let _Z=d0_.X1_[_f];if (_U==(_Z>>bitcast<u32>(16i))){_B=min(_H,unpack2x16float(_Z).x);}else{_B=0f;}let _1=_B;_l=_1;}let _4=_l;_u=_4;}let _P=_u;_E=Ng;if Ng{_E=((_a.x&1024u)!=0u);}let _ah=_E;_w=_P;if _ah{let _J=(_c*4u);let _g=PB.X1_[(_J+2u)];let _V=PB.X1_[(_J+3u)];let _W=_V.zw;let _Y=((abs(((mat2x2<f32>(vec2<f32>(_g.x,_g.y),vec2<f32>(_g.z,_g.w))*_n)+_V.xy))*_W)-_W);_w=min(_P,clamp((min(_Y.x,_Y.y)+0.5f),0f,1f));}let _T=_w;let _j=(_a.x&15u);if (_j<=1u){let _Q=(Mg&&(_j==0u));_D=0u;if _Q{_D=(_a.y|pack2x16float(vec2<f32>(_T,0f)));}let _7=_D;_v=_7;_C=select(unpack4x8unorm(_a.y),vec4<f32>(0f,0f,0f,0f),vec4(_Q));}else{let _X=(_c*4u);let _e=PB.X1_[_X];let _z=PB.X1_[(_X+1u)];let _M=((mat2x2<f32>(vec2<f32>(_e.x,_e.y),vec2<f32>(_e.z,_e.w))*_n)+_z.xy);if (_j==2u){_G=_M.x;}else{_G=length(_M);}let _ad=_G;let _af=textureSampleLevel(DD,Bb,vec2<f32>(((clamp(_ad,0f,1f)*_z.z)+_z.w),bitcast<f32>(_a.y)),0f);_v=0u;_C=_af;}let _8=_v;let _S=_C;let _N=(_S.w*_T);let _y=(_S.xyz*_N);_p=_8;_x=vec4<f32>(_y.x,_y.y,_y.z,_N);}let _K=_p;let _d=_x;let _I=_d.xyz;let _0=k.y3_;let _ac=k.z3_;if Tg{_h=(vec3(((fract((52.982918f*fract(((0.06711056f*_i.x)+(0.00583715f*_i.y)))))*_0)+_ac))+_I);}else{_h=_I;}let _t=_h;let _s=vec4<f32>(_t.x,_d.y,_d.z,_d.w);let _q=vec4<f32>(_s.x,_t.y,_s.z,_s.w);l1_=vec4<f32>(_q.x,_q.y,_t.z,_q.w);if (_K!=0u){d0_.X1_[_f]=_K;}return;}@fragment fn main(@builtin(position) _ab:vec4<f32>,@location(1)@interpolate(flat) z0_:u32,@location(0)@interpolate(flat) j1_:f32)->@location(0) vec4<f32>{_O=_ab;z0_1=z0_;j1_1=j1_;_5();let _2=l1_;return _2;})WGSL",
    .usedOverrides = {{true, true, false, false, true, false, false, true, false, false, false, false, false, false}},
    .label = "atomic_draw_interior_triangles.webgpu_fixedcolor_frag",
};
} // namespace wgsl
