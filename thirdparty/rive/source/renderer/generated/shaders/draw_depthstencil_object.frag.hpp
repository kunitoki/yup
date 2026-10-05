#pragma once

#include "draw_depthstencil_object.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_depthstencil_object_frag[] = R"===(#ifdef FB
#ifdef NB
N3 i3(w5,l4,CC);
#ifdef N
E5(XD);
#endif
O3 x5 m4(r5) y5
#endif
j3(i,IB){
#ifdef NB
q(V5,c);q(Q1,i);
#ifdef N
q(H1,Q);
#endif
#else
q(a1,e);
#ifdef GB
q(r1,O);
#endif
#ifdef EB
q(J2,c);
#endif
#ifdef N
q(Q0,d);
#endif
#endif
#ifdef NB
i p=K7(CC,r5,V5,j.Vd)*Q1;
#else
d n=
#ifdef EB
clamp(o2(FD,ma,J2,.0).x,H0(.0),H0(1.));
#else
1.;
#endif
i p=Y7(
#ifdef GB
r1,
#endif
#ifdef N
k3(Q0),
#endif
a1 e3);
#endif
#if defined(N)&&!defined(W)
#ifdef NB
p.xyz=Q6(p);Q y3=H1;
#else
Q y3=k3(Q0);
#endif
i I1=I6(XD);p.xyz=h5(p.xyz,I1,y3)*p.w;
#endif
#ifndef NB
p*=n;
#endif
p.xyz=M2(p.xyz,p.w,f0.xy,j.L3,j.M3);P2(p);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive