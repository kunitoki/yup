#pragma once

#include "draw_clockwise_image_mesh.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_clockwise_image_mesh[] = R"===(#ifdef AB
U0(a2)i0(0,c,SB);V0 U0(v2)i0(1,c,TB);V0
#endif
o1 n0 H(0,c,q0);p1
#ifdef AB
P2 Q2 N4(PB,a2,c2,v2,w2,n){l0(n,c2,SB,c);l0(n,w2,TB,c);L(q0,c);c J=C0(D1(m0.H6),SB)+m0.S0;q0=TB;f Q=F2(J);P(q0);h1(Q);}
#endif
#ifdef HB
R2 C2(Y5,W8,UB);S2 p4 G3(X8,B3)q4 w3 x3 e2(i,NB){N(q0,c);i n7=o4(UB,B3,q0);n7=E1(Y3(n7),n7.w*m0.H2);f2(n7);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive