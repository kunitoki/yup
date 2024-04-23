#pragma once

#include "draw_path_common.exports.h"

namespace rive {
namespace pls {
namespace glsl {
const char draw_path_common[] = R"===(#ifdef W
N1 P2(r9,BC);O1 U1 Q2(r7,z9,TB);L3(F6,V8,QB);M3(G6,W8,IB);Q2(v7,A9,GC);V1
#ifdef JC
p m0 M4(int F7){return m0(F7&((1<<h7)-1),F7>>h7);}p float G7(A c0,d B9){d Q0=h0(c0,B9);return(abs(Q0.x)+abs(Q0.y))*(1./dot(Q0,Q0));}p bool D6(g a4,g O5,int L,x2(M)R2,x2(d)C9
#ifndef CB
,x2(l0)S2
#else
,x2(M)D9
#endif
T2){int N4=int(a4.x);float R1=a4.y;float O4=a4.z;int H7=floatBitsToInt(a4.w)>>2;int P5=floatBitsToInt(a4.w)&3;int Q5=min(N4,H7-1);int P4=L*H7+Q5;T z3=H1(BC,M4(P4));uint D=z3.w;T R5=z0(GC,X6(D));d I7=uintBitsToFloat(R5.xy);R2=O0(R5.z&0xffffu);uint E9=R5.w;A c0=V0(uintBitsToFloat(z0(TB,R2*2u)));T c4=z0(TB,R2*2u+1u);d K0=uintBitsToFloat(c4.xy);float J1=uintBitsToFloat(c4.z);
#ifdef CB
D9=O0(c4.w);
#endif
uint J7=D&H4;if(J7!=0u){N4=int(O5.x);R1=O5.y;O4=O5.z;}if(N4!=Q5){P4+=N4-Q5;T K7=H1(BC,M4(P4));if((K7.w&0xffffu)!=(D&0xffffu)){bool F9=J1==.0||I7.x!=.0;if(F9){z3=H1(BC,M4(int(E9)));}}else{z3=K7;}D=z3.w|J7;}float o1=uintBitsToFloat(z3.z);d U2=d(sin(o1),-cos(o1));d L7=uintBitsToFloat(z3.xy);d S5;if(J1!=.0){R1*=sign(determinant(c0));if((D&I4)!=0u)R1=min(R1,.0);if((D&m7)!=0u)R1=max(R1,.0);float y2=G7(c0,U2)*j2;h M7=1.;if(y2>J1){M7=E0(J1)/E0(y2);J1=y2;}d V2=h0(U2,J1+y2);
#ifndef CB
float x=R1*(J1+y2);S2=Y1((1./(y2*2.))*(d(x,-x)+J1)+.5);
#endif
uint T5=D&Y3;if(T5!=0u){int d4=2;if((D&K5)==0u)d4=-d4;if((D&H4)!=0u)d4=-d4;m0 G9=M4(P4+d4);T H9=H1(BC,G9);float I9=uintBitsToFloat(H9.z);float e4=abs(I9-o1);if(e4>D4)e4=2.*D4-e4;bool Q4=(D&K5)!=0u;bool J9=(D&I4)!=0u;float N7=e4*(Q4==J9?-.5:.5)+o1;d R4=d(sin(N7),-cos(N7));float U5=G7(c0,R4);float f4=cos(e4*.5);float V5;if((T5==p9)||(T5==q9&&f4>=.25)){float K9=(D&G4)!=0u?1.:.25;V5=J1*(1./max(f4,K9));}else{V5=J1*f4+U5*.5;}float W5=V5+U5*j2;if((D&l7)!=0u){float O7=J1+y2;float L9=y2*.125;if(O7<=W5*f4+L9){float M9=O7*(1./f4);V2=R4*M9;}else{d X5=R4*W5;d N9=d(dot(V2,V2),dot(X5,X5));V2=h0(N9,inverse(A(V2,X5)));}}d O9=abs(R1)*V2;float P7=(W5-dot(O9,R4))/(U5*(j2*2.));
#ifndef CB
if((D&I4)!=0u)S2.y=E0(P7);else S2.x=E0(P7);
#endif
}
#ifndef CB
S2*=M7;S2.y=max(S2.y,E0(1e-4));
#endif
S5=h0(c0,R1*V2);if(P5!=n7)return false;}else{if(P5==p7)L7=I7;S5=sign(h0(R1*U2,inverse(c0)))*j2;if((D&H4)!=0u)O4=-O4;
#ifndef CB
S2=Y1(O4,-1);
#endif
if((D&k7)!=0u&&P5!=o7)return false;}C9=h0(c0,L7)+S5+K0;return true;}
#endif
#ifdef EB
p d E6(C1 Y5,x2(M)R2,x2(h)P9 T2){R2=O0(floatBitsToUint(Y5.z)&0xffffu);A c0=V0(uintBitsToFloat(z0(TB,R2*2u)));T c4=z0(TB,R2*2u+1u);d K0=uintBitsToFloat(c4.xy);P9=float(floatBitsToInt(Y5.z)>>16)*sign(determinant(c0));return h0(c0,Y5.xy)+K0;}
#endif
#endif
)===";
} // namespace glsl
} // namespace pls
} // namespace rive