#pragma once

#include "draw_path.exports.h"

namespace rive {
namespace pls {
namespace glsl {
const char draw_path[] = R"===(#ifdef Y
U0(R)
#ifdef EB
v0(0,M3,LB);
#else
v0(0,g,VB);v0(1,g,WB);
#endif
V0
#endif
D1 n0 K(0,g,q0);
#ifndef DB
#ifdef EB
GB K(1,h,F1);
#else
n0 K(2,F,N0);
#endif
GB K(3,h,Z);
#ifdef CB
GB K(4,h,K0);
#endif
#ifdef AB
n0 K(5,g,m0);
#endif
#endif
#ifdef BB
GB K(6,h,z2);
#endif
E1
#ifdef Y
g1(PB,R,v,k,O){
#ifdef EB
z0(k,v,LB,i0);
#else
z0(k,v,VB,g);z0(k,v,WB,g);
#endif
S(q0,g);
#ifndef eb
#ifdef EB
S(F1,h);
#else
S(N0,F);
#endif
S(Z,h);
#ifdef CB
S(K0,h);
#endif
#ifdef AB
S(m0,g);
#endif
#endif
#ifdef BB
S(z2,h);
#endif
bool J7=false;N k1;d T;
#ifdef DB
N K7;
#endif
#ifdef EB
T=N6(LB,k1,F1 o3);
#else
J7=!M6(VB,WB,O,k1,T
#ifndef DB
,N0
#else
,K7
#endif
o3);
#endif
B0 G=v2(QB,k1);
#ifndef DB
Z=M4(k1,A.A3);if((G.x&T6)!=0u)Z=-Z;
#endif
uint J1=G.x&0xfu;
#ifdef CB
uint M9=(J1==F4?G.y:G.x)>>16;K0=M4(M9,A.A3);if(J1==F4)K0=-K0;
#endif
#ifdef BB
z2=float((G.x>>4)&0xfu);
#endif
d h4=T;
#ifdef N9
h4.y=float(A.F9)-h4.y;
#endif
#ifdef AB
B X0=W0(C0(IB,k1*4u+2u));g i1=C0(IB,k1*4u+3u);
#ifndef DB
m0=x4(X0,i1.xy,h4);
#else
q7(X0,i1.xy,h4);
#endif
#endif
if(J1==U6){j f=unpackUnorm4x8(G.y);q0=g(f);}
#ifdef CB
else if(J1==F4){h R4=M4(G.x>>16,A.A3);q0=g(R4,0,0,0);}
#endif
else{B O9=W0(C0(IB,k1*4u));g V5=C0(IB,k1*4u+1u);d q2=l0(O9,h4)+V5.xy;if(J1==E4||J1==V6){q0.w=-uintBitsToFloat(G.y);if(V5.z>.9){q0.z=2.;}else{q0.z=V5.w;}if(J1==E4){q0.y=.0;q0.x=q2.x;}else{q0.z=-q0.z;q0.xy=q2.xy;}}else{float O2=uintBitsToFloat(G.y);q0=g(q2.x,q2.y,O2,-2.);}}g C;if(!J7){C=k2(T);
#ifdef DB
C.z=S5(K7);
#endif
}else{C=g(A.l2,A.l2,A.l2,A.l2);}U(q0);
#ifndef DB
#ifdef EB
U(F1);
#else
U(N0);
#endif
U(Z);
#ifdef CB
U(K0);
#endif
#ifdef AB
U(m0);
#endif
#endif
#ifdef BB
U(z2);
#endif
h1(C);}
#endif
#ifdef HB
K2 z1(d2,A4,EC);z1(C5,p3,OB);
#ifdef DB
#ifdef BB
z1(d2,G7,FC);
#endif
#endif
L2 O3(A4,D5)q3(p3,p2)Q3 T3 i j L7(g L1
#ifdef FB
,d W5,d X5
#endif
D4){if(L1.w>=.0){return j0(L1);}else if(L1.w>-1.){float t=L1.z>.0?L1.x:length(L1.xy);t=clamp(t,.0,1.);float M7=abs(L1.z);float x=M7>1.?(1.-1./T5)*t+(.5/T5):(1./T5)*t+M7;float P9=-L1.w;return j0(W3(EC,D5,d(x,P9),.0));}else{j f;
#ifdef FB
f=V3(OB,p2,L1.xy,W5,X5);
#else
f=R2(OB,p2,L1.xy);
#endif
f.w*=L1.z;return f;}}
#ifndef DB
G1 F0(E5,e0);G0(F5,Y0);
#ifdef CB
G0(G5,A0);
#endif
F0(I7,y2);H1 T1(JB){P(q0,g);
#ifdef EB
P(F1,h);
#else
P(N0,F);
#endif
P(Z,h);
#ifdef CB
P(K0,h);
#endif
#ifdef AB
P(m0,g);
#endif
#ifdef BB
P(z2,h);
#endif
#ifdef FB
d W5=dFdx(q0.xy);d X5=dFdy(q0.xy);
#endif
#ifndef EB
p1;
#endif
F N7=unpackHalf2x16(H0(Y0));h O7=N7.y;h Z0=O7==Z?N7.x:V(0);
#ifdef EB
Z0+=F1;
#else
if(N0.y>=.0)Z0=max(min(N0.x,N0.y),Z0);else Z0+=N0.x;P0(Y0,packHalf2x16(l1(Z0,Z)));
#endif
h D=abs(Z0);
#ifdef QC
if(Z<.0)D=1.-V(abs(fract(D*.5)*2.+-1.));
#endif
D=min(D,V(1));
#ifdef CB
if(K0<.0){h I1=-K0;
#ifdef HD
h R4=q0.x;if(R4!=.0){F a1=unpackHalf2x16(H0(A0));h D3=a1.y;h S4;if(D3!=I1){S4=D3==R4?a1.x:.0;
#ifndef EB
w0(y2,j0(S4,0,0,0));
#endif
}else{S4=I0(y2).x;
#ifndef EB
n1(y2);
#endif
}D=min(D,S4);}
#endif
P0(A0,packHalf2x16(l1(D,I1)));n1(e0);}else
#endif
{
#ifdef CB
if(K0!=.0){F a1=unpackHalf2x16(H0(A0));h D3=a1.y;h N2=D3==K0?a1.x:V(0);D=min(D,N2);}A1(A0);
#endif
#ifdef AB
h P2=K5(j0(m0));D=clamp(P2,V(0),D);
#endif
j f=L7(q0
#ifdef FB
,W5,X5
#endif
Q2);f.w*=D;j B1;if(O7!=Z){B1=I0(e0);
#ifndef EB
w0(y2,B1);
#endif
}else{B1=I0(y2);
#ifndef EB
n1(y2);
#endif
}
#ifdef BB
if(z2!=V(c7)){
#ifdef KB
f=J3(
#else
f=K3(
#endif
f,X3(B1),J0(z2));}else
#endif
{f.xyz*=f.w;f=f+B1*(1.-f.w);}w0(e0,f);}
#ifndef EB
q1;
#endif
f2;}
#else
w2(j,JB){P(q0,g);
#ifdef BB
P(z2,h);
#endif
j f=L7(q0);
#ifdef BB
j B1=K1(FC,o0(floor(p0.xy)));
#ifdef KB
f=J3(
#else
f=K3(
#endif
f,X3(B1),J0(z2));
#else
f=r2(f);
#endif
x2(f);}
#endif
#endif
)===";
} // namespace glsl
} // namespace pls
} // namespace rive