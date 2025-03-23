#pragma once

#include "draw_clockwise_path.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_clockwise_path[] = R"===(#ifdef JC
#ifdef AB
L0(e0)h0(0,e,LB);h0(1,e,MB);M0
#endif
o1 o0 H(0,e,q);D2 H(1,O,i0);D2 H(2,E0,M2);H(3,c,Q2);p1
#ifdef AB
Z0(PB,e0,o,l,I){k0(l,o,LB,e);k0(l,o,MB,e);P(q,e);P(i0,O);P(M2,E0);P(Q2,c);e Q;uint d0;c K;if(S5(LB,MB,I,d0,K,q g2)){D A3=r0(KB,d0*4u+3u);i0=d0;M2=A3.xy;Q2=K+uintBitsToFloat(A3.zw);Q=v2(K);}else{Q=e(m.q1,m.q1,m.q1,m.q1);}X(q);X(i0);X(M2);X(Q2);U0(Q);}
#endif
#endif
#ifdef GB
#ifdef AB
L0(e0)h0(0,R3,IB);M0
#endif
o1 OB H(0,h,a1);D2 H(1,O,i0);D2 H(2,E0,M2);H(3,c,Q2);p1
#ifdef AB
Z0(PB,e0,o,l,I){k0(l,o,IB,a0);P(a1,h);P(i0,O);P(M2,E0);P(Q2,c);uint d0;c K=g7(IB,d0,a1 g2);D A3=r0(KB,d0*4u+3u);i0=d0;M2=A3.xy;Q2=K+uintBitsToFloat(A3.zw);e Q=v2(K);X(a1);X(i0);X(M2);X(Q2);U0(Q);}
#endif
#endif
#ifdef HB
j3 W3(o7,c9,CC);X3(p7,d9,JB);pc(fc,fe,K4);k3
#ifdef LC
d void qc(h y6,uint C1){uint Z9=uint(abs(y6)*U7+.5);uint aa=m.P2|(o5-Z9);uint N2=W7(K4,C1,aa);if(N2>=m.P2){uint rc=N2-max(N2,aa);ba(K4,C1,rc-Z9);}}
#endif
d void sc(Y3(float)R2,h m0,uint C1){if(min(R2,m0)>=1.){return;}h N;uint tc=uint(abs(m0)*U7+.5);uint N2=W7(K4,C1,m.P2|tc);if(N2<m.P2){N=m0;}else{h V1=n6(N2&T7)*V7;h S2=max(V1,m0);N=(S2-V1)/(1.-V1*R2);}R2*=N;}d void uc(Y3(float)R2,h B3,uint C1){uint X7=vc(K4,C1);if(min(R2,B3)>=1.&&(X7<m.P2||X7>=(m.P2|o5))){return;}h N=.0;uint Y7=uint(abs(B3)*U7+.5);if(X7<m.P2){uint ca=m.P2|(o5+Y7);uint N2=W7(K4,C1,ca);if(N2<=m.P2){N=B3;
#ifdef GB
N=min(N,1.);
#endif
B3=.0;}else if(N2<ca){uint da=(N2&T7)-o5;h V1=n6(da)*V7;h S2=B3;
#ifdef GB
S2=min(S2,1.);
#endif
N=(S2-V1)/(1.-V1*R2);Y7=da;B3=V1;}}if(B3>.0){uint wc=ba(K4,C1,Y7);h V1=J7(int((wc&T7)-o5))*V7;h S2=V1+B3;V1=clamp(V1,.0,1.);S2=clamp(S2,.0,1.);h ea=1.-V1*R2;if(ea<=.0)discard;N+=(1.-N*R2)*(S2-V1)/ea;}R2*=N;}T1(i,NB){Z(q,e);Z(i0,O);Z(M2,E0);Z(Q2,c);i C3;uint d0=i0;E0 z0=a4(CC,d0);uint y1=z0.x&0xfu;if(y1<=w7){C3=unpackUnorm4x8(z0.y);}else{Y w0=r1(r0(JB,d0*4u));e G0=r0(JB,d0*4u+1u);c w2=q0(w0,v0)+G0.xy;if(y1!=dc){float t=y1==d6?w2.x:length(w2);t=clamp(t,.0,1.);float x=t*G0.z+G0.w;float y=uintBitsToFloat(z0.y);C3=S1(KC,x7,c(x,y),.0);}else{float G2=uintBitsToFloat(z0.y);float q5=G0.z;C3=S1(UB,q3,w2,q5);C3.w*=G2;}}if(C3.w==.0){discard;}uint C1=M2.x;uint xc=M2.y;E0 L4=E0(floor(Q2));C1+=(L4.y>>5)*(xc<<5)+(L4.x>>5)*(32<<5);C1+=((L4.x&0x1f)>>2)*(32<<2)+((L4.y&0x1f)>>2)*(4<<2);C1+=(L4.y&0x3)*4+(L4.x&0x3);
#ifdef LC
if(LC){
#ifdef GB
h y6=-a1;
#else
h m0;
#ifdef DB
if(DB&&h6(q)){m0=A4(q z1);}else
#endif
{m0=q.x;}h y6=max(-m0,.0);
#endif
qc(y6,C1);discard;}
#endif
#ifndef GB
if(j6(q)){h m0;
#ifdef DB
if(DB&&z7(q)){m0=g6(q z1);}else
#endif
{m0=min(q.x,q.y);}m0=clamp(m0,.0,1.);sc(C3.w,m0,C1);}else
#endif
{
#ifdef GB
h m0=a1;
#else
h m0;
#ifdef DB
if(DB&&h6(q)){m0=A4(q z1);}else
#endif
{m0=q.x;}m0=clamp(m0,.0,1.);
#endif
uc(C3.w,m0,C1);}U1(C3);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive