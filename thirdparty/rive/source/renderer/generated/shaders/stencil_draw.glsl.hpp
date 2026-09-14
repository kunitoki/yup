#pragma once

#include "stencil_draw.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char stencil_draw[] = R"===(#ifdef CB
A1(a0)p0(0,L3,KB);B1 R3 S3 A4 B4 C1(OF,a0,G,v,T){q0(v,G,KB,L3);g N=K3(KB.xy);uint Z6=floatBitsToUint(KB.z)&0xffffu;N.z=ca(Z6);D1(N);}
#endif
#ifdef FB
B3 C3 Y2(i,DE){G2(B0(.0));}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive