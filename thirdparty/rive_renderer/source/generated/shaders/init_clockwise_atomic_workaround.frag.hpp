#pragma once

#include "init_clockwise_atomic_workaround.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char init_clockwise_atomic_workaround_frag[] = R"===(#ifdef EB
K1
#ifndef K
p0(P2,j0);
#endif
p0(Q2,e0);L1 c5(HB){C0(e0,B0(H0(e0).x,.0,.0,1.));
#ifndef K
T3(H0(j0));
#else
T3(B0(.0));
#endif
}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive