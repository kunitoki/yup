#pragma once

#include "draw_clockwise_atomic_borrowed_coverage.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_clockwise_atomic_borrowed_coverage_frag[] = R"===(#ifdef FB
f4 mb(Na,Te,V0);g4 void main(){
#ifdef DB
T(m1,d);
#else
T(S,G2);
#endif
q(q3,O0);q(F4,c);d A0=
#ifdef DB
m1;
#else
Ub(S);
#endif
O0 N6=O0(floor(F4));uint a8=q3.y;uint d2=q3.x+d9(N6,a8);uint Ue=G7(abs(A0));uint c8=j.j2|(C5-Ue);uint r3=M7(V0,d2,c8);if(r3>=j.j2){uint Yi=r3-max(r3,c8);nb(V0,d2,Yi-Ue);}}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive