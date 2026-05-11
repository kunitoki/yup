#pragma once

#include "stencil_draw.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char stencil_draw[] = R"===(#ifdef BB
A1(c0)r0(0,I3,JB);B1 P3 Q3 B4 C4 C1(LF,c0,G,v,S){v0(v,G,JB,I3);g O=H3(JB.xy);uint Z6=floatBitsToUint(JB.z)&0xffffu;O.z=ca(Z6);D1(O);}
#endif
#ifdef EB
y3 z3 V2(i,YD){F2(B0(.0));}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive