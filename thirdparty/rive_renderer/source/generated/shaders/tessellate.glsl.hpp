#pragma once

#include "tessellate.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char tessellate[] = R"===(#define yd 10
#ifdef AB
L0(e0)h0(0,e,PC);h0(1,e,QC);h0(2,e,GC);
#ifdef G7
h0(3,uint,BD);h0(4,uint,CD);h0(5,uint,DD);h0(6,uint,ED);
#else
h0(3,D,RB);
#endif
M0
#endif
o1 o0 H(0,e,U4);o0 H(1,e,V4);o0 H(2,e,N3);o0 H(3,a0,o4);D2 H(4,uint,O5);p1 d Y Ka(c E1,c T0,c m1,c L1){Y t;t[0]=(any(notEqual(E1,T0))?T0:any(notEqual(T0,m1))?m1:L1)-E1;t[1]=L1-(any(notEqual(L1,m1))?m1:any(notEqual(m1,T0))?T0:E1);return t;}
#ifdef AB
H2 I2 r3 I3(S9,Dc,KB);I3(T9,Ec,OC);v3 d float La(c k,c b){float zd=dot(k,b);float Ma=dot(k,k)*dot(b,b);return(Ma==.0)?1.:clamp(zd*inversesqrt(Ma),-1.,1.);}Z0(SE,e0,o,l,I){k0(I,o,PC,e);k0(I,o,QC,e);k0(I,o,GC,e);
#ifdef G7
k0(I,o,BD,uint);k0(I,o,CD,uint);k0(I,o,DD,uint);k0(I,o,ED,uint);D RB=D(BD,CD,DD,ED);
#else
k0(I,o,RB,D);
#endif
P(U4,e);P(V4,e);P(N3,e);P(o4,a0);P(O5,uint);c E1=PC.xy;c T0=PC.zw;c m1=QC.xy;c L1=QC.zw;bool Na=l<4;float y=Na?GC.z:GC.w;int G8=int(Na?RB.x:RB.y);
#ifdef r9
int Oa=G8<<16;if(RB.z==0xffffffffu){--Oa;}float T6=float(Oa>>16);
#else
float T6=float(G8<<16>>16);
#endif
float U6=float(G8>>16);c j2=c((l&1)==0?T6:U6,(l&2)==0?y+1.:y);uint e2=RB.z&0x3ffu;uint Pa=(RB.z>>10)&0x3ffu;uint d3=RB.z>>20;uint T=RB.w;if(U6<T6){T|=y3;}if((U6-T6)*m.B9<.0){j2.y=2.*y+1.-j2.y;}if((T&Yb)!=0u){uint d0=r0(OC,v9(T)).z;Y Qa=r1(uintBitsToFloat(r0(KB,d0*4u)));c Ra=q0(Qa,-2.*T0+m1+E1);c Sa=q0(Qa,-2.*m1+L1+T0);float O0=max(dot(Ra,Ra),dot(Sa,Sa));float U3=max(ceil(sqrt(.75*4.*sqrt(O0))),1.);e2=min(uint(U3),e2);}uint V6=e2+Pa+d3-1u;Y n1=Ka(E1,T0,m1,L1);float g1=acos(La(n1[0],n1[1]));float C2=g1/float(Pa);float H8=determinant(Y(m1-E1,L1-T0));if(H8==.0)H8=determinant(n1);if(H8<.0)C2=-C2;U4=e(E1,T0);V4=e(m1,L1);N3=e(float(V6)-abs(U6-j2.x),float(V6),(d3<<10)|e2,C2);if(d3>1u){Y I8=Y(n1[1],GC.xy);float Ad=acos(La(I8[0],I8[1]));float Ta=float(d3);if((T&(x3|r6))==(q6|r6)){Ta-=2.;}float J8=Ad/Ta;if(determinant(I8)<.0)J8=-J8;o4.xy=GC.xy;o4.z=J8;}O5=T;e Q=m6(j2,2./Xb,m.B9);X(U4);X(V4);X(N3);X(o4);X(O5);U0(Q);}
#endif
#ifdef HB
J2 L2 T1(D,TE){Z(U4,e);Z(V4,e);Z(N3,e);Z(o4,a0);Z(O5,uint);c E1=U4.xy;c T0=U4.zw;c m1=V4.xy;c L1=V4.zw;Y n1=Ka(E1,T0,m1,L1);float Bd=max(floor(N3.x),.0);float V6=N3.y;uint Ua=uint(N3.z);float e2=float(Ua&0x3ffu);float d3=float(Ua>>10);float C2=N3.w;uint T=O5;float r2=V6-d3;float J0=Bd;float p4=.0;int Va;if(J0<=r2){T&=~x3;}else{E1=T0=m1=L1;n1=Y(n1[1],o4.xy);e2=1.;J0-=r2;r2=d3;C2=o4.z;if((T&x3)>q6){if(J0<2.5)T|=R7;if(J0>1.5&&J0<3.5)T|=M9;}else if((T&r6)!=0u){r2-=2.;--J0;}else if((T&x3)==P7){Va=-int(J0);--J0;p4=C2*d3;float Cd=d3-1.;float K8=Cd-1.-3.;float Wa=clamp(round(abs(p4)/I1*K8),1.,K8-1.);float P5=K8-Wa;if(J0<=P5){n1[1]=-n1[1];p4=-(I1*sign(p4)-p4);r2=P5;}else if(J0==P5+1.){n1[0]=n1[1]=-n1[1];J0=.0;r2=1.;T|=Q7;}else if(J0==P5+2.){n1[1]=n1[0];J0=.0;r2=1.;T|=Q7;}else{J0-=P5+3.;r2=Wa;}C2=p4/r2;}T|=C2<.0?v6:N9;}c W6;float g1=.0;if(J0==.0||J0==r2||(T&x3)>q6){bool J6=J0<r2*.5;W6=J6?E1:L1;g1=x9(J6?n1[0]:n1[1]);}else if((T&L9)!=0u){W6=T0;}else{float A2,q4;if(e2==r2){A2=J0/e2;q4=.0;}else{c G,L,M6=T0-E1;c Ba=L1-E1;c Xa=m1-T0;L=Xa-M6;G=-3.*Xa+Ba;c Dd=L*(e2*2.);c Ed=M6*(e2*e2);float X6=.0;float Fd=min(e2-1.,J0);c L8=normalize(n1[0]);float Gd=-abs(C2);float Hd=(1.+J0)*abs(C2);for(int P3=yd-1;P3>=0;--P3){float Q5=X6+exp2(float(P3));if(Q5<=Fd){c M8=Q5*G+Dd;M8=Q5*M8+Ed;float Id=dot(normalize(M8),L8);float N8=Q5*Gd+Hd;N8=min(N8,I1);if(Id>=cos(N8))X6=Q5;}}float Jd=X6/e2;float Ya=J0-X6;float Y6=acos(clamp(L8.x,-1.,1.));Y6=L8.y>=.0?Y6:-Y6;g1=Ya*C2+Y6;c V2=c(sin(g1),-cos(g1));float k=dot(V2,G),Z6=dot(V2,L),D0=dot(V2,M6);float Kd=max(Z6*Z6-k*D0,.0);float O3=sqrt(Kd);if(Z6>.0)O3=-O3;O3-=Z6;float Za=-.5*O3*k;c O8=(abs(O3*O3+Za)<abs(k*D0+Za))?c(O3,k):c(D0,O3);q4=(O8.y!=.0)?O8.x/O8.y:.0;q4=clamp(q4,.0,1.);if(Ya==.0)q4=.0;A2=max(Jd,q4);}c Ld=E4(E1,T0,A2);c ab=E4(T0,m1,A2);c Md=E4(m1,L1,A2);c bb=E4(Ld,ab,A2);c cb=E4(ab,Md,A2);W6=E4(bb,cb,A2);if(A2!=q4)g1=x9(cb-bb);}D P8=D(floatBitsToUint(a0(W6,g1)),T);if((T&x3)==P7){P8.x=uint(Va);P8.y=floatBitsToUint(p4);}U1(P8);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive