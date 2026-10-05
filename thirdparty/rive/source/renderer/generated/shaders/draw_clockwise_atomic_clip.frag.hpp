#pragma once

#include "draw_clockwise_atomic_clip.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_clockwise_atomic_clip_frag[] = R"===(#ifdef FB
R1
#ifndef W
B0(K2,n0);
#endif
he(c3,m0);S1
#ifdef JD
f4 mb(Na,Te,V0);g4
#endif
#ifdef W
#define z5 z2
#define n4(J5) K1=J5;z3
#else
#define z5 T1
#define n4(J5) y0(n0,J5);h2;
#endif
z5(IB){
#ifdef DB
q(m1,d);d A0=m1;
#else
q(S,G2);d A0=S.x;
#endif
#ifdef JD
if(JD){q(q3,O0);q(F4,c);uint a8=q3.y;uint d2=q3.x+d9(O0(floor(F4)),a8);uint z1=fe(V0,d2);d Vb;if(A0>=1.&&(z1<j.j2||z1>=(j.j2|C5))){Vb=.0;}else{d Ve=A0;d C9=A0;if(z1<j.j2){uint c8=j.j2|(C5+G7(abs(A0)));uint r3=M7(V0,d2,c8);if(r3<=j.j2){C9=.0;}else if(r3<c8){C9=ib(r3);}}if(C9>.0){uint Wb=nb(V0,d2,G7(abs(C9)));Ve=ib(Wb)+A0;}Vb=1.-Ve;}y0(m0,I0(Vb));n4(I0(1.))}else
#endif
{y0(m0,I0(A0));n4(I0(.0))}}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive