#pragma once

#include "atomic_draw.exports.h"

namespace rive {
namespace pls {
namespace glsl {
const char atomic_draw[] = R"===(#ifdef JC
#ifdef W
T0(P)q0(0,g,VB);q0(1,g,WB);U0
#endif
z1 k0 I(0,l0,J0);GB I(1,M,a0);A1
#ifdef W
e1(PB,P,r,j,L){w0(j,r,VB,g);w0(j,r,WB,g);Q(J0,l0);Q(a0,M);g B;d R;if(D6(VB,WB,L,a0,R,J0 i3)){B=h2(R);}else{B=g(v.i2,v.i2,v.i2,v.i2);}S(J0);S(a0);f1(B);}
#endif
#endif
#ifdef EB
#ifdef W
T0(P)q0(0,G3,LB);U0
#endif
z1 GB I(0,h,B1);GB I(1,M,a0);A1
#ifdef W
e1(PB,P,r,j,L){w0(j,r,LB,C1);Q(B1,h);Q(a0,M);d R=E6(LB,a0,B1 i3);g B=h2(R);S(B1);S(a0);f1(B);}
#endif
#endif
#ifdef CC
#ifdef KC
#ifdef W
T0(P)q0(0,g,MB);U0
#endif
z1 k0 I(0,d,B0);k0 I(1,h,C2);
#ifdef BB
k0 I(2,g,i0);
#endif
A1
#ifdef W
N1 O1 U1 V1 j5(PB,P,r,j,L){w0(j,r,MB,g);Q(B0,d);Q(C2,h);
#ifdef BB
Q(i0,g);
#endif
bool k5=MB.z==.0||MB.w==.0;C2=k5?.0:1.;d R=MB.xy;A c0=V0(J.l5);A H3=transpose(inverse(c0));if(!k5){float m5=j2*n5(H3[1])/dot(c0[1],H3[1]);if(m5>=.5){R.x=.5;C2*=.5/m5;}else{R.x+=m5*MB.z;}float o5=j2*n5(H3[0])/dot(c0[0],H3[0]);if(o5>=.5){R.y=.5;C2*=.5/o5;}else{R.y+=o5*MB.w;}}B0=R;R=h0(c0,R)+J.K0;if(k5){d D2=h0(H3,MB.zw);D2*=n5(D2)/dot(D2,D2);R+=j2*D2;}
#ifdef BB
i0=m4(V0(J.W0),J.g1,R);
#endif
g B=h2(R);S(B0);S(C2);
#ifdef BB
S(i0);
#endif
f1(B);}
#endif
#else
#ifdef W
T0(W1)q0(0,d,XB);U0 T0(k2)q0(1,d,YB);U0
#endif
z1 k0 I(0,d,B0);
#ifdef BB
k0 I(1,g,i0);
#endif
A1
#ifdef W
n4(PB,W1,X1,k2,l2,j){w0(j,X1,XB,d);w0(j,l2,YB,d);Q(B0,d);
#ifdef BB
Q(i0,g);
#endif
A c0=V0(J.l5);d R=h0(c0,XB)+J.K0;B0=YB;
#ifdef BB
i0=m4(V0(J.W0),J.g1,R);
#endif
g B=h2(R);S(B0);
#ifdef BB
S(i0);
#endif
f1(B);}
#endif
#endif
#endif
#ifdef AD
#ifdef W
T0(P)U0
#endif
z1 A1
#ifdef W
N1 O1 U1 V1 e1(PB,P,r,j,L){m0 h1;h1.x=(j&1)==0?v.o4.x:v.o4.z;h1.y=(j&2)==0?v.o4.y:v.o4.w;g B=h2(d(h1));f1(B);}
#endif
#endif
#ifdef DC
#define p5
#endif
#ifdef CC
#define p5
#endif
#ifdef HB
E2 w1(p4,EC);
#ifdef p5
w1(j3,NB);
#endif
F2 I3(p4,q5)
#ifdef p5
k3(j3,m2)
#endif
D1
#ifdef AB
#ifdef PC
C0(PC,X);
#else
C0(r5,X);
#endif
#endif
J3(v5,x0);
#ifdef DB
D0(w5,r0);
#endif
E1 K3 L3(F6,V8,QB);M3(G6,W8,IB);N3 uint H6(float x){return uint(x*I6+O3);}float q4(uint x){return float(x)*J6+(-O3*J6);}i r4(h X0,y0 E,M i1 v4 G2){h C=abs(X0);
#ifdef QC
if((E.x&K6)!=0u)C=1.-abs(fract(C*.5)*2.+-1.);
#endif
C=min(C,E0(1));
#ifdef DB
uint F1=E.x>>16u;if(F1!=0u){uint Y0=L0(r0);h H2=F1==(Y0>>16u)?unpackHalf2x16(Y0).x:.0;C=min(C,H2);}
#endif
i f=M0(0,0,0,0);uint G1=E.x&0xfu;switch(G1){case L6:f=unpackUnorm4x8(E.y);break;case w4:case M6:
#ifdef DC
case N6:
#endif
{A c0=V0(z0(IB,i1*4u));g K0=z0(IB,i1*4u+1u);d n2=h0(c0,n0)+K0.xy;
#ifdef DC
if(G1==N6){f=P3(sampler2D(floatBitsToUint(K0.zw)),m2,n2,c0[0],c0[1]);float I2=uintBitsToFloat(E.y);f.w*=I2;}else
#endif
{float t=G1==w4?n2.x:length(n2);t=clamp(t,.0,1.);float x=t*K0.z+K0.w;float y=uintBitsToFloat(E.y);f=M0(Q3(EC,q5,d(x,y),.0));}break;}
#ifdef DB
case x4:Z0(r0,E.y|packHalf2x16(Y1(C,0)));break;
#endif
}
#ifdef BB
if((E.x&X8)!=0u){A c0=V0(z0(IB,i1*4u+2u));g K0=z0(IB,i1*4u+3u);d Y8=h0(c0,n0)+K0.xy;l0 O6=Y1(abs(Y8)*K0.zw-K0.zw);h J2=clamp(min(O6.x,O6.y)+.5,.0,1.);C=min(C,J2);}
#endif
f.w*=C;return f;}i P6(i Q6,i P1){return Q6+P1*(1.-Q6.w);}
#ifdef AB
i x5(i R6,i P1,M Z1){if(Z1!=S6){
#ifdef KB
return D3(
#else
return E3(
#endif
R6,R3(P1),Z1);}else{return P6(o2(R6),P1);}}i T6(i f,y0 E G2){i P1=N0(X);M Z1=O0((E.x>>4)&0xfu);return x5(f,P1,Z1);}void y5(i f,y0 E G2){if(f.w!=.0){i Z8=T6(f,E j1);F0(X,Z8);}}
#endif
#ifdef AB
#define S3 Q1
#define U6 T3
#define l3 a2
#else
#define S3 m3
#define U6 y4
#define l3 U3
#endif
#ifdef JC
S3(JB){N(J0,l0);N(a0,M);d0(r0);d0(x0);
#ifdef AB
d0(X);
#else
P0=M0(0,0,0,0);
#endif
h C=min(min(J0.x,abs(J0.y)),E0(1));uint z4=H6(C);uint z5=(uint(a0)<<16)|z4;uint k1=A4(x0,z5);M l1=O0(k1>>16);if(l1!=a0){h X0=q4(k1&0xffffu);y0 E=p2(QB,l1);i f=r4(X0,E,l1 K2 j1);
#ifdef AB
y5(f,E j1);
#else
P0=o2(f);
#endif
}else if(J0.y<.0){if(k1<z5){z4+=k1-z5;}z4-=uint(O3);B4(x0,z4);}l3}
#endif
#ifdef EB
S3(JB){N(B1,h);N(a0,M);d0(r0);d0(x0);
#ifdef AB
d0(X);
#else
P0=M0(0,0,0,0);
#endif
h C=B1;uint k1=n3(x0);M l1=O0(k1>>16);h V6=q4(k1&0xffffu);if(l1!=a0){y0 E=p2(QB,l1);i f=r4(V6,E,l1 K2 j1);
#ifdef AB
y5(f,E j1);
#else
P0=o2(f);
#endif
}else{C+=V6;}o3(x0,(uint(a0)<<16)|H6(C));l3}
#endif
#ifdef CC
U6(JB){N(B0,d);
#ifdef KC
N(C2,h);
#endif
#ifdef BB
N(i0,g);
#endif
d0(r0);d0(x0);
#ifdef AB
d0(X);
#endif
i p3=L2(NB,m2,B0);h M2=1.;
#ifdef KC
M2=min(C2,M2);
#endif
#ifdef BB
h J2=A5(M0(i0));M2=clamp(J2,E0(0),M2);
#endif
#ifdef RC
m1;
#endif
uint k1=n3(x0);h X0=q4(k1&0xffffu);M l1=O0(k1>>16);y0 W6=p2(QB,l1);i B5=r4(X0,W6,l1 K2 j1);
#ifdef DB
if(J.F1!=0u){C4(r0);uint Y0=L0(r0);uint F1=Y0>>16;h H2=F1==J.F1?unpackHalf2x16(Y0).x:0;M2=min(M2,H2);}
#endif
p3.w*=M2*J.I2;
#ifdef AB
if(B5.w!=.0||p3.w!=0){i P1=N0(X);M a9=O0((W6.x>>4)&0xfu);M c9=O0(J.Z1);P1=x5(B5,P1,a9);p3=x5(p3,P1,c9);F0(X,p3);}
#else
P0=P6(o2(p3),o2(B5));
#endif
o3(x0,uint(O3));
#ifdef RC
n1;
#endif
l3}
#endif
#ifdef BD
S3(JB){
#ifndef AB
P0=M0(0,0,0,0);
#endif
#ifdef CD
F0(X,unpackUnorm4x8(v.d9));
#endif
#ifdef DD
i f=N0(X);F0(X,f.zyxw);
#endif
o3(x0,v.e9);
#ifdef DB
Z0(r0,0u);
#endif
l3}
#endif
#ifdef ED
#ifdef SC
m3(JB)
#else
S3(JB)
#endif
{
#ifdef AB
d0(X);
#endif
uint k1=n3(x0);h X0=q4(k1&0xffffu);M l1=O0(k1>>16);y0 E=p2(QB,l1);i f=r4(X0,E,l1 K2 j1);
#ifdef SC
P0=T6(f,E j1);U3
#else
#ifdef AB
y5(f,E j1);
#else
P0=o2(f);
#endif
l3
#endif
}
#endif
#endif
)===";
} // namespace glsl
} // namespace pls
} // namespace rive