#pragma once

#include "draw_mesh.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_mesh_frag[] = R"===(#ifdef FB
#if(defined(W)&&!defined(A))||defined(QB)
#undef cc
#else
#define cc
#endif
R1
#ifndef W
B0(K2,n0);
#endif
#ifndef QB
o1(c3,m0);
#ifndef W
B0(o6,B4);
#endif
o1(U6,V0);
#else
B0(c3,m0);
#endif
S1
#ifdef NB
N3 i3(w5,l4,CC);O3 x5 m4(r5) y5 f4 g4
#endif
#ifdef W
#ifdef NB
z2(IB)
#else
z2(IB)
#endif
#else
#ifdef NB
T1(IB)
#else
T1(IB)
#endif
#endif
{
#ifdef EB
q(a1,e);
#if defined(GB)
q(r1,O);
#endif
q(J2,c);
#endif
#ifdef A
q(Y3,d);
#endif
#ifdef AB
q(R0,e);
#endif
#if defined(EB)&&defined(N)
q(Q0,d);
#endif
#ifdef NB
q(V5,c);q(Q1,i);
#ifdef N
q(H1,Q);
#endif
#endif
#ifdef EB
i p=Y7(
#ifdef GB
r1,
#endif
#ifdef N
k3(Q0),
#endif
a1 e3);d n=clamp(o2(FD,ma,J2,.0).x,H0(.0),H0(1.));
#endif
#ifdef NB
i p=K7(CC,r5,V5,j.Vd);d n=1.;
#endif
#ifdef AB
if(AB){d m5=max(v3(v5(R0)),H0(.0));n=min(m5,n);}
#endif
#ifdef cc
E2;
#endif
#if defined(A)
if(A&&Y3!=.0){d F3;
#ifndef QB
C T0=unpackHalf2x16(h1(m0));d P6=T0.y;F3=max(P6==Y3?T0.x:H0(.0),H0(.0));
#else
F3=N0(m0).x;
#endif
F3=max(F3,H0(.0));n=min(n,F3);}
#endif
#ifdef NB
p*=Q1;
#endif
#if!defined(W)
i I1=N0(n0);
#ifdef N
#ifdef EB
Q y3=k3(Q0);
#endif
#ifdef NB
Q y3=H1;
#endif
if(N&&y3!=L4){
#ifdef NB
p.xyz=Q6(p);
#endif
p.xyz=h5(p.xyz,I1,y3)*p.w;}
#endif
p*=n;p.xyz=M2(p.xyz,p.w,f0.xy,j.L3,j.M3);
#ifndef QB
p=I1*(1.-p.w)+p;
#endif
y0(n0,p);
#endif
#ifndef QB
Z1(m0);Z1(V0);
#else
y0(m0,I0(.0));
#endif
#ifdef cc
F2;
#endif
#ifdef W
p=(p*n);p.xyz=M2(p.xyz,p.w,f0.xy,j.L3,j.M3);K1=p;z3
#else
h2;
#endif
}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive