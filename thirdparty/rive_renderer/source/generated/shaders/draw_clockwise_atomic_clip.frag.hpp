#pragma once

#include "draw_clockwise_atomic_clip.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_clockwise_atomic_clip_frag[] = R"===(#ifdef EB
K1
#ifndef K
p0(P2,j0);
#endif
ld(Q2,e0);L1
#ifdef SC
K3 Ea(ga,Td,S0);L3
#endif
#ifdef K
#define c5 o2
#define T3(o5) m1=o5;i3
#else
#define c5 v1
#define T3(o5) C0(j0,o5);c2;
#endif
c5(HB){
#ifdef CB
B(l1,d);d q0=l1;
#else
B(I,z2);d q0=I.x;
#endif
#ifdef SC
if(SC){B(a3,W0);B(i4,c);uint N7=a3.y;uint T1=a3.x+ya(W0(floor(i4)),N7);uint p1=kd(S0,T1);d Ya;if(q0>=1.&&(p1<k.W1||p1>=(k.W1|i5))){Ya=.0;}else{d Vd=q0;d g9=q0;if(p1<k.W1){uint O7=k.W1|(i5+q7(abs(q0)));uint c3=A7(S0,T1,O7);if(c3<=k.W1){g9=.0;}else if(c3<O7){g9=Aa(c3);}}if(g9>.0){uint Za=Fa(S0,T1,q7(abs(g9)));Vd=Aa(Za)+q0;}Ya=1.-Vd;}C0(e0,B0(Ya));T3(B0(1.))}else
#endif
{C0(e0,B0(q0));T3(B0(.0))}}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive