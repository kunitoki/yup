#pragma once

#include "draw_clockwise_path.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_clockwise_path[] = R"===(#ifdef KC
#ifdef AB
U0(f0)i0(0,f,LB);i0(1,f,MB);V0
#endif
o1 L2 H(0,a0,j0);n0 H(1,f,D);n0 H(2,c,Q0);L2 H(3,N0,V2);H(4,c,c3);p1
#ifdef AB
q1(PB,f0,B,n,K){l0(n,B,LB,f);l0(n,B,MB,f);L(j0,a0);L(D,f);L(V2,N0);L(c3,c);f Q;uint R;c J;if(E6(LB,MB,K,R,J,D Y1)){M L3=w0(JB,R*4u+3u);j0=R;V2=L3.xy;c3=J+uintBitsToFloat(L3.zw);Q=F2(J);}else{Q=f(q.C1,q.C1,q.C1,q.C1);}P(j0);P(D);P(V2);P(c3);h1(Q);}
#endif
#endif
#ifdef GB
#ifdef AB
U0(f0)i0(0,a4,IB);V0
#endif
o1 L2 H(0,a0,j0);
#ifdef CB
n0 H(1,c,Q0);
#else
OB H(1,g,i1);L2 H(2,N0,V2);H(3,c,c3);
#endif
p1
#ifdef AB
q1(PB,f0,B,n,K){l0(n,B,IB,Z);
#ifdef CB
L(Q0,c);
#else
#endif
L(j0,a0);
#ifdef CB
L(Q0,c);
#else
L(i1,g);L(V2,N0);L(c3,c);
#endif
uint R;c J;
#ifdef CB
J=a8(IB,R,Q0 Y1);
#else
J=c8(IB,R,i1 Y1);M L3=w0(JB,R*4u+3u);V2=L3.xy;c3=J+uintBitsToFloat(L3.zw);
#endif
j0=Q1(R);f Q=F2(J);P(j0);
#ifdef CB
P(Q0);
#else
P(i1);P(V2);P(c3);
#endif
h1(Q);}
#endif
#endif
#ifdef HB
w3 g4(l8,W9,DC);h4(m8,X9,KB);Jd(vd,Kf,h5);x3
#ifdef OC
d void Kd(g o7,uint N1){uint cb=uint(abs(o7)*Z8+.5);uint db=q.Z2|(c6-cb);uint W2=c9(h5,N1,db);if(W2>=q.Z2){uint Ld=W2-max(W2,db);eb(h5,N1,Ld-cb);}}
#endif
d void Md(i4(float)d3,g r0,uint N1){if(min(d3,r0)>=1.){return;}g m;uint Nd=uint(abs(r0)*Z8+.5);uint W2=c9(h5,N1,q.Z2|Nd);if(W2<q.Z2){m=r0;}else{g g2=f7(W2&Y8)*a9;g e3=max(g2,r0);m=(e3-g2)/(1.-g2*d3);}d3*=m;}d void Od(i4(float)d3,g M3,uint N1){uint d9=Pd(h5,N1);if(min(d3,M3)>=1.&&(d9<q.Z2||d9>=(q.Z2|c6))){return;}g m=.0;uint e9=uint(abs(M3)*Z8+.5);if(d9<q.Z2){uint fb=q.Z2|(c6+e9);uint W2=c9(h5,N1,fb);if(W2<=q.Z2){m=M3;
#ifdef GB
m=min(m,1.);
#endif
M3=.0;}else if(W2<fb){uint gb=(W2&Y8)-c6;g g2=f7(gb)*a9;g e3=M3;
#ifdef GB
e3=min(e3,1.);
#endif
m=(e3-g2)/(1.-g2*d3);e9=gb;M3=g2;}}if(M3>.0){uint Qd=eb(h5,N1,e9);g g2=N8(int((Qd&Y8)-c6))*a9;g e3=g2+M3;g2=clamp(g2,.0,1.);e3=clamp(e3,.0,1.);g hb=1.-g2*d3;if(hb<=.0)discard;m+=(1.-m*d3)*(e3-g2)/hb;}d3*=m;}e2(i,NB){N(j0,a0);
#ifdef Lf
N(D,f);
#elif defined(CB)
N(Q0,c);
#else
N(i1,g);
#endif
#ifndef CB
N(V2,N0);N(c3,c);
#endif
i I2;uint R=j0;N0 F0=k4(DC,R);uint J1=F0.x&0xfu;if(J1<=q8){I2=unpackUnorm4x8(F0.y);}else{S D0=D1(w0(KB,R*4u));f S0=w0(KB,R*4u+1u);c G2=C0(D0,y0)+S0.xy;if(J1!=ud){float t=J1==O6?G2.x:length(G2);t=clamp(t,.0,1.);float x=t*S0.z+S0.w;float y=uintBitsToFloat(F0.y);I2=T1(MC,r8,c(x,y),.0);}else{float H2=uintBitsToFloat(F0.y);float d6=S0.z;I2=T1(UB,B3,G2,d6);I2=E1(Y3(I2),I2.w*H2);}}if(I2.w==.0){discard;}
#ifdef CB
I2.w*=W6(Q0,q.U4 x1);
#else
uint N1=V2.x;uint Rd=V2.y;N0 i5=N0(floor(c3));N1+=(i5.y>>5)*(Rd<<5)+(i5.x>>5)*(32<<5);N1+=((i5.x&0x1f)>>2)*(32<<2)+((i5.y&0x1f)>>2)*(4<<2);N1+=(i5.y&0x3)*4+(i5.x&0x3);
#ifdef OC
if(OC){
#ifdef GB
g o7=-i1;
#else
g r0;
#ifdef EB
if(EB&&S6(D)){r0=S4(D x1);}else
#endif
{r0=D.x;}g o7=max(-r0,.0);
#endif
Kd(o7,N1);discard;}
#endif
#ifndef GB
if(U6(D)){g r0;
#ifdef EB
if(EB&&w8(D)){r0=R6(D x1);}else
#endif
{r0=min(D.x,D.y);}r0=clamp(r0,.0,1.);Md(I2.w,r0,N1);}else
#endif
{
#ifdef GB
g r0=i1;
#else
g r0;
#ifdef EB
if(EB&&S6(D)){r0=S4(D x1);}else
#endif
{r0=D.x;}r0=clamp(r0,.0,1.);
#endif
Od(I2.w,r0,N1);}
#endif
f2(I2);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive