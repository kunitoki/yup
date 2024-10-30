#pragma once

#include "draw_image_mesh.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_image_mesh[] = R"===(#ifdef AB
Y0(T1)a0(0,f,XB);Z0 Y0(h2)a0(1,f,YB);Z0
#endif
H1 w0 W(0,f,N0);
#ifdef B
FB W(1,h,P0);
#endif
#ifdef U
w0 W(2,g,x0);
#endif
I1
#ifdef AB
R1 S1 j4(PB,T1,U1,h2,i2,n){f0(n,U1,XB,f);f0(n,i2,YB,f);c0(N0,f);
#ifdef B
c0(P0,h);
#endif
#ifdef U
c0(x0,g);
#endif
f g0=m0(j1(X.O5),XB)+X.a1;N0=YB;
#ifdef B
if(B){P0=f5(X.L1,P.X3);}
#endif
#ifdef U
if(U){
#ifndef CB
x0=U4(j1(X.k1),X.q1,g0);
#else
R7(j1(X.k1),X.q1,g0);
#endif
}
#endif
g T=E2(g0);
#ifdef CB
T.z=o6(X.z4);
#endif
d0(N0);
#ifdef B
d0(P0);
#endif
#ifdef U
d0(x0);
#endif
i1(T);}
#endif
#ifdef EB
H2 v1(S5,J3,QB);
#ifdef CB
#ifdef BB
v1(p2,f8,HC);
#endif
#endif
I2 c3(J3,L3)M3 P3
#ifndef CB
J1 A0(U5,q0);
#if defined(B)||defined(_EXPORTED_PLS_IMPL_ANGLE)
C0(k4,O0);
#endif
A0(h8,P2);C0(W5,C4);K1 U3(IB){Y(N0,f);
#ifdef B
Y(P0,h);
#endif
#ifdef U
Y(x0,g);
#endif
i j=l3(QB,L3,N0);h N=1.;
#ifdef U
if(U){h f3=f6(r4(x0));N=clamp(f3,d1(.0),N);}
#endif
w1;
#ifdef B
if(B&&P0!=.0){r c1=unpackHalf2x16(I0(O0));h c4=c1.y;h ka=c4==P0?c1.x:d1(.0);N=min(N,ka);}
#endif
j.w*=X.m3*N;i W0=r0(q0);
#ifdef BB
if(BB&&X.M2!=g6){j=S4(j,R3(W0),W1(X.M2));}else
#endif
{j.xyz*=j.w;j=j+W0*(1.-j.w);}y0(q0,j);
#ifdef B
X1(O0);
#endif
x1;k2;}
#else
q2(i,IB){Y(N0,f);i j=l3(QB,L3,N0);j.w*=X.m3;
#ifdef BB
if(BB){i W0=N1(HC,e0(floor(h0.xy)));j=S4(j,R3(W0),X.M2);}else
#endif
{j=M7(j);}r2(j);}
#endif
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive