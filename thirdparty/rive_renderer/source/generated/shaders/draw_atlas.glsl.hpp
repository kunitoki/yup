#pragma once

#include "draw_atlas.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_atlas[] = R"===(#ifdef AB
L0(e0)h0(0,e,LB);h0(1,e,MB);M0
#endif
o1 o0 H(0,e,q);p1
#ifdef AB
Z0(ZD,e0,o,l,I){k0(l,o,LB,e);k0(l,o,MB,e);P(q,e);e Q;uint d0;c K;if(S5(LB,MB,I,d0,K,q g2)){D z3=r0(KB,d0*4u+2u);a0 p5=uintBitsToFloat(z3.yzw);K=K*p5.x+p5.yz;Q=m6(K,m.D9.x,m.D9.y);}else{Q=e(m.q1,m.q1,m.q1,m.q1);}X(q);U0(Q);}
#endif
#ifdef HB
#ifdef AE
T1(float,BE){Z(q,e);U1(A4(q z1));}
#endif
#ifdef CE
T1(float,DE){Z(q,e);U1(g6(q z1));}
#endif
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive