#pragma once

#include "tessellate.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char tessellate[] = R"===(#define Sg 10
#ifdef BB
A1(c0)r0(0,g,ZC);r0(1,g,AD);r0(2,g,OC);
#ifdef O9
r0(3,uint,RD);r0(4,uint,SD);r0(5,uint,TD);r0(6,uint,UD);
#else
r0(3,Q,RB);
#endif
B1
#endif
h2 J0 d0(0,g,w6);J0 d0(1,g,x6);J0 d0(2,g,M4);J0 d0(3,V,A5);S4 d0(4,uint,I7);Z1
#ifdef BB
P3 e6(X2,d7,QC);Q3 V3(d7,T9)B4 G4(vc,Jf,LB);G4(wc,Kf,XC);C4 C1(MF,c0,G,v,S){v0(S,G,ZC,g);v0(S,G,AD,g);v0(S,G,OC,g);
#ifdef O9
v0(S,G,RD,uint);v0(S,G,SD,uint);v0(S,G,TD,uint);v0(S,G,UD,uint);Q RB=Q(RD,SD,TD,UD);
#else
v0(S,G,RB,Q);
#endif
Y(w6,g);Y(x6,g);Y(M4,g);Y(A5,V);Y(I7,uint);c w0=ZC.xy;c x0=ZC.zw;c E0=AD.xy;c I0=AD.zw;bool zd=v<4;float y=zd?OC.z:OC.w;int Ma=int(zd?RB.x:RB.y);
#ifdef Xb
int Ad=Ma<<16;if(RB.z==0xffffffffu){--Ad;}float W8=float(Ad>>16);
#else
float W8=float(Ma<<16>>16);
#endif
float X8=float(Ma>>16);c l2=c((v&1)==0?W8:X8,(v&2)==0?y+1.:y);if((X8-W8)*k.Yc<.0){l2.y=2.*y+1.-l2.y;}uint N2=RB.z&0x3ffu;uint Bd=(RB.z>>10)&0x3ffu;uint f2=RB.z>>20;uint f0=RB.w;uint F8=f0&rc;uint m0=F8>0u?P0(XC,max(F8,1u)-1u).z:0u;Q I4=m0!=0u?P0(LB,m0*4u+1u):Q(0u,0u,0u,0u);float G2=uintBitsToFloat(I4.z);float H2=uintBitsToFloat(I4.w);if(H2!=.0&&G2==.0){float Cd;float Tg=Me(w0,x0,E0,I0,Cd);float Na=H2*(1./ea);float Ug=He(w0,x0,E0,I0,Cd,Na);float J7=1.-Ug*(1./x3);float Vg=dot(I0-w0,I0-w0)/(Na*Na);float Wg=(Vg-1.)*.5;J7=min(J7,Wg);J7=min(J7,.99);float Xg=.5*J7;float x=Wb(Xg)*-2.+1.;float Dd=k8(x*H2,Tg);g Ed=mix(w0.xyxy,I0.xyxy,g(1./3.,1./3.,2./3.,2./3.));x0=mix(x0,Ed.xy,Dd);E0=mix(E0,Ed.zw,Dd);}if((f0&mf)!=0u){a0 Fd=j2(uintBitsToFloat(P0(LB,m0*4u)));c Gd=Z0(Fd,-2.*x0+E0+w0);c Hd=Z0(Fd,-2.*E0+I0+x0);float o1=max(dot(Gd,Gd),dot(Hd,Hd));float J3=max(ceil(sqrt(.75*4.*sqrt(o1))),1.);N2=min(uint(J3),N2);}uint Y8=N2+Bd+f2-1u;a0 E2=L9(w0,x0,E0,I0);float j1=acos(K9(E2[0],E2[1]));float h4=j1/float(Bd);float Oa=determinant(a0(E0-w0,I0-x0));if(Oa==.0)Oa=determinant(E2);if(Oa<.0)h4=-h4;w6=g(w0,x0);x6=g(E0,I0);M4=g(float(Y8)-abs(X8-l2.x),float(Y8),(f2<<10)|N2,h4);if(f2>1u){a0 Pa=a0(E2[1],OC.xy);float Yg=acos(K9(Pa[0],Pa[1]));float Id=float(f2);if((f0&(U3|z8))==(x8|z8)){Id-=2.;}float Qa=Yg/Id;if(determinant(Pa)<.0)Qa=-Qa;A5.xy=OC.xy;A5.z=Qa;}if(X8<W8){f0|=A3;}I7=f0;g O=o8(l2,2./jf,k.Yc);
#ifdef JC
O.y=-O.y;
#endif
l0(w6);l0(x6);l0(M4);l0(A5);l0(I7);D1(O);}
#endif
#ifdef EB
y3 z3 V2(D4,NF){B(w6,g);B(x6,g);B(M4,g);B(A5,V);B(I7,uint);c w0=w6.xy;c x0=w6.zw;c E0=x6.xy;c I0=x6.zw;a0 E2=L9(w0,x0,E0,I0);float Zg=max(floor(M4.x),.0);float Y8=M4.y;uint Jd=uint(M4.z);float N2=float(Jd&0x3ffu);float f2=float(Jd>>10);float h4=M4.w;uint f0=I7;float N4=Y8-f2;float y2=Zg;if(y2<=N4){f0&=~U3;}else{w0=x0=E0=I0;E2=a0(E2[1],A5.xy);N2=1.;y2-=N4;N4=f2;h4=A5.z;if((f0&U3)>x8){if(y2<2.5)f0|=fa;if(y2>1.5&&y2<3.5)f0|=pc;}else if((f0&z8)!=0u||(f0&U3)==y8){N4-=2.;--y2;}f0|=h4<.0?A8:qc;}c Z8;float j1=.0;if(y2==.0||y2==N4||(f0&U3)>x8){bool I8=y2<N4*.5;Z8=I8?w0:I0;j1=Zb(I8?E2[0]:E2[1]);}else if((f0&oc)!=0u){Z8=x0;}else{float w1,B5;if(N2==N4){w1=y2/N2;B5=.0;}else{c A,F,d2=x0-w0;c K6=I0-w0;c h8=E0-x0;F=h8-d2;A=-3.*h8+K6;c ah=F*(N2*2.);c M6=d2*(N2*N2);float a9=.0;float bh=min(N2-1.,y2);c Ra=normalize(E2[0]);float ch=-abs(h4);float dh=(1.+y2)*abs(h4);for(int Sa=Sg-1;Sa>=0;--Sa){float K7=a9+exp2(float(Sa));if(K7<=bh){c Ta=K7*A+ah;Ta=K7*Ta+M6;float eh=dot(normalize(Ta),Ra);float Ua=K7*ch+dh;Ua=min(Ua,x3);if(eh>=cos(Ua))a9=K7;}}float fh=a9/N2;float Kd=y2-a9;float c9=acos(clamp(Ra.x,-1.,1.));c9=Ra.y>=.0?c9:-c9;j1=Kd*h4+c9;c T2=c(sin(j1),-cos(j1));float o=dot(T2,A),d9=dot(T2,F),J1=dot(T2,d2);float gh=max(d9*d9-o*J1,.0);float q2=sqrt(gh);if(d9>.0)q2=-q2;q2-=d9;float Ld=-.5*q2*o;c Va=(abs(q2*q2+Ld)<abs(o*J1+Ld))?c(q2,o):c(J1,q2);B5=(Va.y!=.0)?Va.x/Va.y:.0;B5=clamp(B5,.0,1.);if(Kd==.0)B5=.0;w1=max(fh,B5);}c hh=X5(w0,x0,w1);c Md=X5(x0,E0,w1);c ih=X5(E0,I0,w1);c Nd=X5(hh,Md,w1);c Od=X5(Md,ih,w1);Z8=X5(Nd,Od,w1);if(w1!=B5)j1=Zb(Od-Nd);}D4 L7;L7.xy=R9(Z8);if((f0&U3)==y8){L7.z=S9((uint(N4)<<16)|uint(y2));}else{L7.z=R9(mod(j1,p8));}L7.w=S9(f0);F2(L7);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive