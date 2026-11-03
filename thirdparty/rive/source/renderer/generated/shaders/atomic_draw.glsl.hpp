#pragma once

#include "atomic_draw.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char atomic_draw[] = R"===(#ifdef SD
#ifdef CB
h1(d0) K(0,e,YB);K(1,e,ZB);i1
#endif
o2
#ifdef IB
I0 W(0,e,S);
#else
I0 W(0,F,S);
#endif
l3 W(1,O,J0);P1
#ifdef CB
n1(UB,d0,A,C,r){L(C,A,YB,e);L(C,A,ZB,e);
#ifdef IB
V(S,e);
#else
V(S,F);
#endif
V(J0,O);e I;uint g0;c i0;e T;if(xa(YB,ZB,r,g0,i0,T S3)){
#ifdef IB
S=T;
#else
S.xy=N8(T.xy);
#endif
J0=Z1(g0);I=T3(i0);}else{I=e(j.m3,j.m3,j.m3,j.m3);}e0(S);e0(J0);C1(I);}
#endif
#endif
#if defined(DB)||defined(FB)
#ifdef CB
h1(d0) K(0,l4,MB);i1
#endif
o2
#ifdef FB
I0 W(0,c,X2);
#else
NB W(0,d,v1);
#endif
l3 W(1,O,J0);P1
#ifdef CB
n1(UB,d0,A,C,r){L(C,A,MB,M);
#ifdef FB
V(X2,c);
#else
V(v1,d);
#endif
V(J0,O);uint g0;c i0;
#ifdef FB
i0=nd(MB,g0,X2 S3);
#else
i0=od(MB,g0,v1 S3);
#endif
J0=Z1(g0);e I=T3(i0);
#ifdef FB
e0(X2);
#else
e0(v1);
#endif
e0(J0);C1(I);}
#endif
#endif
#ifdef ED
#ifdef CB
h1(d0) K(0,e,HC);i1 h1(H1) K(ya,e,AC);K(za,e,VB);K(Aa,e,QB);K(Ba,uint,BC);K(Ca,uint,CC);K(Da,uint,DC);K(Ea,uint,NC);K(yg,e,TD);K(zg,e,UD);K(Ag,e,FD);K(pd,e,GD);i1
#endif
o2 I0 W(0,c,p2);I0 W(1,d,E5);I0 W(2,e,F5);
#ifdef AB
I0 W(3,e,Y0);
#endif
NB W(4,i,a2);
#ifdef N
l3 W(5,O,U3);
#endif
#ifdef H
l3 W(6,O,Q1);
#endif
P1
#ifdef CB
O8(UB,d0,A,H1,j0,C,r){L(C,A,HC,e);L(r,j0,AC,e);L(r,j0,VB,e);L(r,j0,QB,e);L(r,j0,BC,uint);L(r,j0,CC,uint);L(r,j0,DC,uint);L(r,j0,NC,uint);L(r,j0,TD,e);L(r,j0,UD,e);L(r,j0,FD,e);L(r,j0,GD,e);V(p2,c);V(E5,d);V(F5,e);
#ifdef AB
V(Y0,e);
#endif
V(a2,i);
#ifdef N
V(U3,O);
#endif
#ifdef H
V(Q1,O);
#endif
bool Fa=HC.z==.0||HC.w==.0;E5=Fa?.0:1.;c i0=HC.xy;X Q0=w1(AC);X m7=transpose(inverse(Q0));if(!Fa){float Ga=Z4*Ha(m7[1])/dot(Q0[1],m7[1]);if(Ga>=.5){i0.x=.5;E5*=a5(.5/Ga);}else{i0.x+=Ga*HC.z;}float Ia=Z4*Ha(m7[0])/dot(Q0[0],m7[0]);if(Ia>=.5){i0.y=.5;E5*=a5(.5/Ia);}else{i0.y+=Ia*HC.w;}}X Bg=w1(TD);p2=G0(Bg,i0)+FD.xy;i0=G0(Q0,i0)+QB.xy;if(Fa){c m4=G0(m7,HC.zw);m4*=Ha(m4)/dot(m4,m4);i0+=Z4*m4;}
#ifdef AB
if(AB){Y0=P8(w1(VB),QB.zw,i0);}
#endif
a2=unpackUnorm4x8(BC);
#ifdef N
U3=Z1(CC);
#endif
#ifdef H
Q1=Z1(DC);
#endif
e I=T3(i0);c n0=i0;
#ifdef VD
if(j.Ja!=0u){n0.y=float(j.Ka)-n0.y;}
#endif
uint A2=floatBitsToUint(GD.z);if(A2!=0){X Cg=w1(UD);c Dg=FD.zw;F5=La(n0,Cg,Dg,GD.xy,A2,1.0,1.0);}else{F5=e(.0,.0,.0,.0);}e0(p2);e0(E5);e0(F5);
#ifdef AB
e0(Y0);
#endif
e0(a2);
#ifdef N
e0(U3);
#endif
#ifdef H
e0(Q1);
#endif
C1(I);}
#endif
#elif defined(OB)
#ifdef CB
h1(G3) K(0,c,SC);i1 h1(V3) K(1,c,TC);i1 h1(H1) K(ya,e,AC);K(za,e,VB);K(Aa,e,QB);K(Ba,uint,BC);K(Ca,uint,CC);K(Da,uint,DC);K(Ea,uint,NC);K(Ma,e,IC);i1
#endif
o2 I0 W(0,c,p2);
#ifdef AB
I0 W(1,e,Y0);
#endif
NB W(3,i,a2);
#ifdef N
l3 W(4,O,U3);
#endif
#ifdef H
l3 W(5,O,Q1);
#endif
P1
#ifdef CB
n7(UB,G3,H3,V3,n3,H1,j0,C){L(C,H3,SC,c);L(C,n3,TC,c);L(r,j0,AC,e);L(r,j0,VB,e);L(r,j0,QB,e);L(r,j0,BC,uint);L(r,j0,CC,uint);L(r,j0,DC,uint);L(r,j0,NC,uint);L(r,j0,IC,e);V(p2,c);
#ifdef AB
V(Y0,e);
#endif
V(a2,i);
#ifdef N
V(U3,O);
#endif
#ifdef H
V(Q1,O);
#endif
X Q0=w1(AC);c i0=G0(Q0,SC)+QB.xy;p2=TC*IC.zw+IC.xy;
#ifdef AB
if(AB){Y0=P8(w1(VB),QB.zw,i0);}
#endif
a2=unpackUnorm4x8(BC);
#ifdef N
U3=Z1(CC);
#endif
#ifdef H
Q1=Z1(DC);
#endif
e I=T3(i0);e0(p2);
#ifdef AB
e0(Y0);
#endif
e0(a2);
#ifdef N
e0(U3);
#endif
#ifdef H
e0(Q1);
#endif
C1(I);}
#endif
#endif
#ifdef OF
#ifdef CB
h1(d0) i1
#endif
o2 P1
#ifdef CB
n1(UB,d0,A,C,r){f0 I2;I2.x=(C&1)==0?j.Q8.x:j.Q8.z;I2.y=(C&2)==0?j.Q8.y:j.Q8.w;e I=T3(c(I2));C1(I);}
#endif
#endif
#ifdef SE
#endif
#if defined(TE)&&!defined(Q)
#endif
#ifdef EB
I1
#ifndef Q
#ifdef UE
#define Na UE
#else
#define Na B2
#endif
#ifdef HD
c5(Na,q0);
#else
z0(Na,q0);
#endif
#endif
#ifdef XC
#define d5 i
#define Oa L0
#define R8 A0(.0)
#define qd(G) ((G).w!=.0)
#ifdef N
#ifndef YC
z0(o3,o0);
#else
c5(o3,o0);
#endif
#endif
#else
#define d5 uint
#define R8 0u
#define Oa o1
#define qd(G) ((G)!=0u)
#ifdef N
x1(o3,o0);
#endif
#endif
Y2(o7,e5);J1 n4 o4(Pa,rd,JB);p6(sd,Fg,KB);p4 f uint Gg(float x){return uint(round(x*Qa+Ra));}f d S8(uint x){return a5(float(x)*td+(-Ra*td));}O T8(O g0){
#ifdef PF
g0=min(g0,j.Hg);
#endif
return g0;}
#ifdef N
f void ud(uint D1,d5 Z0,p7(d) n){
#ifdef XC
if(all(lessThan(abs(Z0.xy-unpackUnorm4x8(D1).xy),V2(.25/255.)))) n=min(n,Z0.z);else n=.0;
#else
if(D1==Z0>>16) n=min(n,unpackHalf2x16(Z0).x);else n=.0;
#endif
}
#endif
f void U8(uint g0,d x0,p1(i) P
#if defined(N)&&!defined(YC)
,p7(d5) K1
#endif
q7 r4){R0 y1=p0(JB,g0*4u+1u).zw;d n=x0;if((y1.x&(Ig|Sa))!=0u){n=abs(n);
#ifdef ZC
if(ZC&&(y1.x&Sa)!=0u){n=1.-abs(fract(n*.5)*2.+-1.);}
#endif
}n=clamp(n,B0(.0),B0(1.));
#ifdef N
if(N){uint D1=y1.x>>16u;if(D1!=0u){ud(D1,Oa(o0),n);}}
#endif
#ifdef AB
if(AB&&(y1.x&Jg)!=0u){X Q0=w1(p0(KB,g0*q2+2u));e C2=p0(KB,g0*q2+3u);c Kg=G0(Q0,Y)+C2.xy;F vd=N8(abs(Kg)*C2.zw-C2.zw);d G5=clamp(min(vd.x,vd.y)+.5,.0,1.);n=min(n,G5);}
#endif
uint A2=y1.x&0xfu;O c2=Z1((y1.x>>4)&0xfu);
#ifdef H
bool J2=H&&c2!=W3;
#else
const bool J2=false;
#endif
if(A2<=Ta){P=unpackUnorm4x8(y1.y);
#ifdef N
if(N&&A2==V8){
#ifndef YC
#ifdef XC
K1.xy=P.zw;K1.z=n;K1.w=1.;
#else
K1=y1.y|packHalf2x16(V2(n,.0));
#endif
#endif
P=A0(.0);}
#endif
}else{X Q0=w1(p0(KB,g0*q2));e C2=p0(KB,g0*q2+1u);c Lg=G0(Q0,Y)+C2.xy;c W8=C2.zw;bool v4=W8.x<0.0;W8.x=max(0.0,W8.x);c n3=Mg(float(A2),Lg,W8,v4,j.X8,j.Y8);P=r2(AD,Z8,n3,0.0);if(!J2){P.xyz*=P.w;float w4=uintBitsToFloat(y1.y);P.w*=abs(w4);}}
#if!defined(Q)&&defined(H)
if(J2){if(P.w*n!=.0){i X0=L0(q0);P.xyz=Y4(P.xyz,X0,c2);}P.xyz*=P.w;}
#endif
P*=n;}
#if!defined(Q)&&!defined(HD)
f void a9(i P r4){
#ifndef XC
if(P.x+P.y+P.z+P.w==.0) return;if(P.w!=1.) P=Va(P,L0(q0));
#endif
y0(q0,P);}
#endif
#if defined(N)&&!defined(YC)
f void Wa(d5 K1 r4){
#ifdef XC
y0(o0,K1);
#else
if(K1!=0u) q1(o0,K1);
#endif
}
#endif
#ifdef Q
#define q6 K2
#define r6 I3
#else
#define q6 E1
#define r6 L1
#endif
#ifdef SD
q6(HB){
#ifdef IB
q(S,e);
#else
q(S,F);
#endif
q(J0,O);d c9;
#ifdef IB
if(IB&&wd(S)){c9=f5(S r1);}else if(IB&&xd(S)){c9=d9(S r1);}else
#endif
{c9=min(min(B0(S.x),abs(B0(S.y))),B0(1.));}i P=A0(.0);
#ifdef N
d5 K1=R8;
#endif
uint e9=Gg(c9);uint yd=(zd(J0)<<v6)|e9;uint L2=H5(e5,yd);O R1=Z1(L2>>v6);R1=T8(R1);if(R1==J0){if(!w6(S)){e9+=L2-max(yd,L2);e9-=Xa;I5(e5,e9);}}else{d x0=S8(L2&f9);U8(R1,x0,P
#ifdef N
,K1
#endif
q3 d2);}P.xyz=M2(P.xyz,P.w,Y.xy,j.J3,j.K3);
#ifdef Q
S1=P;
#else
a9(P d2);
#endif
#ifdef N
Wa(K1 d2);
#endif
r6}
#endif
#if defined(DB)||defined(FB)
q6(HB){
#ifdef FB
q(X2,c);
#else
q(v1,d);
#endif
q(J0,O);uint L2=r3(e5);O R1=Z1(L2>>v6);R1=T8(R1);uint Ya;
#ifndef FB
if(R1==J0){Ya=L2;}else
#endif
{Ya=(zd(J0)<<v6)+Xa;}d n;
#ifdef FB
n=clamp(r2(ID,Za,X2,.0).x,B0(.0),B0(1.));
#else
n=v1;
#endif
int Ng=int(round(n*Qa));v3(e5,Ya+uint(Ng));i P=A0(.0);
#ifdef N
d5 K1=R8;
#endif
#ifndef FB
if(R1!=J0)
#endif
{d ab=S8(L2&f9);U8(R1,ab,P
#ifdef N
,K1
#endif
q3 d2);}P.xyz=M2(P.xyz,P.w,Y.xy,j.J3,j.K3);
#ifdef Q
S1=P;
#else
a9(P d2);
#endif
#ifdef N
Wa(K1 d2);
#endif
r6}
#endif
#ifdef SE
q6(HB){q(p2,c);
#ifdef ED
q(E5,d);q(F5,e);
#endif
#ifdef AB
q(Y0,e);
#endif
q(a2,i);
#ifdef N
q(U3,O);
#endif
#ifdef H
q(Q1,O);
#endif
i T1=g9(SB,x4,p2);d x6=1.;
#ifdef ED
x6=min(E5,x6);
#endif
#ifdef AB
if(AB){d G5=E3(y4(Y0));x6=clamp(G5,B0(.0),x6);}
#endif
uint L2=r3(e5);O R1=Z1(L2>>v6);R1=T8(R1);d ab=S8(L2&f9);i P;
#ifdef N
d5 K1=R8;
#endif
U8(R1,ab,P
#ifdef N
,K1
#endif
q3 d2);
#ifdef N
if(N&&U3!=0u){d5 Z0=qd(K1)?K1:Oa(o0);ud(U3,Z0,x6);}
#endif
#ifdef ED
if(F5.w!=0.0){c bb=cb(F5,j.X8,j.Y8);i db=r2(AD,Z8,bb,0.0);db.xyz*=db.w;T1*=db;}
#endif
T1*=a2;
#if!defined(Q)&&defined(H)
if(H&&Q1!=W3){i X0=L0(q0)*(1.-P.w)+P;T1.xyz=Y4(o6(T1),X0,Q1)*T1.w;}
#endif
T1*=x6;P=P*(1.-T1.w)+T1;P.xyz=M2(P.xyz,P.w,Y.xy,j.J3,j.K3);
#ifdef Q
S1=P;
#else
a9(P d2);
#endif
#ifdef N
Wa(K1 d2);
#endif
v3(e5,Xa);r6}
#endif
#ifdef TE
q6(HB){
#ifndef Q
#ifdef WD
if(WD){y0(q0,unpackUnorm4x8(j.Og));}
#endif
#ifdef XD
if(XD){y0(q0,j1(SB,D));}
#endif
#ifdef QF
i k=L0(q0);y0(q0,k.zyxw);
#endif
#endif
v3(e5,j.Qg);
#ifdef N
if(N){q1(o0,0u);}
#endif
#ifdef Q
discard;
#endif
r6}
#endif
#ifdef YC
#ifdef HD
K2(HB)
#else
q6(HB)
#endif
{uint L2=r3(e5);d x0=S8(L2&f9);O R1=Z1(L2>>v6);R1=T8(R1);i P;U8(R1,x0,P q3 d2);
#ifdef HD
float Ad=1.-P.w;if(Ad!=.0) P+=L0(q0)*Ad;S1=P;I3
#else
P.xyz=M2(P.xyz,P.w,Y.xy,j.J3,j.K3);
#ifdef Q
S1=P;
#else
a9(P d2);
#endif
r6
#endif
}
#endif
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive