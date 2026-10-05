#pragma once

#include "clear_clockwise_atomic_clip.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char clear_clockwise_atomic_clip[] = R"===(#ifdef BB
c1(d0) K(0,c4,MB);d1 v1(RB,d0,D,G,r){L(G,D,MB,c4);e I=H3(MB.xy);w1(I);}
#endif
#ifdef FB
R1
#ifndef W
B0(K2,n0);
#endif
B0(c3,m0);S1 z5(IB){y0(m0,I0(.0,.0,.0,1.));n4(I0(.0));}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive