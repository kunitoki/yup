#pragma once

#include "draw_path.vert.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_path_vert[] = R"===(#undef U2
#ifdef IB
#define U2 e
#else
#define U2 F
#endif
#ifdef CB
h1(d0)
#if defined(DB)||defined(FB)
K(0,l4,MB);
#else
K(0,e,YB);K(1,e,ZB);
#endif
i1
#endif
o2 I0 W(0,e,N0);
#ifdef FB
I0 W(1,c,X2);
#elif!defined(BB)
#ifdef DB
NB W(1,d,v1);
#else
I0 W(2,U2,S);
#endif
NB W(3,d,J0);
#endif
#ifdef N
#ifdef FB
NB W(4,d,j4);
#else
NB W(4,F,m2);
#endif
#endif
#if defined(AB)&&!defined(BB)
I0 W(5,e,Y0);
#endif
#ifdef H
NB W(6,d,S0);
#endif
#ifdef TB
l3 W(7,R0,C3);W(8,c,W4);
#endif
#ifdef GB
I0 W(9,M,O0);
#endif
P1
#ifdef CB
n1(UB,d0,A,C,r){
#if defined(DB)||defined(FB)
L(C,A,MB,M);
#else
L(C,A,YB,e);L(C,A,ZB,e);
#endif
V(N0,e);
#if defined(GB)
V(O0,M);
#endif
#ifdef FB
V(X2,c);
#elif!defined(BB)
#ifdef DB
V(v1,d);
#else
V(S,U2);
#endif
V(J0,d);
#endif
#ifdef N
#ifdef FB
V(j4,d);
#else
V(m2,F);
#endif
#endif
#if defined(AB)&&!defined(BB)
V(Y0,e);
#endif
#ifdef H
V(S0,d);
#endif
#ifdef TB
V(C3,R0);V(W4,c);
#endif
bool Sf=false;uint g0;c i0;
#ifdef BB
O L6;
#endif
#ifdef FB
i0=nd(MB,g0,
#ifdef BB
L6,
#endif
X2 S3);
#elif defined(DB)
i0=od(MB,g0
#ifdef BB
,L6
#else
,v1
#endif
S3);
#else
e T;Sf=!xa(YB,ZB,r,g0,i0
#ifndef BB
,T
#else
,L6
#endif
S3);
#ifndef BB
#ifdef IB
S=T;
#else
S.xy=N8(T.xy);
#endif
#endif
#endif
R0 y1=p0(JB,g0*4u+1u).zw;
#if!defined(FB)&&!defined(BB)
J0=r9(g0,j.B6);if((y1.x&Sa)!=0u) J0=-J0;
#endif
uint A2=y1.x&0xfu;
#ifdef N
if(N){uint nk=(A2==V8?y1.y:y1.x)>>16;d D1=r9(nk,j.B6);if(A2==V8) D1=-D1;
#ifdef FB
j4=D1;
#else
m2.x=D1;
#endif
}
#endif
#ifdef H
if(H){S0=float((y1.x>>4)&0xfu);}
#endif
c n0=i0;
#ifdef VD
if(j.Ja!=0u){n0.y=float(j.Ka)-n0.y;}
#endif
#ifdef AB
if(AB){X M3=w1(p0(KB,g0*q2+2u));e Z3=p0(KB,g0*q2+3u);
#ifndef BB
Y0=P8(M3,Z3.xy,n0);
#else
vb(M3,Z3.xy,n0 I4);
#endif
}
#endif
if(A2==Ta){N0=e(unpackUnorm4x8(y1.y));}
#if defined(N)&&!defined(FB)
else if(N&&A2==V8){d k6=r9(y1.x>>16,j.B6);m2.y=k6;}
#endif
else{X Sb=w1(p0(KB,g0*q2));e X7=p0(KB,g0*q2+1u);float w4=uintBitsToFloat(y1.y);N0=La(n0,Sb,X7.xy,X7.zw,A2,w4,1.0);}
#if defined(GB)
if(GB&&(y1.x&oe)!=0u){X Tb=w1(p0(KB,g0*q2+4u));e Y7=p0(KB,g0*q2+5u);c y3=G0(Tb,n0)+Y7.xy;float Z7=1.+Y7.z;if((y1.x&pe)!=0u){uint F3=(y1.x&re)>>qe;Z7=-(1.+float(F3));}O0=M(y3.x,y3.y,Z7);}else{O0=M(0.0,0.0,0.0);}
#endif
e I;if(!Sf){I=T3(i0);
#ifdef OC
I.y=-I.y;
#endif
#ifdef BB
I.z=v9(L6,0xffu);
#elif defined(TB)
R B5=p0(JB,g0*4u+3u);C3=B5.xy;W4=i0+uintBitsToFloat(B5.zw);
#endif
}else{I=e(j.m3,j.m3,j.m3,j.m3);}e0(N0);
#if defined(GB)
e0(O0);
#endif
#ifdef FB
e0(X2);
#elif!defined(BB)
#ifdef DB
e0(v1);
#else
e0(S);
#endif
e0(J0);
#endif
#ifdef N
#ifdef FB
e0(j4);
#else
e0(m2);
#endif
#endif
#if defined(AB)&&!defined(BB)
e0(Y0);
#endif
#ifdef H
e0(S0);
#endif
#ifdef TB
e0(C3);e0(W4);
#endif
C1(I);}
#endif
#ifdef EB
n4 p4 f i G8(
#ifdef GB
M H8,
#endif
#ifdef H
O c2,
#endif
e oa q7){
#ifdef H
bool J2=H&&c2!=W3;
#else
const bool J2=false;
#endif
i k;if(oa.w>=.0){k=y4(oa);}else{c bb=cb(oa,j.X8,j.Y8);k=r2(AD,Z8,bb,.0);if(!J2){d w4=Ee(oa);k.xyz*=k.w;k.w*=w4;}}
#if defined(GB)
if(GB&&H8.z<0.0){return k5(SB,x4,H8.xy,B0(.0));}if(GB&&H8.z>0.0){d Ub=H8.z-1.;i T1=k5(SB,x4,H8.xy,Ub);if(J2) T1=A0(o6(T1),T1.w);k*=T1;}
#endif
return k;}
#if!defined(DB)&&!defined(FB)
f d Tf(U2 T g4){
#ifdef IB
if(IB&&wd(T)) return f5(T r1);else
#endif
return min(T.x,T.y);}f d Uf(U2 T g4){
#if defined(IB)
if(IB&&xd(T)) return d9(T r1);else
#endif
return T.x;}f d Xc(U2 T g4){if(w6(T)) return Tf(T r1);else return Uf(T r1);}f d ok(d C5,U2 T g4){if(w6(T)){d F0=Tf(T r1);return max(F0,C5);}else{d F0=Uf(T r1);return C5+F0;}}
#endif
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive