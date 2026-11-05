#pragma once

#include "draw_mesh.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_mesh_frag[] = R"===(#ifdef EB
#if(defined(Q)&&!defined(N))||defined(TB)
#undef fd
#else
#define fd
#endif
I1
#ifndef Q
z0(B2,q0);
#endif
#ifndef TB
x1(o3,o0);
#ifndef Q
z0(F6,T4);
#endif
x1(o7,d1);
#else
z0(o3,o0);
#endif
J1
#ifdef OB
X3 Z2(J5,C4,SB);Y3 K5 D4(x4) L5 n4 p4
#endif
#ifdef Q
#ifdef OB
K2(HB)
#else
K2(HB)
#endif
#else
#ifdef OB
E1(HB)
#else
E1(HB)
#endif
#endif
{
#ifdef FB
q(N0,e);
#if defined(GB)
q(O0,M);
#endif
q(X2,c);
#endif
#ifdef N
q(j4,d);
#endif
#ifdef AB
q(Y0,e);
#endif
#if defined(FB)&&defined(H)
q(S0,d);
#endif
#ifdef OB
q(j6,c);q(a2,i);
#ifdef H
q(Q1,O);
#endif
#endif
#ifdef FB
i k=G8(
#ifdef GB
O0,
#endif
#ifdef H
a3(S0),
#endif
N0 q3);d n=clamp(r2(ID,Za,X2,.0).x,B0(.0),B0(1.));
#endif
#ifdef OB
i k=m8(SB,x4,j6,j.bf);d n=1.;
#endif
#ifdef AB
if(AB){d G5=max(E3(y4(Y0)),B0(.0));n=min(G5,n);}
#endif
#ifdef fd
R2;
#endif
#if defined(N)
if(N&&j4!=.0){d R3;
#ifndef TB
F Z0=unpackHalf2x16(o1(o0));d k7=Z0.y;R3=max(k7==j4?Z0.x:B0(.0),B0(.0));
#else
R3=L0(o0).x;
#endif
R3=max(R3,B0(.0));n=min(n,R3);}
#endif
#ifdef OB
k*=a2;
#endif
#if!defined(Q)
i X0=L0(q0);
#ifdef H
#ifdef FB
O c2=a3(S0);
#endif
#ifdef OB
O c2=Q1;
#endif
if(H&&c2!=W3){
#ifdef OB
k.xyz=o6(k);
#endif
k.xyz=Y4(k.xyz,X0,c2)*k.w;}
#endif
k*=n;k.xyz=M2(k.xyz,k.w,Y.xy,j.J3,j.K3);
#ifndef TB
k=X0*(1.-k.w)+k;
#endif
y0(q0,k);
#endif
#ifndef TB
l2(o0);l2(d1);
#else
y0(o0,A0(.0));
#endif
#ifdef fd
S2;
#endif
#ifdef Q
k=(k*n);k.xyz=M2(k.xyz,k.w,Y.xy,j.J3,j.K3);S1=k;I3
#else
L1;
#endif
}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive