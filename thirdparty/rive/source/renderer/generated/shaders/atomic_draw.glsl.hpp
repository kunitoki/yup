#pragma once

#include "atomic_draw.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char atomic_draw[] = R"===(#ifdef MD
#ifdef BB
c1(d0) K(0,e,WB);K(1,e,XB);d1
#endif
l2
#ifdef HB
E0 V(0,e,S);
#else
E0 V(0,C,S);
#endif
Z2 V(1,Q,F0);e2
#ifdef BB
v1(RB,d0,D,G,r){L(G,D,WB,e);L(G,D,XB,e);
#ifdef HB
T(S,e);
#else
T(S,C);
#endif
T(F0,Q);e I;uint a0;c k0;e U;if(K9(WB,XB,r,a0,k0,U G3)){
#ifdef HB
S=U;
#else
S.xy=g8(U.xy);
#endif
F0=P1(a0);I=H3(k0);}else{I=e(j.a3,j.a3,j.a3,j.a3);}Z(S);Z(F0);w1(I);}
#endif
#endif
#if defined(DB)||defined(EB)
#ifdef BB
c1(d0) K(0,c4,MB);d1
#endif
l2
#ifdef EB
E0 V(0,c,J2);
#else
KB V(0,d,m1);
#endif
Z2 V(1,Q,F0);e2
#ifdef BB
v1(RB,d0,D,G,r){L(G,D,MB,O);
#ifdef EB
T(J2,c);
#else
T(m1,d);
#endif
T(F0,Q);uint a0;c k0;
#ifdef EB
k0=kc(MB,a0,J2 G3);
#else
k0=lc(MB,a0,m1 G3);
#endif
F0=P1(a0);e I=H3(k0);
#ifdef EB
Z(J2);
#else
Z(m1);
#endif
Z(F0);w1(I);}
#endif
#endif
#ifdef BD
#ifdef BB
c1(d0) K(0,e,GC);d1 c1(A1) K(L9,e,YB);K(M9,e,SB);K(N9,e,PB);K(O9,uint,ZB);K(P9,uint,AC);K(Q9,uint,BC);K(R9,uint,LC);K(xf,e,ND);K(yf,e,OD);K(zf,e,CD);K(mc,e,OC);d1
#endif
l2 E0 V(0,c,f2);E0 V(1,d,i5);E0 V(2,e,j5);
#ifdef AB
E0 V(3,e,R0);
#endif
KB V(4,i,Q1);
#ifdef A
Z2 V(5,Q,I3);
#endif
#ifdef N
Z2 V(6,Q,H1);
#endif
e2
#ifdef BB
h8(RB,d0,D,A1,h0,G,r){L(G,D,GC,e);L(r,h0,YB,e);L(r,h0,SB,e);L(r,h0,PB,e);L(r,h0,ZB,uint);L(r,h0,AC,uint);L(r,h0,BC,uint);L(r,h0,LC,uint);L(r,h0,ND,e);L(r,h0,OD,e);L(r,h0,CD,e);L(r,h0,OC,e);T(f2,c);T(i5,d);T(j5,e);
#ifdef AB
T(R0,e);
#endif
T(Q1,i);
#ifdef A
T(I3,Q);
#endif
#ifdef N
T(H1,Q);
#endif
bool S9=GC.z==.0||GC.w==.0;i5=S9?.0:1.;c k0=GC.xy;Y S0=n1(YB);Y S6=transpose(inverse(S0));if(!S9){float T9=H4*U9(S6[1])/dot(S0[1],S6[1]);if(T9>=.5){k0.x=.5;i5*=d4(.5/T9);}else{k0.x+=T9*GC.z;}float V9=H4*U9(S6[0])/dot(S0[0],S6[0]);if(V9>=.5){k0.y=.5;i5*=d4(.5/V9);}else{k0.y+=V9*GC.w;}}Y Af=n1(ND);f2=M0(Af,k0)+CD.xy;k0=M0(S0,k0)+PB.xy;if(S9){c e4=M0(S6,GC.zw);e4*=U9(e4)/dot(e4,e4);k0+=H4*e4;}
#ifdef AB
if(AB){R0=i8(n1(SB),PB.zw,k0);}
#endif
Q1=unpackUnorm4x8(ZB);
#ifdef A
I3=P1(AC);
#endif
#ifdef N
H1=P1(BC);
#endif
e I=H3(k0);c l0=k0;
#ifdef PD
if(j.W9!=0u){l0.y=float(j.X9)-l0.y;}
#endif
if(OC.w!=0.0){Y Bf=n1(OD);c Cf=CD.zw;j5=Y9(l0,Bf,Cf,OC.w,OC.xy,OC.z);}else{j5=e(.0,.0,.0,.0);}Z(f2);Z(i5);Z(j5);
#ifdef AB
Z(R0);
#endif
Z(Q1);
#ifdef A
Z(I3);
#endif
#ifdef N
Z(H1);
#endif
w1(I);}
#endif
#elif defined(NB)
#ifdef BB
c1(w3) K(0,c,PC);d1 c1(J3) K(1,c,QC);d1 c1(A1) K(L9,e,YB);K(M9,e,SB);K(N9,e,PB);K(O9,uint,ZB);K(P9,uint,AC);K(Q9,uint,BC);K(R9,uint,LC);K(Z9,e,HC);d1
#endif
l2 E0 V(0,c,f2);
#ifdef AB
E0 V(1,e,R0);
#endif
KB V(3,i,Q1);
#ifdef A
Z2 V(4,Q,I3);
#endif
#ifdef N
Z2 V(5,Q,H1);
#endif
e2
#ifdef BB
T6(RB,w3,x3,J3,K3,A1,h0,G){L(G,x3,PC,c);L(G,K3,QC,c);L(r,h0,YB,e);L(r,h0,SB,e);L(r,h0,PB,e);L(r,h0,ZB,uint);L(r,h0,AC,uint);L(r,h0,BC,uint);L(r,h0,LC,uint);L(r,h0,HC,e);T(f2,c);
#ifdef AB
T(R0,e);
#endif
T(Q1,i);
#ifdef A
T(I3,Q);
#endif
#ifdef N
T(H1,Q);
#endif
Y S0=n1(YB);c k0=M0(S0,PC)+PB.xy;f2=QC*HC.zw+HC.xy;
#ifdef AB
if(AB){R0=i8(n1(SB),PB.zw,k0);}
#endif
Q1=unpackUnorm4x8(ZB);
#ifdef A
I3=P1(AC);
#endif
#ifdef N
H1=P1(BC);
#endif
e I=H3(k0);Z(f2);
#ifdef AB
Z(R0);
#endif
Z(Q1);
#ifdef A
Z(I3);
#endif
#ifdef N
Z(H1);
#endif
w1(I);}
#endif
#endif
#ifdef IF
#ifdef BB
c1(d0) d1
#endif
l2 e2
#ifdef BB
v1(RB,d0,D,G,r){e0 y2;y2.x=(G&1)==0?j.j8.x:j.j8.z;y2.y=(G&2)==0?j.j8.y:j.j8.w;e I=H3(c(y2));w1(I);}
#endif
#endif
#ifdef ME
#endif
#if defined(NE)&&!defined(W)
#endif
#ifdef FB
R1
#ifndef W
#ifdef OE
#define aa OE
#else
#define aa K2
#endif
#ifdef DD
I4(aa,n0);
#else
B0(aa,n0);
#endif
#endif
#ifdef UC
#define J4 i
#define ba N0
#define k8 I0(.0)
#define nc(F) ((F).w!=.0)
#ifdef A
#ifndef VC
B0(c3,m0);
#else
I4(c3,m0);
#endif
#endif
#else
#define J4 uint
#define k8 0u
#define ba h1
#define nc(F) ((F)!=0u)
#ifdef A
o1(c3,m0);
#endif
#endif
L2(U6,K4);S1 f4 Z5(oc,Ef,WC);a6(pc,Ff,JB);g4 f uint Gf(float x){return uint(round(x*ca+da));}f d l8(uint x){return d4(float(x)*qc+(-da*qc));}Q m8(Q a0){
#ifdef JF
a0=min(a0,j.Hf);
#endif
return a0;}
#ifdef A
f void rc(uint X0,J4 T0,V6(d) n){
#ifdef UC
if(all(lessThan(abs(T0.xy-unpackUnorm4x8(X0).xy),H2(.25/255.)))) n=min(n,T0.z);else n=.0;
#else
if(X0==T0>>16) n=min(n,unpackHalf2x16(T0).x);else n=.0;
#endif
}
#endif
f void n8(uint a0,d w0,i1(i) P
#if defined(A)&&!defined(VC)
,V6(J4) B1
#endif
W6 h4){O0 G0=l5(WC,a0);d n=w0;if((G0.x&(If|ea))!=0u){n=abs(n);
#ifdef XC
if(XC&&(G0.x&ea)!=0u){n=1.-abs(fract(n*.5)*2.+-1.);}
#endif
}n=clamp(n,H0(.0),H0(1.));
#ifdef A
if(A){uint X0=G0.x>>16u;if(X0!=0u){rc(X0,ba(m0),n);}}
#endif
#ifdef AB
if(AB&&(G0.x&Jf)!=0u){Y S0=n1(p0(JB,a0*g2+2u));e m2=p0(JB,a0*g2+3u);c Kf=M0(S0,f0)+m2.xy;C sc=g8(abs(Kf)*m2.zw-m2.zw);d m5=clamp(min(sc.x,sc.y)+.5,.0,1.);n=min(n,m5);}
#endif
uint n2=G0.x&0xfu;Q y3=P1((G0.x>>4)&0xfu);
#ifdef N
bool n5=N&&y3!=L4;
#else
const bool n5=false;
#endif
if(n2<=fa){P=unpackUnorm4x8(G0.y);
#ifdef A
if(A&&n2==o5){
#ifndef VC
#ifdef UC
B1.xy=P.zw;B1.z=n;B1.w=1.;
#else
B1=G0.y|packHalf2x16(H2(n,.0));
#endif
#endif
P=I0(.0);}
#endif
}else{Y S0=n1(p0(JB,a0*g2));e m2=p0(JB,a0*g2+1u);c tc=M0(S0,f0)+m2.xy;float t=n2==uc?tc.x:length(tc);t=clamp(t,.0,1.);float x=t*m2.z+m2.w;float vc=uintBitsToFloat(G0.y);float Lf=floor(vc)*j.wc+j.xc;P=o2(ED,ha,c(x,Lf),.0);if(!n5){P.xyz*=P.w;d ia=d4(fract(vc)*(256./255.));P.w*=ia;}}
#if!defined(W)&&defined(N)
if(n5){if(P.w*n!=.0){i I1=N0(n0);P.xyz=h5(P.xyz,I1,y3);}P.xyz*=P.w;}
#endif
P*=n;}
#if!defined(W)&&!defined(DD)
f void o8(i P h4){
#ifndef UC
if(P.x+P.y+P.z+P.w==.0) return;float X6=1.-P.w;if(X6!=.0) P+=N0(n0)*X6;
#endif
y0(n0,P);}
#endif
#if defined(A)&&!defined(VC)
f void ja(J4 B1 h4){
#ifdef UC
y0(m0,B1);
#else
if(B1!=0u) j1(m0,B1);
#endif
}
#endif
#ifdef W
#define c6 z2
#define d6 z3
#else
#define c6 T1
#define d6 h2
#endif
#ifdef MD
c6(IB){
#ifdef HB
q(S,e);
#else
q(S,C);
#endif
q(F0,Q);d p8;
#ifdef HB
if(HB&&yc(S)){p8=M4(S k1);}else if(HB&&zc(S)){p8=q8(S k1);}else
#endif
{p8=min(min(H0(S.x),abs(H0(S.y))),H0(1.));}i P=I0(.0);
#ifdef A
J4 B1=k8;
#endif
uint r8=Gf(p8);uint Ac=(Bc(F0)<<e6)|r8;uint A2=p5(K4,Ac);Q J1=P1(A2>>e6);J1=m8(J1);if(J1==F0){if(!f6(S)){r8+=A2-max(Ac,A2);r8-=ka;q5(K4,r8);}}else{d w0=l8(A2&v8);n8(J1,w0,P
#ifdef A
,B1
#endif
e3 U1);}P.xyz=M2(P.xyz,P.w,f0.xy,j.L3,j.M3);
#ifdef W
K1=P;
#else
o8(P U1);
#endif
#ifdef A
ja(B1 U1);
#endif
d6}
#endif
#if defined(DB)||defined(EB)
c6(IB){
#ifdef EB
q(J2,c);
#else
q(m1,d);
#endif
q(F0,Q);uint A2=f3(K4);Q J1=P1(A2>>e6);J1=m8(J1);uint la;
#ifndef EB
if(J1==F0){la=A2;}else
#endif
{la=(Bc(F0)<<e6)+ka;}d n;
#ifdef EB
n=clamp(o2(FD,ma,J2,.0).x,H0(.0),H0(1.));
#else
n=m1;
#endif
int Mf=int(round(n*ca));g3(K4,la+uint(Mf));i P=I0(.0);
#ifdef A
J4 B1=k8;
#endif
#ifndef EB
if(J1!=F0)
#endif
{d na=l8(A2&v8);n8(J1,na,P
#ifdef A
,B1
#endif
e3 U1);}P.xyz=M2(P.xyz,P.w,f0.xy,j.L3,j.M3);
#ifdef W
K1=P;
#else
o8(P U1);
#endif
#ifdef A
ja(B1 U1);
#endif
d6}
#endif
#ifdef ME
c6(IB){q(f2,c);
#ifdef BD
q(i5,d);q(j5,e);
#endif
#ifdef AB
q(R0,e);
#endif
q(Q1,i);
#ifdef A
q(I3,Q);
#endif
#ifdef N
q(H1,Q);
#endif
i N2=w8(CC,r5,f2);d g6=1.;
#ifdef BD
g6=min(i5,g6);
#endif
#ifdef AB
if(AB){d m5=v3(v5(R0));g6=clamp(m5,H0(.0),g6);}
#endif
uint A2=f3(K4);Q J1=P1(A2>>e6);J1=m8(J1);d na=l8(A2&v8);i P;
#ifdef A
J4 B1=k8;
#endif
n8(J1,na,P
#ifdef A
,B1
#endif
e3 U1);
#ifdef A
if(A&&I3!=0u){J4 T0=nc(B1)?B1:ba(m0);rc(I3,T0,g6);}
#endif
#ifdef BD
if(j5.w!=0.0){c oa=Cc(j5);i pa=o2(ED,ha,oa,0.0);pa.xyz*=pa.w;N2*=pa;}
#endif
N2*=Q1;
#if!defined(W)&&defined(N)
if(N&&H1!=L4){i I1=N0(n0)*(1.-P.w)+P;N2.xyz=h5(Q6(N2),I1,H1)*N2.w;}
#endif
N2*=g6;P=P*(1.-N2.w)+N2;P.xyz=M2(P.xyz,P.w,f0.xy,j.L3,j.M3);
#ifdef W
K1=P;
#else
o8(P U1);
#endif
#ifdef A
ja(B1 U1);
#endif
g3(K4,ka);d6}
#endif
#ifdef NE
c6(IB){
#ifndef W
#ifdef QD
if(QD){y0(n0,unpackUnorm4x8(j.Nf));}
#endif
#ifdef RD
if(RD){y0(n0,p1(CC,H));}
#endif
#ifdef KF
i p=N0(n0);y0(n0,p.zyxw);
#endif
#endif
g3(K4,j.Of);
#ifdef A
if(A){j1(m0,0u);}
#endif
#ifdef W
discard;
#endif
d6}
#endif
#ifdef VC
#ifdef DD
z2(IB)
#else
c6(IB)
#endif
{uint A2=f3(K4);d w0=l8(A2&v8);Q J1=P1(A2>>e6);J1=m8(J1);i P;n8(J1,w0,P e3 U1);
#ifdef DD
float X6=1.-P.w;if(X6!=.0) P+=N0(n0)*X6;K1=P;z3
#else
P.xyz=M2(P.xyz,P.w,f0.xy,j.L3,j.M3);
#ifdef W
K1=P;
#else
o8(P U1);
#endif
d6
#endif
}
#endif
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive