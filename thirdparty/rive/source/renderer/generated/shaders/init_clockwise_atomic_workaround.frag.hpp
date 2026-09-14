#pragma once

#include "init_clockwise_atomic_workaround.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char init_clockwise_atomic_workaround_frag[] = R"===(#ifdef FB
J1
#ifndef K
r0(Q2,g0);
#endif
r0(R2,d0);K1 d5(IB){v0(d0,B0(H0(d0).x,.0,.0,1.));
#ifndef K
V3(H0(g0));
#else
V3(B0(.0));
#endif
}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive