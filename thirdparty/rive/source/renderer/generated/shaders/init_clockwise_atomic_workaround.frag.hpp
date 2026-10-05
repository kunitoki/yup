#pragma once

#include "init_clockwise_atomic_workaround.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char init_clockwise_atomic_workaround_frag[] = R"===(#ifdef FB
R1
#ifndef W
B0(K2,n0);
#endif
B0(c3,m0);S1 z5(IB){y0(m0,I0(N0(m0).x,.0,.0,1.));
#ifndef W
n4(N0(n0));
#else
n4(I0(.0));
#endif
}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive