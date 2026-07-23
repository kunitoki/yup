#pragma once

#include "clear_clockwise_atomic_clip.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char clear_clockwise_atomic_clip[] = R"===(#ifdef CB
A1(a0)p0(0,L3,KB);B1 C1(YB,a0,G,v,T){q0(v,G,KB,L3);g N=K3(KB.xy);D1(N);}
#endif
#ifdef FB
J1
#ifndef K
r0(Q2,g0);
#endif
r0(R2,d0);K1 d5(IB){v0(d0,B0(.0,.0,.0,1.));V3(B0(.0));}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive