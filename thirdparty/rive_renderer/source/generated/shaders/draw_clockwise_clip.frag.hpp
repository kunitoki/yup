#pragma once

#include "draw_clockwise_clip.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_clockwise_clip_frag[] = R"===(#ifdef FB
J1
#ifndef K
r0(Q2,g0);
#endif
k1(R2,d0);
#ifndef K
Ka(d6,g4);
#endif
k1(F6,S0);K1 m1(IB){B(S1,D);d V0=-S1.x;
#ifdef DB
B(j1,d);d o0=j1;
#else
B(I,z2);d o0=I.x;
#endif
v2;D O0;d H5,r3;
#if defined(DB)&&defined(WB)
if(WB){r3=o0;}else
#endif
{O0=unpackHalf2x16(d1(d0));H5=O0.y;d O4=H5==V0?O0.x:G0(.0);r3=O4+o0;}
#ifdef RC
d F5=S1.y;if(RC&&F5!=.0){d k4=.0;
#if defined(DB)&&defined(WB)
if(WB){O0=unpackHalf2x16(d1(d0));H5=O0.y;}
#endif
if(H5!=V0){k4=H5==F5?O0.x:.0;f1(S0,packHalf2x16(A2(k4,vf)));}else{k4=unpackHalf2x16(d1(S0)).x;Y1(S0);}r3=min(r3,k4);}else
#endif
{Y1(S0);}f1(d0,packHalf2x16(A2(r3,V0)));
#ifndef K
r2(g0);
#endif
w2;U1;}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive