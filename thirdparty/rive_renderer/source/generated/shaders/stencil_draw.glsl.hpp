#pragma once

#include "stencil_draw.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char stencil_draw[] = R"===(#ifdef AB
L0(e0)h0(0,R3,IB);M0 H2 I2 r3 v3 Z0(RE,e0,o,l,I){e Q=v2(IB.xy);uint m5=floatBitsToUint(IB.z)&0xffffu;Q.z=N7(m5);U0(Q);}
#endif
#ifdef HB
J2 L2 T1(i,JD){U1(f2(.0));}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive