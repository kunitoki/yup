#pragma once

#include "clear_clockwise_atomic_clip.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char clear_clockwise_atomic_clip[] = R"===(#ifdef CB
h1(d0) K(0,l4,MB);i1 n1(UB,d0,A,C,r){L(C,A,MB,l4);e I=T3(MB.xy);C1(I);}
#endif
#ifdef EB
I1
#ifndef Q
z0(B2,q0);
#endif
z0(o3,o0);J1 M5(HB){y0(o0,A0(.0,.0,.0,1.));E4(A0(.0));}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive