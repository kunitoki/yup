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

inline constexpr Shader atomic_draw_image_rect_webgpu_vert = {
    .source = R"WGSL(
struct LC{r9_:vec4<f32>,c2_:vec2<f32>,x4_:f32,ki:f32,k2_:vec4<f32>,D2_:vec2<f32>,V0_:u32,n2_:u32,Z6_:u32,}struct NB{Ub:f32,dd:f32,Xe:f32,Ye:f32,q5_:u32,ug:u32,Je:u32,Ke:u32,U7_:vec4<i32>,rg:vec2<f32>,ed:vec2<f32>,W1_:u32,vg:f32,Y5_:u32,P2_:f32,fd:f32,Ee:u32,y3_:f32,z3_:f32,gd:f32,og:u32,}struct _w{@builtin(position) _t:vec4<f32>,_6:f32,_5:array<f32,1>,_4:array<f32,1>,}struct Rf{X1_:array<vec4<u32>>,}struct Be{X1_:array<vec2<u32>>,}struct Ce{X1_:array<vec4<f32>>,}struct Sf{X1_:array<vec4<u32>>,}struct VertexOutput{@location(1) member:f32,@location(0) member_1:vec2<f32>,@location(2) member_2:vec4<f32>,@builtin(position) _t:vec4<f32>,}@id(1) override Ng:bool=true;var<private>_T:i32;var<private>_Z:i32;var<private>ZB_1:vec4<f32>;var<private>S4_:f32;@group(0)@binding(2) var<uniform>A0_:LC;var<private>U0_:vec2<f32>;var<private>N0_:vec4<f32>;@group(0)@binding(0) var<uniform>k:NB;var<private>_x:_w=_w(vec4<f32>(0f,0f,0f,1f),1f,array<f32,1>(),array<f32,1>());@group(0)@binding(8) var DC:texture_2d<u32>;@group(0)@binding(10) var QC:texture_2d<f32>;@group(0)@binding(3) var<storage>MB:Rf;@group(0)@binding(4) var<storage>TC:Be;@group(0)@binding(5) var<storage>PB:Ce;@group(0)@binding(6) var<storage>XC:Sf;@group(3)@binding(10) var T9_:sampler;fn _U(){var _o:bool;var _f:vec2<f32>;var _i:vec2<f32>;var _u:vec2<f32>;var _s:vec2<f32>;var _n:bool;var _p:vec4<f32>;let _z=ZB_1[2u];let _I=(_z==0f);_o=_I;if!(_I){let _0=ZB_1[3u];_o=(_0==0f);}let _m=_o;S4_=select(1f,0f,_m);let _e=ZB_1;let _h=_e.xy;let _c=A0_.r9_;let _F=vec2<f32>(_c.x,_c.y);let _K=vec2<f32>(_c.z,_c.w);let _v=mat2x2<f32>(_F,_K);let _a=transpose(_Q(_v));_u=_h;if!(_m){let _j=((0.5f*(abs(_a[1].x)+abs(_a[1].y)))/dot(_K,_a[1]));if (_j>=0.5f){let _1=S4_;S4_=(_1*(0.5f/_j));_f=vec2<f32>(0.5f,_h.y);}else{_f=vec2<f32>((_e.x+(_j*_z)),_h.y);}let _r=_f;let _g=((0.5f*(abs(_a[0].x)+abs(_a[0].y)))/dot(_F,_a[0]));if (_g>=0.5f){let _M=S4_;S4_=(_M*(0.5f/_g));_i=vec2<f32>(_r.x,0.5f);}else{let _3=ZB_1[3u];_i=vec2<f32>(_r.x,(_r.y+(_g*_3)));}let _L=_i;_u=_L;}let _B=_u;U0_=_B;let _Y=A0_.c2_;let _C=((_v*_B)+_Y);_s=_C;if _m{let _b=(_a*_e.zw);_s=(_C+((_b*((abs(_b.x)+abs(_b.y))/dot(_b,_b)))*0.5f));}let _k=_s;if Ng{let _d=A0_.k2_;let _E=vec2<f32>(_d.x,_d.y);let _D=vec2<f32>(_d.z,_d.w);let _G=A0_.D2_;switch bitcast<i32>(0u){default:{let _q=(abs(_E)+abs(_D));let _H=(_q.x!=0f);_n=_H;if _H{_n=(_q.y!=0f);}let _O=_n;if _O{let _l=((mat2x2<f32>(_E,_D)*_k)+_G);let _y=-(_l);let _A=(vec2<f32>(1f,1f)/_q).xyxy;_p=(((vec4<f32>(_l.x,_l.y,_y.x,_y.y)*_A)+_A)+vec4<f32>(0.5f,0.5f,0.5f,0.5f));break;}else{_p=_G.xyxy;break;}}}let _X=_p;N0_=_X;}let _R=k.Xe;let _J=k.Ye;_x._t=vec4<f32>(((_k.x*_R)-1f),((_k.y*_J)-sign(_J)),0f,1f);return;}@vertex fn main(@builtin(vertex_index) _N:u32,@builtin(instance_index) _2:u32,@location(0) ZB:vec4<f32>)->VertexOutput{_T=i32(_N);_Z=i32(_2);ZB_1=ZB;_U();let _S=S4_;let _V=U0_;let _P=N0_;let _W=_x._t;return VertexOutput(_S,_V,_P,_W);}fn _Q(m:mat2x2<f32>)->mat2x2<f32>{var adj:mat2x2<f32>;adj[0][0]=m[1][1];adj[0][1]=-m[0][1];adj[1][0]=-m[1][0];adj[1][1]=m[0][0];let det:f32=m[0][0]*m[1][1]-m[1][0]*m[0][1];return adj*(1/det);})WGSL",
    .usedOverrides = {{false, true, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "atomic_draw_image_rect.webgpu_vert",
};
} // namespace wgsl
