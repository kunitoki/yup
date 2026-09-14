#pragma once

#include "tessellate.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char tessellate[] = R"===(#define ah 10
#ifdef CB
A1(a0)p0(0,g,ZC);p0(1,g,AD);p0(2,g,NC);
#ifdef O9
p0(3,uint,VD);p0(4,uint,WD);p0(5,uint,XD);p0(6,uint,YD);
#else
p0(3,Q,RB);
#endif
B1
#endif
h2 J0 c0(0,g,w6);J0 c0(1,g,x6);J0 c0(2,g,L4);J0 c0(3,V,C5);R4 c0(4,uint,I7);a2
#ifdef CB
R3 f6(a3,d7,QC);S3 X3(d7,T9)A4 F4(Ac,Rf,MB);F4(Bc,Sf,XC);B4 C1(PF,a0,G,v,T){q0(T,G,ZC,g);q0(T,G,AD,g);q0(T,G,NC,g);
#ifdef O9
q0(T,G,VD,uint);q0(T,G,WD,uint);q0(T,G,XD,uint);q0(T,G,YD,uint);Q RB=Q(VD,WD,XD,YD);
#else
q0(T,G,RB,Q);
#endif
Y(w6,g);Y(x6,g);Y(L4,g);Y(C5,V);Y(I7,uint);c w0=ZC.xy;c x0=ZC.zw;c E0=AD.xy;c I0=AD.zw;bool Ed=v<4;float y=Ed?NC.z:NC.w;int Ra=int(Ed?RB.x:RB.y);
#ifdef cc
int Fd=Ra<<16;if(RB.z==0xffffffffu){--Fd;}float W8=float(Fd>>16);
#else
float W8=float(Ra<<16>>16);
#endif
float X8=float(Ra>>16);c l2=c((v&1)==0?W8:X8,(v&2)==0?y+1.:y);if((X8-W8)*k.dd<.0){l2.y=2.*y+1.-l2.y;}uint O2=RB.z&0x3ffu;uint Gd=(RB.z>>10)&0x3ffu;uint f2=RB.z>>20;uint e0=RB.w;uint F8=e0&wc;uint l0=F8>0u?P0(XC,max(F8,1u)-1u).z:0u;Q H4=l0!=0u?P0(MB,l0*4u+1u):Q(0u,0u,0u,0u);float H2=uintBitsToFloat(H4.z);float I2=uintBitsToFloat(H4.w);if(I2!=.0&&H2==.0){float Hd;float bh=Se(w0,x0,E0,I0,Hd);float Sa=I2*(1./ea);float ch=Ne(w0,x0,E0,I0,Hd,Sa);float J7=1.-ch*(1./A3);float dh=dot(I0-w0,I0-w0)/(Sa*Sa);float eh=(dh-1.)*.5;J7=min(J7,eh);J7=min(J7,.99);float fh=.5*J7;float x=bc(fh)*-2.+1.;float Id=k8(x*I2,bh);g Jd=mix(w0.xyxy,I0.xyxy,g(1./3.,1./3.,2./3.,2./3.));x0=mix(x0,Jd.xy,Id);E0=mix(E0,Jd.zw,Id);}if((e0&rf)!=0u){Z Kd=j2(uintBitsToFloat(P0(MB,l0*4u)));c Ld=Z0(Kd,-2.*x0+E0+w0);c Md=Z0(Kd,-2.*E0+I0+x0);float o1=max(dot(Ld,Ld),dot(Md,Md));float M3=max(ceil(sqrt(.75*4.*sqrt(o1))),1.);O2=min(uint(M3),O2);}uint Y8=O2+Gd+f2-1u;Z F2=L9(w0,x0,E0,I0);float h1=acos(K9(F2[0],F2[1]));float i4=h1/float(Gd);float Ta=determinant(Z(E0-w0,I0-x0));if(Ta==.0)Ta=determinant(F2);if(Ta<.0)i4=-i4;w6=g(w0,x0);x6=g(E0,I0);L4=g(float(Y8)-abs(X8-l2.x),float(Y8),(f2<<10)|O2,i4);if(f2>1u){Z Ua=Z(F2[1],NC.xy);float gh=acos(K9(Ua[0],Ua[1]));float Nd=float(f2);if((e0&(W3|z8))==(x8|z8)){Nd-=2.;}float Va=gh/Nd;if(determinant(Ua)<.0)Va=-Va;C5.xy=NC.xy;C5.z=Va;}if(X8<W8){e0|=D3;}I7=e0;g N=o8(l2,2./of,k.dd);
#ifdef JC
N.y=-N.y;
#endif
k0(w6);k0(x6);k0(L4);k0(C5);k0(I7);D1(N);}
#endif
#ifdef FB
B3 C3 Y2(C4,QF){B(w6,g);B(x6,g);B(L4,g);B(C5,V);B(I7,uint);c w0=w6.xy;c x0=w6.zw;c E0=x6.xy;c I0=x6.zw;Z F2=L9(w0,x0,E0,I0);float hh=max(floor(L4.x),.0);float Y8=L4.y;uint Od=uint(L4.z);float O2=float(Od&0x3ffu);float f2=float(Od>>10);float i4=L4.w;uint e0=I7;float M4=Y8-f2;float y2=hh;if(y2<=M4){e0&=~W3;}else{w0=x0=E0=I0;F2=Z(F2[1],C5.xy);O2=1.;y2-=M4;M4=f2;i4=C5.z;if((e0&W3)>x8){if(y2<2.5)e0|=fa;if(y2>1.5&&y2<3.5)e0|=uc;}else if((e0&z8)!=0u||(e0&W3)==y8){M4-=2.;--y2;}e0|=i4<.0?A8:vc;}c Z8;float h1=.0;if(y2==.0||y2==M4||(e0&W3)>x8){bool I8=y2<M4*.5;Z8=I8?w0:I0;h1=ec(I8?F2[0]:F2[1]);}else if((e0&tc)!=0u){Z8=x0;}else{float w1,D5;if(O2==M4){w1=y2/O2;D5=.0;}else{c A,F,d2=x0-w0;c K6=I0-w0;c h8=E0-x0;F=h8-d2;A=-3.*h8+K6;c ih=F*(O2*2.);c M6=d2*(O2*O2);float a9=.0;float jh=min(O2-1.,y2);c Wa=normalize(F2[0]);float kh=-abs(i4);float lh=(1.+y2)*abs(i4);for(int Xa=ah-1;Xa>=0;--Xa){float K7=a9+exp2(float(Xa));if(K7<=jh){c Ya=K7*A+ih;Ya=K7*Ya+M6;float mh=dot(normalize(Ya),Wa);float Za=K7*kh+lh;Za=min(Za,A3);if(mh>=cos(Za))a9=K7;}}float nh=a9/O2;float Pd=y2-a9;float c9=acos(clamp(Wa.x,-1.,1.));c9=Wa.y>=.0?c9:-c9;h1=Pd*i4+c9;c W2=c(sin(h1),-cos(h1));float o=dot(W2,A),d9=dot(W2,F),I1=dot(W2,d2);float oh=max(d9*d9-o*I1,.0);float q2=sqrt(oh);if(d9>.0)q2=-q2;q2-=d9;float Qd=-.5*q2*o;c ab=(abs(q2*q2+Qd)<abs(o*I1+Qd))?c(q2,o):c(I1,q2);D5=(ab.y!=.0)?ab.x/ab.y:.0;D5=clamp(D5,.0,1.);if(Pd==.0)D5=.0;w1=max(nh,D5);}c ph=X5(w0,x0,w1);c Rd=X5(x0,E0,w1);c qh=X5(E0,I0,w1);c Sd=X5(ph,Rd,w1);c Td=X5(Rd,qh,w1);Z8=X5(Sd,Td,w1);if(w1!=D5)h1=ec(Td-Sd);}C4 L7;L7.xy=R9(Z8);if((e0&W3)==y8){L7.z=S9((uint(M4)<<16)|uint(y2));}else{L7.z=R9(mod(h1,p8));}L7.w=S9(e0);G2(L7);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive