#pragma once

#include "tessellate.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char tessellate[] = R"===(#define Ve 10
#ifdef AB
U0(f0)i0(0,f,RC);i0(1,f,SC);i0(2,f,GC);
#ifdef H8
i0(3,uint,GD);i0(4,uint,HD);i0(5,uint,ID);i0(6,uint,JD);
#else
i0(3,M,RB);
#endif
V0
#endif
o1 n0 H(0,f,x5);n0 H(1,f,y5);n0 H(2,f,W3);n0 H(3,Z,J4);L2 H(4,uint,A6);p1
#ifdef AB
P2 l5(U2,Z5,JC);Q2 P3(Z5,M8)E3 O3(Va,Td,JB);O3(Wa,Ud,QC);F3 q1(VE,f0,B,n,K){l0(K,B,RC,f);l0(K,B,SC,f);l0(K,B,GC,f);
#ifdef H8
l0(K,B,GD,uint);l0(K,B,HD,uint);l0(K,B,ID,uint);l0(K,B,JD,uint);M RB=M(GD,HD,ID,JD);
#else
l0(K,B,RB,M);
#endif
L(x5,f);L(y5,f);L(W3,f);L(J4,Z);L(A6,uint);c o0=RC.xy;c p0=RC.zw;c x0=SC.xy;c z0=SC.zw;bool Ub=n<4;float y=Ub?GC.z:GC.w;int E9=int(Ub?RB.x:RB.y);
#ifdef za
int Vb=E9<<16;if(RB.z==0xffffffffu){--Vb;}float O7=float(Vb>>16);
#else
float O7=float(E9<<16>>16);
#endif
float P7=float(E9>>16);c S1=c((n&1)==0?O7:P7,(n&2)==0?y+1.:y);if((P7-O7)*q.Fa<.0){S1.y=2.*y+1.-S1.y;}uint r2=RB.z&0x3ffu;uint Wb=(RB.z>>10)&0x3ffu;uint O1=RB.z>>20;uint Y=RB.w;uint R=Y!=sd?w0(QC,Aa(Y)).z:0u;M R3=R!=0u?w0(JB,R*4u+1u):M(0u,0u,0u,0u);float k2=uintBitsToFloat(R3.z);float l2=uintBitsToFloat(R3.w);if(l2!=.0&&k2==.0){float Xb;float We=Xc(o0,p0,x0,z0,Xb);float F9=l2*(1./U8);float Xe=Sc(o0,p0,x0,z0,Xb,F9);float B6=1.-Xe*(1./O2);float Ye=dot(z0-o0,z0-o0)/(F9*F9);float Ze=(Ye-1.)*.5;B6=min(B6,Ze);B6=min(B6,.99);float af=.5*B6;float x=Z4(af)*-2.+1.;float Yb=a7(x*l2,We);f Zb=mix(o0.xyxy,z0.xyxy,f(1./3.,1./3.,2./3.,2./3.));p0=mix(p0,Zb.xy,Yb);x0=mix(x0,Zb.zw,Yb);}if((Y&od)!=0u){S ac=D1(uintBitsToFloat(w0(JB,R*4u)));c bc=C0(ac,-2.*p0+x0+o0);c cc=C0(ac,-2.*x0+z0+p0);float W0=max(dot(bc,bc),dot(cc,cc));float e4=max(ceil(sqrt(.75*4.*sqrt(W0))),1.);r2=min(uint(e4),r2);}uint Q7=r2+Wb+O1-1u;S d2=D8(o0,p0,x0,z0);float O0=acos(C8(d2[0],d2[1]));float n3=O0/float(Wb);float G9=determinant(S(x0-o0,z0-p0));if(G9==.0)G9=determinant(d2);if(G9<.0)n3=-n3;x5=f(o0,p0);y5=f(x0,z0);W3=f(float(Q7)-abs(P7-S1.x),float(Q7),(O1<<10)|r2,n3);if(O1>1u){S H9=S(d2[1],GC.xy);float bf=acos(C8(H9[0],H9[1]));float dc=float(O1);if((Y&(a3|k7))==(i7|k7)){dc-=2.;}float I9=bf/dc;if(determinant(H9)<.0)I9=-I9;J4.xy=GC.xy;J4.z=I9;}if(P7<O7){Y|=T2;}A6=Y;f Q=d7(S1,2./md,q.Fa);P(x5);P(y5);P(W3);P(J4);P(A6);h1(Q);}
#endif
#ifdef HB
R2 S2 e2(H3,WE){N(x5,f);N(y5,f);N(W3,f);N(J4,Z);N(A6,uint);c o0=x5.xy;c p0=x5.zw;c x0=y5.xy;c z0=y5.zw;S d2=D8(o0,p0,x0,z0);float cf=max(floor(W3.x),.0);float Q7=W3.y;uint ec=uint(W3.z);float r2=float(ec&0x3ffu);float O1=float(ec>>10);float n3=W3.w;uint Y=A6;float X3=Q7-O1;float X1=cf;if(X1<=X3){Y&=~a3;}else{o0=p0=x0=z0;d2=S(d2[1],J4.xy);r2=1.;X1-=X3;X3=O1;n3=J4.z;if((Y&a3)>i7){if(X1<2.5)Y|=V8;if(X1>1.5&&X1<3.5)Y|=Qa;}else if((Y&k7)!=0u||(Y&a3)==j7){X3-=2.;--X1;}Y|=n3<.0?l7:Ra;}c R7;float O0=.0;if(X1==.0||X1==X3||(Y&a3)>i7){bool x7=X1<X3*.5;R7=x7?o0:z0;O0=Ca(x7?d2[0]:d2[1]);}else if((Y&Pa)!=0u){R7=p0;}else{float c1,K4;if(r2==X3){c1=X1/r2;K4=.0;}else{c o,r,L1=p0-o0;c K5=z0-o0;c X6=x0-p0;r=X6-L1;o=-3.*X6+K5;c df=r*(r2*2.);c M5=L1*(r2*r2);float S7=.0;float ef=min(r2-1.,X1);c J9=normalize(d2[0]);float ff=-abs(n3);float gf=(1.+X1)*abs(n3);for(int z5=Ve-1;z5>=0;--z5){float C6=S7+exp2(float(z5));if(C6<=ef){c K9=C6*o+df;K9=C6*K9+M5;float hf=dot(normalize(K9),J9);float L9=C6*ff+gf;L9=min(L9,O2);if(hf>=cos(L9))S7=C6;}}float jf=S7/r2;float fc=X1-S7;float T7=acos(clamp(J9.x,-1.,1.));T7=J9.y>=.0?T7:-T7;O0=fc*n3+T7;c B2=c(sin(O0),-cos(O0));float k=dot(B2,o),U7=dot(B2,r),B0=dot(B2,L1);float kf=max(U7*U7-k*B0,.0);float V1=sqrt(kf);if(U7>.0)V1=-V1;V1-=U7;float gc=-.5*V1*k;c M9=(abs(V1*V1+gc)<abs(k*B0+gc))?c(V1,k):c(B0,V1);K4=(M9.y!=.0)?M9.x/M9.y:.0;K4=clamp(K4,.0,1.);if(fc==.0)K4=.0;c1=max(jf,K4);}c lf=c5(o0,p0,c1);c hc=c5(p0,x0,c1);c mf=c5(x0,z0,c1);c ic=c5(lf,hc,c1);c jc=c5(hc,mf,c1);R7=c5(ic,jc,c1);if(c1!=K4)O0=Ca(jc-ic);}H3 D6;D6.xy=K8(R7);if((Y&a3)==j7){D6.z=L8((uint(X3)<<16)|uint(X1));}else{D6.z=K8(mod(O0,e7));}D6.w=L8(Y);f2(D6);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive