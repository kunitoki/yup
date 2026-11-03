#pragma once

#include "stencil_draw.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char stencil_draw[] = R"===(o2 P1
#ifdef CB
h1(d0) K(0,l4,MB);i1 A4 B4 i5 j5 n1(MG,d0,A,C,r){L(C,A,MB,l4);e I=T3(MB.xy);uint Tj=floatBitsToUint(MB.z)&0xffffu;I.z=v9(Tj,0xffu);C1(I);}
#endif
#ifdef EB
X3 Y3 v2(i,NG){w2(A0(.0));}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive