#pragma once

#include "draw_clockwise_path.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_clockwise_path_frag[] = R"===(#ifdef FB
J1
#ifndef K
r0(Q2,g0);
#endif
k1(R2,d0);
#ifndef K
Ka(d6,z6);
#endif
k1(F6,S0);K1
#ifdef K
o2(IB)
#else
m1(IB)
#endif
{B(i1,g);
#ifdef DB
B(j1,d);
#else
B(I,z2);
#endif
B(z0,d);
#ifdef O
B(S1,D);
#endif
#ifdef AB
B(N0,g);
#endif
#ifdef GB
B(Z1,d);
#endif
d o0=
#ifdef DB
j1;
#else
cb(I);
#endif
i F0;d H1;
#if defined(DB)&&defined(WB)
if(!WB)
#endif
{F0=M7(i1,1. S2);H1=1.;
#ifdef AB
if(AB){d hb=g3(Y4(N0));H1=min(hb,H1);}
#endif
}v2;
#if defined(DB)&&defined(WB)
if(WB){f1(S0,packHalf2x16(A2(o0,z0)));
#ifndef K
r2(g0);
#endif
}else
#endif
{D N4=unpackHalf2x16(d1(S0));d h9=N4.y;d O4=h9==z0?N4.x:G0(.0);d ee=
#ifndef DB
Q5(I)?max(O4,o0):
#endif
O4+o0;
#ifdef O
if(O&&S1.x!=.0){D O0=unpackHalf2x16(d1(d0));d H5=O0.y;d ib=H5==S1.x?O0.x:G0(.0);H1=min(ib,H1);}
#endif
H1=max(H1,.0);d V1=W9(O4,.0,H1);d G1=W9(ee,.0,H1);
#ifdef JB
d G5;if(JB){G5=Z9(S.xy,k.y3,k.z3);}
#endif
#ifndef K
i L1=H0(g0);
#ifdef GB
if(GB){if(Z1!=V5(M5)&&G1!=.0){if(V1==.0){F0.xyz=Q4(F0.xyz,L1,W5(Z1));
#ifndef DB
if(G1<H1){r P7=F0.xyz;
#ifdef JB
if(JB){P7+=G5*k.gd;}
#endif
v0(z6,B0(P7,0.0));}
#endif
}else{F0.xyz=H0(z6).xyz;r2(z6);}}F0.xyz*=F0.w;}
#endif
#endif
F0*=K8(V1,G1,F0.w);
#ifdef JB
if(JB){F0.xyz+=G5;}
#endif
#ifndef DB
#ifdef GB
#define fe (!GB||Z1==V5(M5))&&F0.w>=1.
#else
#define fe F0.w>=1.
#endif
td(fe,S0,packHalf2x16(A2(ee,z0)));
#else
Y1(S0);
#endif
#ifndef K
sd(F0.w==.0,g0,L1*(1.-F0.w)+F0);
#endif
}Y1(d0);w2;
#ifdef K
l1=F0;l3
#else
U1;
#endif
}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive