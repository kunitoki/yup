#pragma once

#include "stencil_draw.exports.h"

namespace rive {
namespace pls {
namespace glsl {
const char stencil_draw[] = R"===(#ifdef V
U0(P)q0(0,H3,KB);V0 O1 P1 V1 W1 g1(TD,P,r,j,L){g B=i2(KB.xy);uint Y3=floatBitsToUint(KB.z)&0xffffu;B.z=J5(Y3);h1(B);}
#endif
#ifdef GB
F2 G2 r2(i,SC){v2(M0(0));}
#endif
)===";
} // namespace glsl
} // namespace pls
} // namespace rive