#pragma once

#include "draw_clockwise_atomic_clip.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_clockwise_atomic_clip_frag[] = R"===(#ifdef EB
I1
#ifndef Q
z0(B2,q0);
#endif
mf(o3,o0);J1
#ifdef PD
n4 pc(Ib,Vf,d1);p4
#endif
#ifdef Q
#define M5 K2
#define E4(W5) S1=W5;I3
#else
#define M5 E1
#define E4(W5) y0(q0,W5);L1;
#endif
M5(HB){
#ifdef DB
q(v1,d);d F0=v1;
#else
q(S,U2);d F0=S.x;
#endif
#ifdef PD
if(PD){q(C3,R0);q(W4,c);uint I8=C3.y;uint n2=C3.x+S9(R0(floor(W4)),I8);uint G1=Q5(d1,n2);d Yc;if(F0>=1.&&(G1<j.z2||G1>=(j.z2|P5))){Yc=.0;}else{d Xf=F0;d pa=F0;if(G1<j.z2){uint J8=j.z2|(P5+j8(abs(F0)));uint D3=o8(d1,n2,J8);if(D3<=j.z2){pa=.0;}else if(D3<J8){pa=kc(D3);}}if(pa>.0){uint Zc=qc(d1,n2,j8(abs(pa)));Xf=kc(Zc)+F0;}Yc=1.-Xf;}y0(o0,A0(Yc));E4(A0(1.))}else
#endif
{y0(o0,A0(F0));E4(A0(.0))}}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive