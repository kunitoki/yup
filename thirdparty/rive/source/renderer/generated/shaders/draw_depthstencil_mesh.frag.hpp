#pragma once

#include "draw_depthstencil_mesh.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_depthstencil_mesh_frag[] = R"===(#ifdef EB
#ifdef OB
X3 Z2(J5,C4,SB);
#if defined(H)&&!defined(Q)&&!defined(LB)
K4(LD);
#endif
Y3
#ifdef LB
I1 L4(B2,LC);J1
#endif
K5 D4(x4) L5
#endif
#ifdef LB
E1(HB)
#else
v2(i,HB)
#endif
{
#ifdef OB
q(j6,c);q(a2,i);
#ifdef H
q(Q1,O);
#endif
#else
q(N0,e);
#ifdef GB
q(O0,M);
#endif
#ifdef FB
q(X2,c);
#endif
#ifdef H
q(S0,d);
#endif
#endif
#ifdef OB
i k=m8(SB,x4,j6,j.bf)*a2;
#else
d n=
#ifdef FB
clamp(r2(ID,Za,X2,.0).x,B0(.0),B0(1.));
#else
1.;
#endif
i k=G8(
#ifdef GB
O0,
#endif
#ifdef H
a3(S0),
#endif
N0 q3);
#endif
#if defined(H)&&!defined(Q)
#ifdef OB
k.xyz=o6(k);O c2=Q1;
#else
O c2=a3(S0);
#endif
#ifdef LB
i X0=e4(LC);
#else
i X0=J4(LD);
#endif
k.xyz=Y4(k.xyz,X0,c2)*k.w;
#elif defined(LB)
i X0=e4(LC);
#endif
#ifndef OB
k*=n;
#endif
k.xyz=M2(k.xyz,k.w,Y.xy,j.J3,j.K3);
#ifdef LB
if(any(notEqual(k,A0(.0)))){r5(LC,Va(k,X0));}
#endif
#ifdef LB
L1
#else
w2(k);
#endif
}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive