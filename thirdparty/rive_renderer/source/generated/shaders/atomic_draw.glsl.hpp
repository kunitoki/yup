#pragma once

#include "atomic_draw.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char atomic_draw[] = R"===(#ifdef LC
#ifdef AB
Y0(V)a0(0,g,VB);a0(1,g,WB);Z0
#endif
H1 w0 W(0,r,V0);H3 W(1,M,j0);I1
#ifdef AB
h1(PB,V,q,n,H){f0(n,q,VB,g);f0(n,q,WB,g);c0(V0,r);c0(j0,M);g T;f g0;if(k7(VB,WB,H,j0,g0,V0 Y2)){T=E2(g0);}else{T=g(P.F2,P.F2,P.F2,P.F2);}d0(V0);d0(j0);i1(T);}
#endif
#endif
#ifdef DB
#ifdef AB
Y0(V)a0(0,I3,KB);Z0
#endif
H1 FB W(0,h,Q1);H3 W(1,M,j0);I1
#ifdef AB
h1(PB,V,q,n,H){f0(n,q,KB,Q);c0(Q1,h);c0(j0,M);f g0=l7(KB,j0,Q1 Y2);g T=E2(g0);d0(Q1);d0(j0);i1(T);}
#endif
#endif
#ifdef MC
#ifdef AB
Y0(V)a0(0,g,MB);Z0
#endif
H1 w0 W(0,f,N0);w0 W(1,h,Z2);
#ifdef U
w0 W(2,g,x0);
#endif
I1
#ifdef AB
R1 S1 f2 g2 T4(PB,V,q,n,H){f0(n,q,MB,g);c0(N0,f);c0(Z2,h);
#ifdef U
c0(x0,g);
#endif
bool N5=MB.z==.0||MB.w==.0;Z2=N5?.0:1.;f g0=MB.xy;I p0=j1(X.O5);I i4=transpose(inverse(p0));if(!N5){float P5=G2*Q5(i4[1])/dot(p0[1],i4[1]);if(P5>=.5){g0.x=.5;Z2*=o2(.5/P5);}else{g0.x+=P5*MB.z;}float R5=G2*Q5(i4[0])/dot(p0[0],i4[0]);if(R5>=.5){g0.y=.5;Z2*=o2(.5/R5);}else{g0.y+=R5*MB.w;}}N0=g0;g0=m0(p0,g0)+X.a1;if(N5){f a3=m0(i4,MB.zw);a3*=Q5(a3)/dot(a3,a3);g0+=G2*a3;}
#ifdef U
if(U){x0=U4(j1(X.k1),X.q1,g0);}
#endif
g T=E2(g0);d0(N0);d0(Z2);
#ifdef U
d0(x0);
#endif
i1(T);}
#endif
#elif defined(LD)
#ifdef AB
Y0(T1)a0(0,f,XB);Z0 Y0(h2)a0(1,f,YB);Z0
#endif
H1 w0 W(0,f,N0);
#ifdef U
w0 W(1,g,x0);
#endif
I1
#ifdef AB
j4(PB,T1,U1,h2,i2,n){f0(n,U1,XB,f);f0(n,i2,YB,f);c0(N0,f);
#ifdef U
c0(x0,g);
#endif
I p0=j1(X.O5);f g0=m0(p0,XB)+X.a1;N0=YB;
#ifdef U
if(U){x0=U4(j1(X.k1),X.q1,g0);}
#endif
g T=E2(g0);d0(N0);
#ifdef U
d0(x0);
#endif
i1(T);}
#endif
#endif
#ifdef MD
#ifdef AB
Y0(V)Z0
#endif
H1 I1
#ifdef AB
R1 S1 f2 g2 h1(PB,V,q,n,H){e0 r1;r1.x=(n&1)==0?P.V4.x:P.V4.z;r1.y=(n&2)==0?P.V4.y:P.V4.w;g T=E2(f(r1));i1(T);}
#endif
#endif
#ifdef NC
#define m7
#endif
#ifdef EB
H2 v1(p2,W4,FC);
#ifdef m7
v1(S5,J3,QB);
#endif
I2 K3(W4,T5)
#ifdef m7
c3(J3,L3)
#endif
J1
#ifndef GB
#ifdef AD
A0(AD,q0);
#else
A0(U5,q0);
#endif
#endif
#ifdef RB
#define J2 i
#define V5 r0
#define X4 C2(.0)
#define n7(n0) ((n0).w!=.0)
#ifdef B
#ifndef CC
A0(k4,O0);
#else
o7(k4,O0);
#endif
#endif
#else
#define J2 uint
#define X4 0u
#define V5 I0
#define n7(n0) ((n0)!=0u)
#ifdef B
C0(k4,O0);
#endif
#endif
d3(W5,K2);K1 M3 N3(p7,E9,GC);O3(q7,F9,HB);P3 d uint G9(float x){return uint(x*X5+Y5);}d h Y4(uint x){return o2(float(x)*r7+(-Y5*r7));}
#ifdef B
d void v7(uint L1,J2 c1,l4(h)N){
#ifdef RB
if(all(lessThan(abs(c1.xy-unpackUnorm4x8(L1).xy),D2(.25/255.))))N=min(N,c1.z);else N=.0;
#else
if(L1==c1>>16)N=min(N,unpackHalf2x16(c1).x);else N=.0;
#endif
}
#endif
d void Z4(uint l1,h m1,j2(i)J
#if defined(B)&&!defined(CC)
,l4(J2)D0
#endif
m4 L2){n1 J0=Q3(GC,l1);
#ifdef BD
h N=clamp(m1,d1(.0),d1(1.));
#else
h N=abs(m1);
#ifdef DC
if(DC&&(J0.x&w7)!=0u){N=1.-abs(fract(N*.5)*2.+-1.);}
#endif
N=min(N,d1(1.));
#endif
#ifdef B
if(B){uint L1=J0.x>>16u;if(L1!=0u){v7(L1,V5(O0),N);}}
#endif
#ifdef U
if(U&&(J0.x&H9)!=0u){I p0=j1(E0(HB,l1*4u+2u));g a1=E0(HB,l1*4u+3u);f I9=m0(p0,h0)+a1.xy;r x7=Z5(abs(I9)*a1.zw-a1.zw);h f3=clamp(min(x7.x,x7.y)+.5,.0,1.);N=min(N,f3);}
#endif
uint V1=J0.x&0xfu;if(V1<=y7){J=unpackUnorm4x8(J0.y);
#ifdef B
if(B&&V1==a5){
#ifndef CC
#ifdef RB
D0.xy=J.zw;D0.z=N;D0.w=1.;
#else
D0=J0.y|packHalf2x16(D2(N,.0));
#endif
#endif
J=C2(.0);}
#endif
}else{I p0=j1(E0(HB,l1*4u));g a1=E0(HB,l1*4u+1u);f g3=m0(p0,h0)+a1.xy;float t=V1==a6?g3.x:length(g3);t=clamp(t,.0,1.);float x=t*a1.z+a1.w;float y=uintBitsToFloat(J0.y);J=h3(FC,T5,f(x,y),.0);}J.w*=N;
#if!defined(GB)&&defined(BB)
M M2;if(BB&&J.w!=.0&&(M2=W1((J0.x>>4)&0xfu))!=0u){i W0=r0(q0);J.xyz=j7(J.xyz,R3(W0),M2);}
#endif
#ifndef RB
J.xyz*=J.w;
#endif
}
#ifndef GB
d void c5(i J L2){
#ifndef RB
if(J.w==.0)return;float A7=1.-J.w;if(A7!=.0)J=r0(q0)*A7+J;
#endif
y0(q0,J);}
#endif
#if defined(B)&&!defined(CC)
d void c6(J2 D0 L2){
#ifdef RB
y0(O0,D0);
#else
if(D0!=0u)K0(O0,D0);
#endif
}
#endif
#ifdef GB
#define n4 N2
#define B7 o4
#define T3 S3
#else
#define n4 M1
#define B7 U3
#define T3 k2
#endif
#ifdef LC
n4(IB){Y(V0,r);Y(j0,M);h J9=min(min(V0.x,abs(V0.y)),d1(1.));uint d5=G9(J9);uint C7=(D7(j0)<<16)|d5;uint o1=p4(K2,C7);M l2=W1(o1>>16);if(l2==j0){if(V0.y<.0){d5+=o1-max(C7,o1);d5-=d6;q4(K2,d5);}discard;}h m1=Y4(o1&0xffffu);i J;
#ifdef B
J2 D0=X4;
#endif
Z4(l2,m1,J
#ifdef B
,D0
#endif
O2 X0);
#ifdef GB
e1=J;
#else
c5(J X0);
#endif
#ifdef B
c6(D0 X0);
#endif
T3}
#endif
#ifdef DB
n4(IB){Y(Q1,h);Y(j0,M);uint o1=i3(K2);M l2=W1(o1>>16);int K9=int(Q1*X5);uint L9=l2==j0?o1:(D7(j0)<<16)+d6;j3(K2,L9+uint(K9));if(l2==j0){discard;}h e6=Y4(o1&0xffffu);i J;
#ifdef B
J2 D0=X4;
#endif
Z4(l2,e6,J
#ifdef B
,D0
#endif
O2 X0);
#ifdef GB
e1=J;
#else
c5(J X0);
#endif
#ifdef B
c6(D0 X0);
#endif
T3}
#endif
#ifdef NC
B7(IB){Y(N0,f);
#ifdef MC
Y(Z2,h);
#endif
#ifdef U
Y(x0,g);
#endif
i k3=l3(QB,L3,N0);h V3=1.;
#ifdef MC
V3=min(Z2,V3);
#endif
#ifdef U
if(U){h f3=f6(r4(x0));V3=clamp(f3,d1(.0),V3);}
#endif
uint o1=i3(K2);M l2=W1(o1>>16);h e6=Y4(o1&0xffffu);i J;
#ifdef B
J2 D0=X4;
#endif
Z4(l2,e6,J
#ifdef B
,D0
#endif
O2 X0);
#ifdef RB
J.xyz*=J.w;
#endif
#ifdef B
if(B&&X.L1!=0u){J2 c1=n7(D0)?D0:V5(O0);v7(X.L1,c1,V3);}
#endif
k3.w*=V3*o2(X.m3);
#if!defined(GB)&&defined(BB)
if(BB&&X.M2!=g6){i W0=r0(q0)*(1.-J.w)+J;k3.xyz=j7(k3.xyz,R3(W0),W1(X.M2));}
#endif
k3.xyz*=k3.w;J=J*(1.-k3.w)+k3;
#ifdef GB
e1=J;
#else
c5(J X0);
#endif
#ifdef B
c6(D0 X0);
#endif
j3(K2,d6);T3}
#endif
#ifdef ND
n4(IB){
#ifdef OD
y0(q0,unpackUnorm4x8(P.M9));
#endif
#ifdef PD
i j=r0(q0);y0(q0,j.zyxw);
#endif
j3(K2,P.N9);
#ifdef B
if(B){K0(O0,0u);}
#endif
#ifdef GB
discard;
#endif
T3}
#endif
#ifdef CC
#ifdef CD
N2(IB)
#else
n4(IB)
#endif
{uint o1=i3(K2);h m1=Y4(o1&0xffffu);M l2=W1(o1>>16);i J;Z4(l2,m1,J O2 X0);
#ifdef CD
e1=r0(q0)*(1.-J.w)+J;S3
#else
#ifdef GB
e1=J;
#else
c5(J X0);
#endif
T3
#endif
}
#endif
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive