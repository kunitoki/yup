#pragma once

#include "draw_depthstencil_path.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_depthstencil_path[] = R"===(f bool S7(){
#ifdef PC
if(PC) return true;
#endif
#ifdef QC
if(QC) return true;
#endif
return false;}
#ifdef CB
h1(d0) i1
#endif
o2 I0 W(0,e,N0);
#ifdef H
NB W(1,d,S0);
#endif
#ifdef GB
I0 W(2,M,O0);
#endif
P1
#ifdef CB
n1(UB,d0,A,c4,H6){V(N0,e);
#ifdef H
V(S0,d);
#endif
#ifdef GB
V(O0,M);
#endif
#ifdef PC
const bool I6=PC;
#else
const bool I6=false;
#endif
#ifdef QC
const bool xe=QC;
#else
const bool xe=false;
#endif
int F4=c4;bool C9=(F4&ke)!=0;bool D9=(F4&Fh)!=0;int J6;
#if defined(PC)||defined(QC)
if(S7()){J6=ie(D9);int ye=F4&je;uint wi=uint(ye>>J6);int xi=ye&((1<<J6)-1);F4=int(Q5(WE,wi))+xi;}else
#endif
{J6=he(C9);}bool yi=(F4&Eh)!=0;bool zi=(F4&Gh)!=0;int ze=F4&je;int Ai=ze>>J6;int E9=ze&((1<<J6)-1);float K0=.0;float n=1.;bool F9=false;int R5;int G9;if(S7()){int Ae=Ch(D9);int H9=E9&((1<<Ae)-1);if(I6&&!D9){++H9;}K0=H9<2?1.:-1.;n=(H9==0||H9==3)?.0:1.;R5=E9>>Ae;G9=int(xb);}else{F9=!C9&&E9==Dh;R5=F9?0:E9;G9=C9?int(xh):int(xb);}int S5=min(R5,G9-1);int d4=Ai*G9+S5;R e2=j1(WB,G4(d4));uint c0=e2.w;uint K6=max(c0&Gb,1u);R T5=p0(CD,K6-1u);c T7=uintBitsToFloat(T5.xy);uint g0=T5.z&0xffffu;uint I9=T5.w;X Q0=w1(uintBitsToFloat(p0(JB,g0*4u)));R x2=p0(JB,g0*4u+1u);c C2=uintBitsToFloat(x2.xy);O L6=Z1(x2.z>>16u);float f2=.0;if(I6){f2=uintBitsToFloat(p0(JB,g0*4u+2u).x);}uint M6=c0&c3;bool U7;if(I6){U7=false;}else if(xe){U7=M6!=0u;if(U7) K0=-K0;}else{U7=M6!=0u&&!C9&&!F9;}if(U7) R5=R5-1;if(R5!=S5){int J9=d4+R5-S5;R V7=j1(WB,G4(J9));if((V7.w&(c3|0xffffu))!=(c0&(c3|0xffffu))){bool W7;if(I6) W7=T7.x!=.0;else W7=true;if(W7){e2=j1(WB,G4(int(I9)));}}else{e2=V7;}c0=(e2.w&~c3)|M6;}c i0;if(S7()){K0*=sign(determinant(Q0));float k1=K9(e2.z);c N1=c(sin(k1),-cos(k1));c N6=uintBitsToFloat(e2.xy);c O3=N1;c Be=N1;float Pb=(n==.0)?K0:.0;if(I6){if((c0&E6)!=0u) K0=min(K0,.0);if((c0&Fb)!=0u) K0=max(K0,.0);uint p5=c0&x3;if(p5>O7){bool O6=(c0&Db)!=0u;bool Qb=(c0&E6)!=0u;float g2=Ce(e2.z);float H4=sqrt(max(1.-g2*g2,.0));if(O6==Qb) H4=-H4;X Rb=X(g2,H4,-H4,g2);c q5=G0(Rb,N1);bool L9=p5==Lh||(p5!=Cb&&g2<.25);bool De=(c0&Eb)!=0u;if(p5==Cb){O3=N1+q5;}else if(De||!L9){float t=L9?g2:1./g2;O3=q5*t;}if(L9||De) Be=q5;if(!D9&&L9) Pb=.5*K0;}}i0=G0(Q0,N6+O3*(K0*f2))+C2;if(Pb!=.0){i0+=sign(G0(Be,inverse(Q0)))*Pb;}}else{c N6=F9?T7:uintBitsToFloat(e2.xy);i0=G0(Q0,N6)+C2;}c n0=i0;
#ifdef VD
if(j.Ja!=0u){n0.y=float(j.Ka)-n0.y;}
#endif
#ifdef AB
if(AB){X M3=w1(p0(KB,g0*q2+2u));e Z3=p0(KB,g0*q2+3u);vb(M3,Z3.xy,n0 I4);}
#endif
if(yi){N0=e(.0,.0,.0,.0);
#ifdef GB
O0=M(0.0,0.0,0.0);
#endif
}else{uint A2=x2.z&0xfu;bool J2=false;
#ifdef H
if(H){uint c2=(x2.z>>4)&0xfu;S0=float(c2);J2=c2!=W3;}
#endif
if(A2==Ta){N0=e(unpackUnorm4x8(x2.w));if(J2){N0.w*=n;}else{N0*=n;}}else{X Sb=w1(p0(KB,g0*q2));e X7=p0(KB,g0*q2+1u);float w4=uintBitsToFloat(x2.w);N0=La(n0,Sb,X7.xy,X7.zw,A2,w4,n);}
#ifdef GB
if(GB&&(x2.z&oe)!=0u){X Tb=w1(p0(KB,g0*q2+4u));e Y7=p0(KB,g0*q2+5u);c y3=G0(Tb,n0)+Y7.xy;float Z7=1.+Y7.z;if((x2.z&pe)!=0u){uint F3=(x2.z&re)>>qe;Z7=-(1.+float(F3));}O0=M(y3.x,y3.y,Z7);}else{O0=M(0.0,0.0,0.0);}
#endif
}e I=T3(i0);
#ifdef OC
I.y=-I.y;
#endif
uint M9;if(S7()){M9=uint(n*254.);if(!zi)++M9;}else{M9=0xffu;}I.z=v9(L6,M9);e0(N0);
#ifdef H
e0(S0);
#endif
#ifdef GB
e0(O0);
#endif
C1(I);}
#endif
#ifdef EB
#ifdef LB
E1(HB)
#else
v2(i,HB)
#endif
{q(N0,e);
#ifdef GB
q(O0,M);
#endif
#ifdef H
q(S0,d);
#endif
#ifdef H
O c2=a3(S0);bool J2=H&&c2!=W3;
#else
const bool J2=false;
#endif
i k;if(N0.w>=.0){k=y4(N0);}else{c Bi=cb(N0,j.X8,j.Y8);k=r2(AD,Z8,Bi,.0);if(S7()){k.w*=Ci(N0);}if(!J2){k.xyz*=k.w;k.w*=Ee(N0);}}
#ifdef GB
if(GB&&O0.z>0.0){d Ub=O0.z-1.;i T1=k5(SB,x4,O0.xy,Ub);if(J2) T1=A0(o6(T1),T1.w);k*=T1;}
#endif
#if defined(H)&&!defined(Q)
#ifdef LB
i X0=e4(LC);
#else
i X0=J4(LD);
#endif
k.xyz=Y4(k.xyz,X0,c2);k.xyz*=k.w;
#elif defined(LB)
i X0=e4(LC);
#endif
k.xyz=M2(k.xyz,k.w,Y.xy,j.J3,j.K3);
#ifdef GB
if(GB&&O0.z<.0){i Di=k5(SB,x4,O0.xy,B0(.0));uint Vb=uint(-O0.z-1.);d Wb=Fe(Di,Vb);k=A0(mix(B0(1.),Wb,a5(N0.w)));}
#endif
#ifdef LB
if(any(notEqual(k,A0(.0)))){r5(LC,Va(k,X0));}
#endif
#ifdef LB
L1
#else
w2(k);
#endif
}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive