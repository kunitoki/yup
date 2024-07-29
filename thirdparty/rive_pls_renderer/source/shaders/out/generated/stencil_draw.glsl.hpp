#pragma once

#include "stencil_draw.exports.h"

namespace rive {
namespace pls {
namespace glsl {
const char stencil_draw[] = R"===(#ifdef Y
U0(R)v0(0,M3,LB);V0 Q1 R1 Y1 Z1 g1(VD,R,v,k,O){g C=k2(LB.xy);uint e4=floatBitsToUint(LB.z)&0xffffu;C.z=S5(e4);h1(C);}
#endif
#ifdef HB
K2 L2 w2(j,UC){x2(j0(0));}
#endif
)===";
} // namespace glsl
} // namespace pls
} // namespace rive