#pragma once

#include "draw_path_common.exports.h"

namespace rive {
namespace pls {
namespace glsl {
const char draw_path_common[] = R"===(#ifdef Y
Q1 V2(d2,L9,BC);R1 Y1 W2(D7,Q9,TB);R3(O6,n9,QB);S3(P6,o9,IB);W2(E7,R9,GC);Z1
#ifdef JC
i o0 T4(int P7){return o0(P7&((1<<r7)-1),P7>>r7);}i float Q7(B d0,d S9){d R0=l0(d0,S9);return(abs(R0.x)+abs(R0.y))*(1./dot(R0,R0));}i bool M6(g i4,g Y5,int O,A2(N)X2,A2(d)T9
#ifndef DB
,A2(F)Y2
#else
,A2(N)U9
#endif
Z2){int U4=int(i4.x);float U1=i4.y;float V4=i4.z;int R7=floatBitsToInt(i4.w)>>2;int Z5=floatBitsToInt(i4.w)&3;int a6=min(U4,R7-1);int W4=O*R7+a6;W F3=K1(BC,T4(W4));uint E=F3.w;W c6=C0(GC,i7(E));d S7=uintBitsToFloat(c6.xy);X2=J0(c6.z&0xffffu);uint V9=c6.w;B d0=W0(uintBitsToFloat(C0(TB,X2*2u)));W j4=C0(TB,X2*2u+1u);d O0=uintBitsToFloat(j4.xy);float M1=uintBitsToFloat(j4.z);
#ifdef DB
U9=J0(j4.w);
#endif
uint T7=E&P4;if(T7!=0u){U4=int(Y5.x);U1=Y5.y;V4=Y5.z;}if(U4!=a6){W4+=U4-a6;W U7=K1(BC,T4(W4));if((U7.w&0xffffu)!=(E&0xffffu)){bool W9=M1==.0||S7.x!=.0;if(W9){F3=K1(BC,T4(int(V9)));}}else{F3=U7;}E=F3.w|T7;}float r1=uintBitsToFloat(F3.z);d a3=d(sin(r1),-cos(r1));d V7=uintBitsToFloat(F3.xy);d d6;if(M1!=.0){U1*=sign(determinant(d0));if((E&Q4)!=0u)U1=min(U1,.0);if((E&z7)!=0u)U1=max(U1,.0);float B2=Q7(d0,a3)*m2;h W7=1.;if(B2>M1){W7=V(M1)/V(B2);M1=B2;}d c3=l0(a3,M1+B2);
#ifndef DB
float x=U1*(M1+B2);Y2=l1((1./(B2*2.))*(d(x,-x)+M1)+.5);
#endif
uint e6=E&f4;if(e6!=0u){int k4=2;if((E&U5)==0u)k4=-k4;if((E&P4)!=0u)k4=-k4;o0 X9=T4(W4+k4);W Y9=K1(BC,X9);float Z9=uintBitsToFloat(Y9.z);float l4=abs(Z9-r1);if(l4>L4)l4=2.*L4-l4;bool X4=(E&U5)!=0u;bool aa=(E&Q4)!=0u;float X7=l4*(X4==aa?-.5:.5)+r1;d Y4=d(sin(X7),-cos(X7));float f6=Q7(d0,Y4);float m4=cos(l4*.5);float g6;if((e6==J9)||(e6==K9&&m4>=.25)){float ba=(E&O4)!=0u?1.:.25;g6=M1*(1./max(m4,ba));}else{g6=M1*m4+f6*.5;}float h6=g6+f6*m2;if((E&y7)!=0u){float Y7=M1+B2;float ca=B2*.125;if(Y7<=h6*m4+ca){float da=Y7*(1./m4);c3=Y4*da;}else{d i6=Y4*h6;d ea=d(dot(c3,c3),dot(i6,i6));c3=l0(ea,inverse(B(c3,i6)));}}d fa=abs(U1)*c3;float Z7=(h6-dot(fa,Y4))/(f6*(m2*2.));
#ifndef DB
if((E&Q4)!=0u)Y2.y=V(Z7);else Y2.x=V(Z7);
#endif
}
#ifndef DB
Y2*=W7;Y2.y=max(Y2.y,V(1e-4));
#endif
d6=l0(d0,U1*c3);if(Z5!=A7)return false;}else{if(Z5==C7)V7=S7;d6=sign(l0(U1*a3,inverse(d0)))*m2;if((E&P4)!=0u)V4=-V4;
#ifndef DB
Y2=l1(V4,-1);
#endif
if((E&x7)!=0u&&Z5!=B7)return false;}T9=l0(d0,V7)+d6+O0;return true;}
#endif
#ifdef EB
i d N6(i0 j6,A2(N)X2,A2(h)ga Z2){X2=J0(floatBitsToUint(j6.z)&0xffffu);B d0=W0(uintBitsToFloat(C0(TB,X2*2u)));W j4=C0(TB,X2*2u+1u);d O0=uintBitsToFloat(j4.xy);ga=V(floatBitsToInt(j6.z)>>16)*sign(determinant(d0));return l0(d0,j6.xy)+O0;}
#endif
#endif
)===";
} // namespace glsl
} // namespace pls
} // namespace rive