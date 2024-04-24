#pragma once

#include "stencil_draw.exports.h"

namespace rive {
namespace pls {
namespace glsl {
const char stencil_draw[] = R"===(#ifdef W
T0(P)q0(0,G3,LB);U0 N1 O1 U1 V1 e1(VD,P,r,j,L){g B=h2(LB.xy);uint X3=floatBitsToUint(LB.z)&0xffffu;B.z=I5(X3);f1(B);}
#endif
#ifdef HB
E2 F2 q2(i,UC){r2(M0(0));}
#endif
)===";
} // namespace glsl
} // namespace pls
} // namespace rive