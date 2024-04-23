#pragma once

#include "draw_path.exports.h"

namespace rive {
namespace pls {
namespace glsl {
const char draw_path[] = R"===(#ifdef W
T0(P)
#ifdef EB
q0(0,G3,LB);
#else
q0(0,g,VB);q0(1,g,WB);
#endif
U0
#endif
z1 k0 I(0,g,o0);
#ifndef CB
#ifdef EB
GB I(1,h,B1);
#else
k0 I(2,l0,J0);
#endif
GB I(3,h,a0);
#ifdef DB
GB I(4,h,G0);
#endif
#ifdef BB
k0 I(5,g,i0);
#endif
#endif
#ifdef AB
GB I(6,h,w2);
#endif
A1
#ifdef W
e1(PB,P,r,j,L){
#ifdef EB
w0(j,r,LB,C1);
#else
w0(j,r,VB,g);w0(j,r,WB,g);
#endif
Q(o0,g);
#ifndef Sa
#ifdef EB
Q(B1,h);
#else
Q(J0,l0);
#endif
Q(a0,h);
#ifdef DB
Q(G0,h);
#endif
#ifdef BB
Q(i0,g);
#endif
#endif
#ifdef AB
Q(w2,h);
#endif
bool z7=false;M i1;d R;
#ifdef CB
M A7;
#endif
#ifdef EB
R=E6(LB,i1,B1 i3);
#else
z7=!D6(VB,WB,L,i1,R
#ifndef CB
,J0
#else
,A7
#endif
i3);
#endif
y0 E=p2(QB,i1);
#ifndef CB
a0=E4(i1,v.r3);if((E.x&K6)!=0u)a0=-a0;
#endif
uint G1=E.x&0xfu;
#ifdef DB
uint v9=(G1==x4?E.y:E.x)>>16;G0=E4(v9,v.r3);if(G1==x4)G0=-G0;
#endif
#ifdef AB
w2=float((E.x>>4)&0xfu);
#endif
d Z3=R;
#ifdef w9
Z3.y=float(v.l9)-Z3.y;
#endif
#ifdef BB
A W0=V0(z0(IB,i1*4u+2u));g g1=z0(IB,i1*4u+3u);
#ifndef CB
i0=m4(W0,g1.xy,Z3);
#else
g7(W0,g1.xy,Z3);
#endif
#endif
if(G1==L6){i f=unpackUnorm4x8(E.y);o0=g(f);}
#ifdef DB
else if(G1==x4){h K4=E4(E.x>>16,v.r3);o0=g(K4,0,0,0);}
#endif
else{A x9=V0(z0(IB,i1*4u));g L5=z0(IB,i1*4u+1u);d n2=h0(x9,Z3)+L5.xy;if(G1==w4||G1==M6){o0.w=-uintBitsToFloat(E.y);if(L5.z>.9){o0.z=2.;}else{o0.z=L5.w;}if(G1==w4){o0.y=.0;o0.x=n2.x;}else{o0.z=-o0.z;o0.xy=n2.xy;}}else{float I2=uintBitsToFloat(E.y);o0=g(n2.x,n2.y,I2,-2.);}}g B;if(!z7){B=h2(R);
#ifdef CB
B.z=I5(A7);
#endif
}else{B=g(v.i2,v.i2,v.i2,v.i2);}S(o0);
#ifndef CB
#ifdef EB
S(B1);
#else
S(J0);
#endif
S(a0);
#ifdef DB
S(G0);
#endif
#ifdef BB
S(i0);
#endif
#endif
#ifdef AB
S(w2);
#endif
f1(B);}
#endif
#ifdef HB
E2 w1(p4,EC);w1(j3,NB);
#ifdef CB
#ifdef AB
w1(x7,FC);
#endif
#endif
F2 I3(p4,q5)k3(j3,m2)K3 N3 p i B7(g I1
#ifdef FB
,d M5,d N5
#endif
v4){if(I1.w>=.0){return M0(I1);}else if(I1.w>-1.){float t=I1.z>.0?I1.x:length(I1.xy);t=clamp(t,.0,1.);float C7=abs(I1.z);float x=C7>1.?(1.-1./J5)*t+(.5/J5):(1./J5)*t+C7;float y9=-I1.w;return M0(Q3(EC,q5,d(x,y9),.0));}else{i f;
#ifdef FB
f=P3(NB,m2,I1.xy,M5,N5);
#else
f=L2(NB,m2,I1.xy);
#endif
f.w*=I1.z;return f;}}
#ifndef CB
D1 C0(r5,X);D0(v5,x0);D0(w5,r0);C0(q7,v2);E1 Q1(JB){N(o0,g);
#ifdef EB
N(B1,h);
#else
N(J0,l0);
#endif
N(a0,h);
#ifdef DB
N(G0,h);
#endif
#ifdef BB
N(i0,g);
#endif
#ifdef AB
N(w2,h);
#endif
#ifdef FB
d M5=dFdx(o0.xy);d N5=dFdy(o0.xy);
#endif
#ifndef EB
m1;
#endif
l0 D7=unpackHalf2x16(L0(x0));h E7=D7.y;h X0=E7==a0?D7.x:E0(0);
#ifdef EB
X0+=B1;
#else
if(J0.y>=.0)X0=max(min(J0.x,J0.y),X0);else X0+=J0.x;Z0(x0,packHalf2x16(Y1(X0,a0)));
#endif
h C=abs(X0);
#ifdef QC
if(a0<.0)C=1.-abs(fract(C*.5)*2.+-1.);
#endif
C=min(C,E0(1));
#ifdef DB
if(G0<.0){h F1=-G0;
#ifdef ID
h K4=o0.x;if(K4!=.0){l0 Y0=unpackHalf2x16(L0(r0));h x3=Y0.y;h L4;if(x3!=F1){L4=x3==K4?Y0.x:.0;
#ifndef EB
F0(v2,M0(L4,0,0,0));
#endif
}else{L4=N0(v2).x;
#ifndef EB
d0(v2);
#endif
}C=min(C,L4);}
#endif
Z0(r0,packHalf2x16(Y1(C,F1)));d0(X);}else
#endif
{
#ifdef DB
if(G0!=.0){l0 Y0=unpackHalf2x16(L0(r0));h x3=Y0.y;h H2=x3==G0?Y0.x:E0(0);C=min(C,H2);}
#endif
#ifdef BB
h J2=A5(M0(i0));C=clamp(J2,E0(0),C);
#endif
d0(r0);i f=B7(o0
#ifdef FB
,M5,N5
#endif
K2);f.w*=C;i x1;if(E7!=a0){x1=N0(X);
#ifndef EB
F0(v2,x1);
#endif
}else{x1=N0(v2);
#ifndef EB
d0(v2);
#endif
}
#ifdef AB
if(w2!=E0(S6)){
#ifdef KB
f=D3(
#else
f=E3(
#endif
f,R3(x1),O0(w2));}else
#endif
{
#ifndef VC
f.xyz*=f.w;f=f+x1*(1.-f.w);
#endif
}F0(X,f);}
#ifndef EB
n1;
#endif
a2;}
#else
q2(i,JB){N(o0,g);
#ifdef AB
N(w2,h);
#endif
i f=B7(o0);
#ifdef AB
i x1=H1(FC,m0(floor(n0.xy)));
#ifdef KB
f=D3(
#else
f=E3(
#endif
f,R3(x1),O0(w2));
#else
f=o2(f);
#endif
r2(f);}
#endif
#endif
)===";
} // namespace glsl
} // namespace pls
} // namespace rive