#pragma once

#include "draw_clockwise_clip.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_clockwise_clip_frag[] = R"===(#ifdef EB
K1
#ifndef K
p0(P2,j0);
#endif
f1(Q2,e0);
#ifndef K
Q8(d6,f4);
#endif
f1(F6,S0);L1 v1(HB){B(S1,D);d V0=-S1.x;
#ifdef CB
B(l1,d);d q0=l1;
#else
B(I,z2);d q0=I.x;
#endif
v2;D O0;d F5,o3;
#if defined(CB)&&defined(QB)
if(QB){o3=q0;}else
#endif
{O0=unpackHalf2x16(e1(e0));F5=O0.y;d P4=F5==V0?O0.x:G0(.0);o3=P4+q0;}
#ifdef NC
d D5=S1.y;if(NC&&D5!=.0){d j4=.0;
#if defined(CB)&&defined(QB)
if(QB){O0=unpackHalf2x16(e1(e0));F5=O0.y;}
#endif
if(F5!=V0){j4=F5==D5?O0.x:.0;h1(S0,packHalf2x16(A2(j4,qf)));}else{j4=unpackHalf2x16(e1(S0)).x;X1(S0);}o3=min(o3,j4);}else
#endif
{X1(S0);}h1(e0,packHalf2x16(A2(o3,V0)));
#ifndef K
r2(j0);
#endif
w2;c2;}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive