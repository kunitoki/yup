#pragma once

#include "atomic_draw.exports.h"

namespace rive {
namespace pls {
namespace glsl {
const char atomic_draw[] = R"===(#ifdef IC
#ifdef V
U0(P)q0(0,g,UB);q0(1,g,VB);V0
#endif
A1 k0 I(0,l0,J0);FB I(1,M,W);B1
#ifdef V
g1(OB,P,r,j,L){v0(j,r,UB,g);v0(j,r,VB,g);Q(J0,l0);Q(W,M);g B;d R;if(E6(UB,VB,L,W,R,J0 j3)){B=i2(R);}else{B=g(v.j2,v.j2,v.j2,v.j2);}S(J0);S(W);h1(B);}
#endif
#endif
#ifdef DB
#ifdef V
U0(P)q0(0,H3,KB);V0
#endif
A1 FB I(0,h,C1);FB I(1,M,W);B1
#ifdef V
g1(OB,P,r,j,L){v0(j,r,KB,D1);Q(C1,h);Q(W,M);d R=F6(KB,W,C1 j3);g B=i2(R);S(C1);S(W);h1(B);}
#endif
#endif
#ifdef BC
#ifdef JC
#ifdef V
U0(P)q0(0,g,LB);V0
#endif
A1 k0 I(0,d,B0);k0 I(1,h,D2);
#ifdef Z
k0 I(2,g,i0);
#endif
B1
#ifdef V
O1 P1 V1 W1 k5(OB,P,r,j,L){v0(j,r,LB,g);Q(B0,d);Q(D2,h);
#ifdef Z
Q(i0,g);
#endif
bool l5=LB.z==.0||LB.w==.0;D2=l5?.0:1.;d R=LB.xy;A a0=W0(J.m5);A I3=transpose(inverse(a0));if(!l5){float n5=k2*o5(I3[1])/dot(a0[1],I3[1]);if(n5>=.5){R.x=.5;D2*=.5/n5;}else{R.x+=n5*LB.z;}float p5=k2*o5(I3[0])/dot(a0[0],I3[0]);if(p5>=.5){R.y=.5;D2*=.5/p5;}else{R.y+=p5*LB.w;}}B0=R;R=h0(a0,R)+J.K0;if(l5){d E2=h0(I3,LB.zw);E2*=o5(E2)/dot(E2,E2);R+=k2*E2;}
#ifdef Z
i0=n4(W0(J.X0),J.i1,R);
#endif
g B=i2(R);S(B0);S(D2);
#ifdef Z
S(i0);
#endif
h1(B);}
#endif
#else
#ifdef V
U0(X1)q0(0,d,WB);V0 U0(l2)q0(1,d,XB);V0
#endif
A1 k0 I(0,d,B0);
#ifdef Z
k0 I(1,g,i0);
#endif
B1
#ifdef V
o4(OB,X1,Y1,l2,m2,j){v0(j,Y1,WB,d);v0(j,m2,XB,d);Q(B0,d);
#ifdef Z
Q(i0,g);
#endif
A a0=W0(J.m5);d R=h0(a0,WB)+J.K0;B0=XB;
#ifdef Z
i0=n4(W0(J.X0),J.i1,R);
#endif
g B=i2(R);S(B0);
#ifdef Z
S(i0);
#endif
h1(B);}
#endif
#endif
#endif
#ifdef YC
#ifdef V
U0(P)V0
#endif
A1 B1
#ifdef V
O1 P1 V1 W1 g1(OB,P,r,j,L){m0 j1;j1.x=(j&1)==0?v.p4.x:v.p4.z;j1.y=(j&2)==0?v.p4.y:v.p4.w;g B=i2(d(j1));h1(B);}
#endif
#endif
#ifdef CC
#define q5
#endif
#ifdef BC
#define q5
#endif
#ifdef GB
F2 x1(q4,DC);
#ifdef q5
x1(k3,MB);
#endif
G2 J3(q4,r5)
#ifdef q5
l3(k3,n2)
#endif
E1
#ifdef AB
#ifdef NC
C0(NC,c0);
#else
C0(v5,c0);
#endif
#endif
K3(w5,Q0);
#ifdef BB
D0(x5,w0);
#endif
F1 L3 M3(G6,X8,PB);N3(H6,Y8,HB);O3 uint I6(float x){return uint(x*J6+P3);}float r4(uint x){return float(x)*K6+(-P3*K6);}i v4(h Y0,x0 E,uint k1 w4 H2){h C=abs(Y0);
#ifdef OC
if((E.x&L6)!=0u)C=1.-abs(fract(C*.5)*2.+-1.);
#endif
C=min(C,E0(1));
#ifdef BB
uint G1=E.x>>16u;if(G1!=0u){uint Z0=L0(w0);h I2=G1==(Z0>>16u)?unpackHalf2x16(Z0).x:.0;C=min(C,I2);}
#endif
i f=M0(0,0,0,0);uint H1=E.x&0xfu;switch(H1){case M6:f=unpackUnorm4x8(E.y);
#ifdef BB
y0(w0);
#endif
break;case x4:case N6:
#ifdef CC
case O6:
#endif
{A a0=W0(z0(HB,k1*4u));g K0=z0(HB,k1*4u+1u);d o2=h0(a0,n0)+K0.xy;
#ifdef CC
if(H1==O6){f=Q3(sampler2D(floatBitsToUint(K0.zw)),n2,o2,a0[0],a0[1]);float J2=uintBitsToFloat(E.y);f.w*=J2;}else
#endif
{float t=H1==x4?o2.x:length(o2);t=clamp(t,.0,1.);float x=t*K0.z+K0.w;float y=uintBitsToFloat(E.y);f=M0(R3(DC,r5,d(x,y),.0));}
#ifdef BB
y0(w0);
#endif
break;}
#ifdef BB
case y4:a1(w0,E.y|packHalf2x16(Z1(C,0)));break;
#endif
}
#ifdef Z
if((E.x&Z8)!=0u){A a0=W0(z0(HB,k1*4u+2u));g K0=z0(HB,k1*4u+3u);d a9=h0(a0,n0)+K0.xy;l0 P6=Z1(abs(a9)*K0.zw-K0.zw);h K2=clamp(min(P6.x,P6.y)+.5,.0,1.);C=min(C,K2);}
#endif
f.w*=C;return f;}i Q6(i R6,i Q1){return R6+Q1*(1.-R6.w);}
#ifdef AB
i y5(i S6,i Q1,M a2){if(a2!=T6){
#ifdef JB
return E3(
#else
return F3(
#endif
S6,S3(Q1),a2);}else{return Q6(p2(S6),Q1);}}i U6(i f,x0 E H2){i Q1=N0(c0);M a2=O0((E.x>>4)&0xfu);return y5(f,Q1,a2);}void z5(i f,x0 E H2){if(f.w!=.0){i c9=U6(f,E l1);F0(c0,c9);}else{y0(c0);}}
#endif
#ifdef AB
#define T3 R1
#define V6 U3
#define m3 c2
#else
#define T3 n3
#define V6 z4
#define m3 V3
#endif
#ifdef IC
T3(IB){N(J0,l0);N(W,M);
#ifndef AB
P0=M0(0,0,0,0);
#endif
h C=min(min(J0.x,abs(J0.y)),E0(1));uint A4=I6(C);uint A5=(W6(W)<<16)|A4;uint m1=B4(Q0,A5);M c1=O0(m1>>16);if(c1!=W){h Y0=r4(m1&0xffffu);x0 E=q2(PB,c1);i f=v4(Y0,E,c1 L2 l1);
#ifdef AB
z5(f,E l1);
#else
P0=p2(f);
#endif
}else{if(J0.y<.0){if(m1<A5){A4+=m1-A5;}A4-=uint(P3);C4(Q0,A4);}discard;}m3}
#endif
#ifdef DB
T3(IB){N(C1,h);N(W,M);
#ifndef AB
P0=M0(0,0,0,0);
#endif
h C=C1;uint m1=o3(Q0);M c1=O0(m1>>16);h X6=r4(m1&0xffffu);if(c1!=W){x0 E=q2(PB,c1);i f=v4(X6,E,c1 L2 l1);
#ifdef AB
z5(f,E l1);
#else
P0=p2(f);
#endif
}else{C+=X6;}p3(Q0,(W6(W)<<16)|I6(C));if(c1==W){discard;}m3}
#endif
#ifdef BC
V6(IB){N(B0,d);
#ifdef JC
N(D2,h);
#endif
#ifdef Z
N(i0,g);
#endif
i q3=M2(MB,n2,B0);h N2=1.;
#ifdef JC
N2=min(D2,N2);
#endif
#ifdef Z
h K2=B5(M0(i0));N2=clamp(K2,E0(0),N2);
#endif
#ifdef PC
n1;
#endif
uint m1=o3(Q0);h Y0=r4(m1&0xffffu);M c1=O0(m1>>16);x0 Y6=q2(PB,c1);i C5=v4(Y0,Y6,c1 L2 l1);
#ifdef BB
if(J.G1!=0u){D4(w0);uint Z0=L0(w0);uint G1=Z0>>16;h I2=G1==J.G1?unpackHalf2x16(Z0).x:.0;N2=min(N2,I2);}
#endif
q3.w*=N2*J.J2;
#ifdef AB
if(C5.w!=.0||q3.w!=.0){i Q1=N0(c0);M d9=O0((Y6.x>>4)&0xfu);M e9=O0(J.a2);Q1=y5(C5,Q1,d9);q3=y5(q3,Q1,e9);F0(c0,q3);}else{y0(c0);}
#else
P0=Q6(p2(q3),p2(C5));
#endif
p3(Q0,uint(P3));
#ifdef PC
o1;
#endif
m3}
#endif
#ifdef ZC
T3(IB){
#ifndef AB
P0=M0(0,0,0,0);
#endif
#ifdef AD
F0(c0,unpackUnorm4x8(v.f9));
#endif
#ifdef BD
i f=N0(c0);F0(c0,f.zyxw);
#endif
p3(Q0,v.g9);
#ifdef BB
a1(w0,0u);
#endif
m3}
#endif
#ifdef CD
#ifdef QC
n3(IB)
#else
T3(IB)
#endif
{uint m1=o3(Q0);h Y0=r4(m1&0xffffu);M c1=O0(m1>>16);x0 E=q2(PB,c1);i f=v4(Y0,E,c1 L2 l1);
#ifdef QC
P0=U6(f,E l1);V3
#else
#ifdef AB
z5(f,E l1);
#else
P0=p2(f);
#endif
m3
#endif
}
#endif
#endif
)===";
} // namespace glsl
} // namespace pls
} // namespace rive