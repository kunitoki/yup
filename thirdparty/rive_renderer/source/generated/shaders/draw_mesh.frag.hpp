#pragma once

#include "draw_mesh.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_mesh_frag[] = R"===(#ifdef FB
#if(defined(K)&&!defined(O))||defined(QB)
#undef kb
#else
#define kb
#endif
J1
#ifndef K
r0(Q2,g0);
#endif
#ifndef QB
k1(R2,d0);
#ifndef K
r0(d6,g4);
#endif
k1(F6,S0);
#else
r0(R2,d0);
#endif
K1
#ifdef LB
B3 X2(Z4,T3,AC);C3 a5 U3(R5)c5 N3 O3
#endif
#ifdef K
#ifdef LB
r4(IB)
#else
o2(IB)
#endif
#else
#ifdef LB
O5(IB)
#else
m1(IB)
#endif
#endif
{
#ifdef EB
B(i1,g);B(C2,c);
#endif
#ifdef LB
B(U0,c);
#endif
#ifdef O
B(I3,d);
#endif
#ifdef AB
B(N0,g);
#endif
#if defined(EB)&&defined(GB)
B(Z1,d);
#endif
#ifdef EB
i j=M7(i1,1. S2);d n=clamp(m2(UC,I9,C2,.0).x,G0(.0),G0(1.));
#endif
#ifdef LB
i j=y7(AC,R5,U0,k.fd);d n=1.;
#endif
#ifdef AB
if(AB){d U4=max(g3(Y4(N0)),G0(.0));n=min(U4,n);}
#endif
#ifdef kb
v2;
#endif
#if defined(O)
if(O&&I3!=.0){d r3;
#ifndef QB
D O0=unpackHalf2x16(d1(d0));d A6=O0.y;r3=max(A6==I3?O0.x:G0(.0),G0(.0));
#else
r3=H0(d0).x;
#endif
r3=max(r3,G0(.0));n=min(n,r3);}
#endif
#ifdef LB
n*=A0.x4;
#endif
#if!defined(K)
i L1=H0(g0);
#ifdef GB
if(GB){
#ifdef EB
X n2=W5(Z1);
#endif
#ifdef LB
j.xyz=B6(j);X n2=i2(A0.n2);
#endif
if(n2!=M5){j.xyz=Q4(j.xyz,L1,n2);}j.w*=n;j.xyz*=j.w;}else
#endif
{j*=n;}
#ifdef UB
if(UB){j=k3(j);}
#endif
j.xyz=Q3(j.xyz,S.xy,k.y3,k.z3);
#ifndef QB
j=L1*(1.-j.w)+j;
#endif
v0(g0,j);
#endif
#ifndef QB
Y1(d0);Y1(S0);
#else
v0(d0,B0(.0));
#endif
#ifdef kb
w2;
#endif
#ifdef K
j=(j*n);j.xyz=Q3(j.xyz,S.xy,k.y3,k.z3);l1=j;l3
#else
U1;
#endif
}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive