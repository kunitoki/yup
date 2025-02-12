#pragma once

#include "draw_clockwise_image_mesh.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_clockwise_image_mesh[] = R"===(#ifdef AB
L0(P1)h0(0,c,SB);M0 L0(h2)h0(1,c,TB);M0
#endif
o1 o0 H(0,c,A0);p1
#ifdef AB
H2 I2 v4(PB,P1,Q1,h2,i2,l){k0(l,Q1,SB,c);k0(l,i2,TB,c);P(A0,c);c K=q0(r1(l0.V5),SB)+l0.G0;A0=TB;e Q=v2(K);X(A0);U0(Q);}
#endif
#ifdef HB
J2 o2(w6,I4,UB);L2 h4(I4,q3)j3 k3 T1(i,NB){Z(A0,c);i Y9=p3(UB,q3,A0);Y9.w*=l0.G2;U1(Y9);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive