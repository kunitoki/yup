#pragma once

#include "draw_clockwise_path.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_clockwise_path_frag[] = R"===(#ifdef EB
I1
#ifndef Q
z0(B2,q0);
#endif
x1(o3,o0);
#ifndef Q
sc(F6,j7);
#endif
x1(o7,d1);J1
#ifdef Q
K2(HB)
#else
E1(HB)
#endif
{q(N0,e);
#ifdef GB
q(O0,M);
#endif
#ifdef DB
q(v1,d);
#else
q(S,U2);
#endif
q(J0,d);
#ifdef N
q(m2,F);
#endif
#ifdef AB
q(Y0,e);
#endif
#ifdef H
q(S0,d);
#endif
d F0=
#ifdef DB
v1;
#else
Xc(S);
#endif
i r0;d X1;
#if defined(DB)&&defined(FC)
if(!FC)
#endif
{r0=G8(
#ifdef GB
O0,
#endif
#ifdef H
a3(S0),
#endif
N0 q3);X1=1.;
#ifdef AB
if(AB){d cd=E3(y4(Y0));X1=min(cd,X1);}
#endif
}R2;
#if defined(DB)&&defined(FC)
if(FC){q1(d1,packHalf2x16(V2(F0,J0)));
#ifndef Q
Q2(q0);
#endif
}else
#endif
{F B5=unpackHalf2x16(o1(d1));d qa=B5.y;d C5=qa==J0?B5.x:B0(.0);d bg=
#ifndef DB
w6(S)?max(C5,F0):
#endif
C5+F0;
#ifdef N
if(N&&m2.x!=.0){F Z0=unpackHalf2x16(o1(o0));d m6=Z0.y;d dd=m6==m2.x?Z0.x:B0(.0);X1=min(dd,X1);}
#endif
X1=max(X1,.0);d y2=pb(C5,.0,X1);d U1=pb(bg,.0,X1);
#ifdef PB
d l6;if(PB){l6=sb(Y.xy,j.J3,j.K3);}
#endif
#ifndef Q
i X0=L0(q0);
#ifdef H
if(H&&S0!=z6(W3)){if(U1!=.0){if(y2==.0){r0.xyz=Y4(r0.xyz,X0,a3(S0));
#ifndef DB
if(U1<X1){v L8=r0.xyz;
#ifdef PB
if(PB){L8+=l6*j.cf;}
#endif
y0(j7,A0(L8,0.0));}
#endif
}else{r0.xyz=L0(j7).xyz;Q2(j7);}}r0.xyz*=r0.w;}
#endif
#endif
r0*=R9(y2,U1,r0.w);
#ifdef PB
r0.xyz=M2(r0.xyz,r0.w,l6);
#endif
#ifndef DB
#ifdef H
#define cg (!H||S0==z6(W3))&&r0.w>=1.
#else
#define cg r0.w>=1.
#endif
of(cg,d1,packHalf2x16(V2(bg,J0)));
#else
l2(d1);
#endif
#ifndef Q
nf(r0.x+r0.y+r0.z+r0.w==.0,q0,X0*(1.-r0.w)+r0);
#endif
}l2(o0);S2;
#ifdef Q
S1=r0;I3
#else
L1;
#endif
}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive