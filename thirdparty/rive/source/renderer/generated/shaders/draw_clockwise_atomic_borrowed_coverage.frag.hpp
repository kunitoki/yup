#pragma once

#include "draw_clockwise_atomic_borrowed_coverage.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_clockwise_atomic_borrowed_coverage_frag[] = R"===(#ifdef EB
n4 pc(Ib,Vf,d1);p4 void main(){
#ifdef DB
V(v1,d);
#else
V(S,U2);
#endif
q(C3,R0);q(W4,c);d F0=
#ifdef DB
v1;
#else
Xc(S);
#endif
R0 i7=R0(floor(W4));uint I8=C3.y;uint n2=C3.x+S9(i7,I8);uint Wf=j8(abs(F0));uint J8=j.z2|(P5-Wf);uint D3=o8(d1,n2,J8);if(D3>=j.z2){uint pk=D3-max(D3,J8);qc(d1,n2,pk-Wf);}}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive