#pragma once

#include "stencil_draw.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char stencil_draw[] = R"===(#ifdef AB
U0(f0)i0(0,a4,IB);V0 P2 Q2 E3 F3 q1(UE,f0,B,n,K){f Q=F2(IB.xy);uint X5=floatBitsToUint(IB.z)&0xffffu;Q.z=S8(X5);h1(Q);}
#endif
#ifdef HB
R2 S2 e2(i,MD){f2(E1(.0));}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive