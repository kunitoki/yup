#pragma once

#include "init_clockwise_atomic_workaround.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char init_clockwise_atomic_workaround_frag[] = R"===(#ifdef EB
I1
#ifndef Q
z0(B2,q0);
#endif
z0(o3,o0);J1 M5(HB){y0(o0,A0(L0(o0).x,.0,.0,1.));
#ifndef Q
E4(L0(q0));
#else
E4(A0(.0));
#endif
}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive