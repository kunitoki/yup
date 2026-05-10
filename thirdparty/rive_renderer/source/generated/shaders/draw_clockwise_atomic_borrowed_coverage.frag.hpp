#pragma once

#include "draw_clockwise_atomic_borrowed_coverage.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_clockwise_atomic_borrowed_coverage_frag[] = R"===(#ifdef EB
K3 Ea(ga,Td,S0);L3 void main(){
#ifdef CB
Y(l1,d);
#else
Y(I,z2);
#endif
B(a3,W0);B(i4,c);d q0=
#ifdef CB
l1;
#else
Xa(I);
#endif
W0 y6=W0(floor(i4));uint N7=a3.y;uint T1=a3.x+ya(y6,N7);uint Ud=q7(abs(q0));uint O7=k.W1|(i5-Ud);uint c3=A7(S0,T1,O7);if(c3>=k.W1){uint oh=c3-max(c3,O7);Fa(S0,T1,oh-Ud);}}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive