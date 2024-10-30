#pragma once

#include "stencil_draw.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char stencil_draw[] = R"===(#ifdef AB
Y0(V)a0(0,I3,KB);Z0 R1 S1 f2 g2 h1(EE,V,q,n,H){g T=E2(KB.xy);uint z4=floatBitsToUint(KB.z)&0xffffu;T.z=o6(z4);i1(T);}
#endif
#ifdef EB
H2 I2 q2(i,ED){r2(C2(.0));}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive