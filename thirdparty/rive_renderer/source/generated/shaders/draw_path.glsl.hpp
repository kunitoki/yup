#pragma once

#include "draw_path.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_path[] = R"===(#ifdef FB
#define o6 !FB
#else
#define o6 true
#endif
#ifdef AB
U0(f0)
#ifdef GB
i0(0,a4,IB);
#else
i0(0,f,LB);i0(1,f,MB);
#endif
V0
#endif
o1 n0 H(0,f,H1);
#ifdef CB
n0 H(1,c,Q0);
#elif!defined(DB)
#ifdef GB
OB H(1,g,i1);
#elif defined(EB)
n0 H(2,f,D);
#else
n0 H(2,G,D);
#endif
OB H(3,g,j0);
#endif
#ifdef T
OB H(4,G,i3);
#endif
#ifdef BB
n0 H(5,f,R0);
#endif
#ifdef FB
OB H(6,g,U3);
#endif
p1
#ifdef AB
q1(PB,f0,B,n,K){
#ifdef GB
l0(n,B,IB,Z);
#else
l0(n,B,LB,f);l0(n,B,MB,f);
#endif
L(H1,f);
#ifdef CB
L(Q0,c);
#elif!defined(DB)
#ifdef GB
L(i1,g);
#elif defined(EB)
L(D,f);
#else
L(D,G);
#endif
L(j0,g);
#endif
#ifdef T
L(i3,G);
#endif
#ifdef BB
L(R0,f);
#endif
#ifdef FB
L(U3,g);
#endif
bool Eb=false;uint R;c J;
#ifdef DB
a0 z7;
#endif
#ifdef CB
J=a8(IB,R,
#ifdef DB
z7,
#endif
Q0 Y1);
#elif defined(GB)
J=c8(IB,R
#ifdef DB
,z7
#else
,i1
#endif
Y1);
#else
f O;Eb=!E6(LB,MB,K,R,J
#ifndef DB
,O
#else
,z7
#endif
Y1);
#ifndef DB
#ifdef EB
D=O;
#else
D.xy=F6(O.xy);
#endif
#endif
#endif
N0 F0=k4(DC,R);
#if!defined(CB)&&!defined(DB)
j0=g7(R,q.d5);if((F0.x&p8)!=0u)j0=-j0;
#endif
uint J1=F0.x&0xfu;
#ifdef T
if(T){uint te=(J1==N6?F0.y:F0.x)>>16;g Z0=g7(te,q.d5);if(J1==N6)Z0=-Z0;i3.x=Z0;}
#endif
#ifdef FB
if(FB){U3=float((F0.x>>4)&0xfu);}
#endif
c p6=J;
#ifdef DE
p6.y=float(q.id)-p6.y;
#endif
#ifdef BB
if(BB){S R1=D1(w0(KB,R*4u+2u));f Z1=w0(KB,R*4u+3u);
#ifndef DB
R0=I6(R1,Z1.xy,p6);
#else
Ka(R1,Z1.xy,p6);
#endif
}
#endif
if(J1==q8){i j=unpackUnorm4x8(F0.y);if(o6)j.xyz*=j.w;H1=f(j);}
#ifdef T
else if(T&&J1==N6){g A7=g7(F0.x>>16,q.d5);i3.y=A7;}
#endif
else{S ue=D1(w0(KB,R*4u));f B7=w0(KB,R*4u+1u);c G2=C0(ue,p6)+B7.xy;if(J1==O6||J1==td){H1.w=-uintBitsToFloat(F0.y);float ve=B7.z;if(ve>.9){H1.z=2.;}else{H1.z=B7.w;}if(J1==O6){H1.y=.0;H1.x=G2.x;}else{H1.z=-H1.z;H1.xy=G2.xy;}}else{float H2=uintBitsToFloat(F0.y);float d6=B7.z;H1=f(G2.x,G2.y,H2,-2.-d6);}}f Q;if(!Eb){Q=F2(J);
#ifdef EE
Q.y=-Q.y;
#endif
#ifdef DB
Q.z=S8(z7);
#endif
}else{Q=f(q.C1,q.C1,q.C1,q.C1);}P(H1);
#ifdef CB
P(Q0);
#elif!defined(DB)
#ifdef GB
P(i1);
#elif defined(EB)
P(D);
#else
P(D);
#endif
P(j0);
#endif
#ifdef T
P(i3);
#endif
#ifdef BB
P(R0);
#endif
#ifdef FB
P(U3);
#endif
h1(Q);}
#endif
#ifdef HB
w3 x3 d i Fb(f J2,float E C5){i j;if(J2.w>=.0){j=I5(J2);if(o6)j*=E;else j.w*=E;}else if(J2.w>-1.){float t=J2.z>.0?J2.x:length(J2.xy);t=clamp(t,.0,1.);float Gb=abs(J2.z);float x=Gb>1.?(1.-1./T8)*t+(.5/T8):(1./T8)*t+Gb;float we=-J2.w;j=T1(MC,r8,c(x,we),.0);j.w*=E;if(o6)j.xyz*=j.w;}else{g d6=-J2.w-2.;j=C7(UB,B3,J2.xy,d6);g H2=J2.z*E;if(o6)j*=H2;else j=E1(Y3(j),j.w*H2);}return j;}
#ifndef DB
x2 M0(i8,H0);Y0(B5,r1);M0(bb,N3);Y0(k8,x4);y2 z2(NB){N(H1,f);
#ifdef CB
N(Q0,c);
#elif!defined(DB)
#ifdef GB
N(i1,g);
#elif defined(EB)
N(D,f);
#else
N(D,G);
#endif
N(j0,g);
#endif
#ifdef T
N(i3,G);
#endif
#ifdef BB
N(R0,f);
#endif
#ifdef FB
N(U3,g);
#endif
#if!defined(GB)||defined(CB)
h2;
#endif
g E;
#ifdef CB
E=W6(Q0,q.U4 x1);
#else
G L3=unpackHalf2x16(j1(x4));g Hb=L3.y;g F1=Hb==j0?L3.x:v1(.0);
#ifdef GB
F1+=i1;i2(x4);
#else
if(U6(D)){g r0;
#ifdef EB
if(EB&&w8(D)){r0=R6(D x1);}else
#endif
{r0=min(D.x,D.y);}F1=max(r0,F1);}else{g r0;
#if defined(EB)
if(EB&&S6(D)){r0=S4(D x1);}else
#endif
{r0=D.x;}F1+=r0;}l1(x4,packHalf2x16(Z3(F1,j0)));
#endif
#ifdef AD
if(AD){
#ifdef BD
if(BD==yd){if(F1<.0)E=.0;else if(F1<=1.)E=F1;else E=1.;}else
#endif
{E=clamp(F1,v1(.0),v1(1.));}}else
#endif
{E=abs(F1);
#ifdef IC
if(IC&&j0<.0){E=1.-v1(abs(fract(E*.5)*2.+-1.));}
#endif
E=min(E,v1(1.));}
#endif
#ifdef T
if(T&&i3.x<.0){g Z0=-i3.x;
#ifdef CD
if(CD){g A7=i3.y;if(A7!=.0){G I1=unpackHalf2x16(j1(r1));g k5=I1.y;g D7;if(k5!=Z0){D7=k5==A7?I1.x:.0;
#ifndef GB
T0(N3,E1(D7,.0,.0,.0));
#endif
}else{D7=I0(N3).x;
#ifndef GB
E2(N3);
#endif
}E=min(E,D7);}}
#endif
l1(r1,packHalf2x16(Z3(E,Z0)));E2(H0);}else
#endif
{
#ifdef T
if(T){g Z0=i3.x;if(Z0!=.0){G I1=unpackHalf2x16(j1(r1));g k5=I1.y;E=(k5==Z0)?min(I1.x,E):v1(.0);}}
#endif
#ifdef BB
if(BB){g l4=A8(I5(R0));E=clamp(l4,v1(.0),E);}
#endif
i j=Fb(H1,E Y2);i w1;
#ifdef CB
w1=I0(H0);
#else
if(Hb!=j0){w1=I0(H0);
#ifndef GB
T0(N3,w1);
#endif
}else{w1=I0(N3);
#ifndef GB
E2(N3);
#endif
}
#endif
#ifdef FB
if(FB){if(U3!=f7(B8)){j.xyz=M4(j.xyz,w1,O8(U3));}j.xyz*=j.w;}
#endif
j+=w1*(1.-j.w);T0(H0,j);i2(r1);}
#if!defined(GB)||defined(CB)
j2;
#endif
M2;}
#else
e2(i,NB){N(H1,f);
#ifdef CB
N(Q0,c);
#endif
#ifdef FB
N(U3,g);
#endif
g E=
#ifdef CB
W6(Q0,q.U4 x1);
#else
1.;
#endif
i j=Fb(H1,E Y2);
#ifdef FB
if(FB){i w1=d1(PC,c0(floor(y0.xy)));j.xyz=M4(j.xyz,w1,O8(U3));j.xyz*=j.w;}
#endif
f2(j);}
#endif
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive