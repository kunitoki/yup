#pragma once

#include "draw_image_mesh.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_image_mesh[] = R"===(#ifdef AB
U0(a2)i0(0,c,SB);V0 U0(v2)i0(1,c,TB);V0
#endif
o1 n0 H(0,c,q0);
#ifdef T
OB H(1,g,j5);
#endif
#ifdef BB
n0 H(2,f,R0);
#endif
p1
#ifdef AB
P2 Q2 N4(PB,a2,c2,v2,w2,n){l0(n,c2,SB,c);l0(n,w2,TB,c);L(q0,c);
#ifdef T
L(j5,g);
#endif
#ifdef BB
L(R0,f);
#endif
c J=C0(D1(m0.H6),SB)+m0.S0;q0=TB;
#ifdef T
if(T){j5=g7(m0.Z0,q.d5);}
#endif
#ifdef BB
if(BB){
#ifndef DB
R0=I6(D1(m0.R1),m0.Z1,J);
#else
Ka(D1(m0.R1),m0.Z1,J);
#endif
}
#endif
f Q=F2(J);
#ifdef DB
Q.z=S8(m0.X5);
#endif
P(q0);
#ifdef T
P(j5);
#endif
#ifdef BB
P(R0);
#endif
h1(Q);}
#endif
#ifdef HB
R2 C2(Y5,W8,UB);
#ifdef DB
#ifdef FB
C2(U2,Za,PC);
#endif
#endif
S2 p4 G3(X8,B3)q4 w3 x3
#ifndef DB
x2 M0(i8,H0);Y0(B5,r1);M0(bb,N3);Y0(k8,x4);y2 R4(NB){N(q0,c);
#ifdef T
N(j5,g);
#endif
#ifdef BB
N(R0,f);
#endif
i j=o4(UB,B3,q0);g E=1.;
#ifdef BB
if(BB){g l4=A8(I5(R0));E=clamp(l4,v1(.0),E);}
#endif
h2;
#ifdef T
if(T&&j5!=.0){G I1=unpackHalf2x16(j1(r1));g k5=I1.y;g Sd=k5==j5?I1.x:v1(.0);E=min(E,Sd);}
#endif
i w1=I0(H0);
#ifdef FB
if(FB&&m0.z3!=B8){j.xyz=M4(Y3(j),w1,Q1(m0.z3))*j.w;}
#endif
j*=m0.H2*E;j+=w1*(1.-j.w);T0(H0,j);i2(r1);i2(x4);j2;M2;}
#else
e2(i,NB){N(q0,c);i j=o4(UB,B3,q0)*m0.H2;
#ifdef FB
if(FB){i w1=d1(PC,c0(floor(y0.xy)));j.xyz=M4(Y3(j),w1,m0.z3);j.xyz*=j.w;}
#endif
f2(j);}
#endif
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive