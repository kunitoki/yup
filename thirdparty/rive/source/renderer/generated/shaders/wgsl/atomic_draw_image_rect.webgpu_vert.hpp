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
struct RB{Od:f32,Ze:f32,dh:f32,eh:f32,X5_:u32,Ka:u32,Og:u32,Qg:u32,Q8_:vec4<i32>,Yi:vec2<f32>,af:vec2<f32>,z2_:u32,cj:f32,B6_:u32,m3_:f32,bf:f32,Hg:u32,J3_:f32,K3_:f32,cf:f32,Vi:u32,Ja:u32,X8_:f32,Y8_:f32,}struct _B{@builtin(position) _q:vec4<f32>,_al:f32,_am:array<f32,1>,_ak:array<f32,1>,}struct rd{k2_:array<vec4<u32>>,}struct Fg{k2_:array<vec4<f32>>,}struct Ei{k2_:array<vec4<u32>>,}struct VertexOutput{@location(1) member:f32,@location(0) member_1:vec2<f32>,@location(3) member_2:vec4<f32>,@location(4)@interpolate(flat,either) member_3:vec4<f32>,@location(5)@interpolate(flat,either) member_4:u32,@location(6)@interpolate(flat,either) member_5:u32,@location(2) member_6:vec4<f32>,@builtin(position) _q:vec4<f32>,}@id(1) override Cj:bool=true;var<private>_2:i32;var<private>_ai:i32;var<private>HC_1:vec4<f32>;var<private>E5_:f32;var<private>AC_1:vec4<f32>;var<private>TD_1:vec4<f32>;var<private>p2_:vec2<f32>;var<private>FD_1:vec4<f32>;var<private>QB_1:vec4<f32>;var<private>Y0_:vec4<f32>;var<private>VB_1:vec4<f32>;var<private>a2_:vec4<f32>;var<private>BC_1:u32;var<private>U3_:u32;var<private>CC_1:u32;var<private>Q1_:u32;var<private>DC_1:u32;@group(0)@binding(0) var<uniform>j:RB;var<private>GD_1:vec4<f32>;var<private>UD_1:vec4<f32>;var<private>F5_:vec4<f32>;var<private>_G:_B=_B(vec4<f32>(0f,0f,0f,1f),1f,array<f32,1>(),array<f32,1>());@group(0)@binding(7) var WB:texture_2d<u32>;@group(0)@binding(9) var BD:texture_2d<f32>;@group(0)@binding(2) var<storage>JB:rd;@group(0)@binding(3) var<storage>KB:Fg;@group(0)@binding(4) var<storage>CD:Ei;@group(3)@binding(9) var kb:sampler;var<private>NC_1:u32;fn _ag(){var _u:bool;var _x:vec2<f32>;var _v:vec2<f32>;var _h:vec2<f32>;var _l:vec2<f32>;var _i:bool;var _p:vec4<f32>;var _m:f32;let _J=HC_1[2u];let _C=(_J==0f);_u=_C;if!(_C){let _ab=HC_1[3u];_u=(_ab==0f);}let _s=_u;E5_=select(1f,0f,_s);let _w=HC_1;let _r=_w.xy;let _c=AC_1;let _R=vec2<f32>(_c.x,_c.y);let _O=vec2<f32>(_c.z,_c.w);let _S=mat2x2<f32>(_R,_O);let _a=transpose(_8(_S));_h=_r;if!(_s){let _y=((0.5f*(abs(_a[1].x)+abs(_a[1].y)))/dot(_O,_a[1]));if (_y>=0.5f){let _5=E5_;E5_=(_5*(0.5f/_y));_x=vec2<f32>(0.5f,_r.y);}else{_x=vec2<f32>((_w.x+(_y*_J)),_r.y);}let _t=_x;let _n=((0.5f*(abs(_a[0].x)+abs(_a[0].y)))/dot(_R,_a[0]));if (_n>=0.5f){let _0=E5_;E5_=(_0*(0.5f/_n));_v=vec2<f32>(_t.x,0.5f);}else{let _ah=HC_1[3u];_v=vec2<f32>(_t.x,(_t.y+(_n*_ah)));}let _T=_v;_h=_T;}let _H=_h;let _g=TD_1;let _K=FD_1;p2_=((mat2x2<f32>(vec2<f32>(_g.x,_g.y),vec2<f32>(_g.z,_g.w))*_H)+_K.xy);let _z=QB_1;let _P=((_S*_H)+_z.xy);_l=_P;if _s{let _b=(_a*_w.zw);_l=(_P+((_b*((abs(_b.x)+abs(_b.y))/dot(_b,_b)))*0.5f));}let _f=_l;if Cj{let _d=VB_1;let _I=vec2<f32>(_d.x,_d.y);let _F=vec2<f32>(_d.z,_d.w);switch bitcast<i32>(0u){default:{let _j=(abs(_I)+abs(_F));let _L=(_j.x!=0f);_i=_L;if _L{_i=(_j.y!=0f);}let _V=_i;if _V{let _o=((mat2x2<f32>(_I,_F)*_f)+_z.zw);let _Q=-(_o);let _A=(vec2<f32>(1f,1f)/_j).xyxy;_p=(((vec4<f32>(_o.x,_o.y,_Q.x,_Q.y)*_A)+_A)+vec4<f32>(0.5f,0.5f,0.5f,0.5f));break;}else{_p=_z.zwzw;break;}}}let _W=_p;Y0_=_W;}let _1=BC_1;a2_=unpack4x8unorm(_1);let _9=CC_1;U3_=_9;let _U=DC_1;Q1_=_U;let _4=j.dh;let _M=j.eh;let _aj=GD_1[2u];let _E=bitcast<u32>(_aj);if (_E!=0u){let _e=UD_1;let _k=GD_1;let _D=((mat2x2<f32>(vec2<f32>(_e.x,_e.y),vec2<f32>(_e.z,_e.w))*_f)+_K.zw);let _N=((_k.y+(f32(_E)*0.125f))+(max(_k.x,0f)*0.00024414063f));if (_k.x<0f){_m=-(_N);}else{_m=_N;}let _aa=_m;F5_=vec4<f32>(_D.x,_D.y,_aa,(-0.75f-round(255f)));}else{F5_=vec4<f32>(0f,0f,0f,0f);}_G._q=vec4<f32>(((_f.x*_4)-1f),((_f.y*_M)-sign(_M)),0f,1f);return;}@vertex fn main(@builtin(vertex_index) _af:u32,@builtin(instance_index) _3:u32,@location(0) HC:vec4<f32>,@location(2) AC:vec4<f32>,@location(9) TD:vec4<f32>,@location(11) FD:vec4<f32>,@location(4) QB:vec4<f32>,@location(3) VB:vec4<f32>,@location(5) BC:u32,@location(6) CC:u32,@location(7) DC:u32,@location(12) GD:vec4<f32>,@location(10) UD:vec4<f32>,@location(8) NC:u32)->VertexOutput{_2=i32(_af);_ai=i32(_3);HC_1=HC;AC_1=AC;TD_1=TD;FD_1=FD;QB_1=QB;VB_1=VB;BC_1=BC;CC_1=CC;DC_1=DC;GD_1=GD;UD_1=UD;NC_1=NC;_ag();let _X=E5_;let _6=p2_;let _Y=Y0_;let _ac=a2_;let _ad=U3_;let _Z=Q1_;let _ae=F5_;let _7=_G._q;return VertexOutput(_X,_6,_Y,_ac,_ad,_Z,_ae,_7);}fn _8(m:mat2x2<f32>)->mat2x2<f32>{var adj:mat2x2<f32>;adj[0][0]=m[1][1];adj[0][1]=-m[0][1];adj[1][0]=-m[1][0];adj[1][1]=m[0][0];let det:f32=m[0][0]*m[1][1]-m[1][0]*m[0][1];return adj*(1/det);})WGSL",
    .usedOverrides = {{false, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false}},
    .label = "atomic_draw_image_rect.webgpu_vert",
};
} // namespace wgsl
