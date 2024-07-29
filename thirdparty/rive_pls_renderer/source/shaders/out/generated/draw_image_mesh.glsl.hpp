#pragma once

#include "draw_image_mesh.exports.h"

namespace rive {
namespace pls {
namespace glsl {
const char draw_image_mesh[] = R"===(#ifdef Y
U0(a2)v0(0,d,XB);V0 U0(n2)v0(1,d,YB);V0
#endif
D1 n0 K(0,d,E0);
#ifdef CB
GB K(1,h,K0);
#endif
#ifdef AB
n0 K(2,g,m0);
#endif
E1
#ifdef Y
Q1 R1 y4(PB,a2,c2,n2,o2,k){z0(k,c2,XB,d);z0(k,o2,YB,d);S(E0,d);
#ifdef CB
S(K0,h);
#endif
#ifdef AB
S(m0,g);
#endif
d T=l0(W0(L.x5),XB)+L.O0;E0=YB;
#ifdef CB
K0=M4(L.I1,A.A3);
#endif
#ifdef AB
#ifndef DB
m0=x4(W0(L.X0),L.i1,T);
#else
q7(W0(L.X0),L.i1,T);
#endif
#endif
g C=k2(T);
#ifdef DB
C.z=S5(L.e4);
#endif
U(E0);
#ifdef CB
U(K0);
#endif
#ifdef AB
U(m0);
#endif
h1(C);}
#endif
#ifdef HB
K2 z1(C5,p3,OB);
#ifdef DB
#ifdef BB
z1(d2,G7,FC);
#endif
#endif
L2 q3(p3,p2)Q3 T3
#ifndef DB
G1 F0(E5,e0);G0(F5,Y0);G0(G5,A0);F0(I7,y2);H1 Z3(JB){P(E0,d);
#ifdef CB
P(K0,h);
#endif
#ifdef AB
P(m0,g);
#endif
j f=R2(OB,p2,E0);h D=1.;
#ifdef AB
h P2=K5(j0(m0));D=clamp(P2,V(0),D);
#endif
p1;
#ifdef CB
if(K0!=.0){F a1=unpackHalf2x16(H0(A0));h D3=a1.y;h N2=D3==K0?a1.x:V(0);D=min(D,N2);}
#endif
f.w*=L.O2*D;j B1=I0(e0);
#ifdef BB
if(L.e2!=0u){
#ifdef KB
f=J3(
#else
f=K3(
#endif
f,X3(B1),J0(L.e2));}else
#endif
{f.xyz*=f.w;f=f+B1*(1.-f.w);}w0(e0,f);A1(A0);q1;f2;}
#else
w2(j,JB){P(E0,d);j f=R2(OB,p2,E0);f.w*=L.O2;
#ifdef BB
j B1=K1(FC,o0(floor(p0.xy)));
#ifdef KB
f=J3(
#else
f=K3(
#endif
f,X3(B1),J0(L.e2));
#else
f=r2(f);
#endif
x2(f);}
#endif
#endif
)===";
} // namespace glsl
} // namespace pls
} // namespace rive