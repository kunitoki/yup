#pragma once

#include "draw_clockwise_atomic_borrowed_coverage.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_clockwise_atomic_borrowed_coverage_frag[] = R"===(#ifdef FB
N3 Ea(ga,Yd,S0);O3 void main(){
#ifdef DB
Y(j1,d);
#else
Y(I,z2);
#endif
B(e3,W0);B(j4,c);d o0=
#ifdef DB
j1;
#else
cb(I);
#endif
W0 y6=W0(floor(j4));uint N7=e3.y;uint T1=e3.x+L8(y6,N7);uint Zd=q7(abs(o0));uint O7=k.W1|(j5-Zd);uint f3=A7(S0,T1,O7);if(f3>=k.W1){uint wh=f3-max(f3,O7);Fa(S0,T1,wh-Zd);}}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive