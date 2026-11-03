#pragma once

#include "draw_path_common.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_path_common[] = R"===(#define a8 -2.
#define Ge -1.5
#define He .25
#define N9 1e3
#define Ie (N9*N9)
#ifdef CB
A4 v5(O2,Wh,WB);
#ifdef IB
P6(O2,Q7,BD);
#endif
B4 i5 o4(Pa,rd,JB);p6(sd,Fg,KB);o4(se,Ei,CD);
#if defined(BB)&&(defined(PC)||defined(QC))
c8(Vh,fl,WE);
#endif
j5
#endif
#if defined(IB)||defined(FB)
f4(Q7,kb)
#endif
#ifdef EB
X3 Z2(O2,te,AD);
#if defined(IB)||defined(FB)
P6(O2,Q7,BD);
#endif
#ifdef FB
U5(O2,ue,ID);
#endif
Z2(J5,C4,SB);
#if defined(BB)&&defined(H)&&!defined(Q)&&!defined(LB)
K4(LD);
#endif
Y3
#ifdef LB
I1 L4(B2,LC);J1
#endif
Q6(te,Z8)
#ifdef FB
f4(ue,Za)
#endif
K5 D4(x4) L5
#endif
#ifdef EB
f bool w6(e T){return T.y>=.0;}f bool w6(F T){return T.y>=.0;}
#endif
#ifdef EB
f d Fe(i Xb,uint F3){d Je=dot(Xb.xyz,e1(.30,.59,.11));if(F3==Sh) return Xb.w;if(F3==Th) return 1.-Xb.w;if(F3==Uh) return Je;return 1.-Je;}
#endif
#if defined(EB)&&defined(IB)
f bool wd(e T){return T.x<Ge;}f bool xd(e T){return T.y<Ge;}
#endif
#ifdef CB
e Ke(float Yb,c O9,float K0){c R6=(1.-O9*abs(K0))*.5;float M4,V5;if(abs(Yb-E7)<1./N9){M4=.0;V5=.0;}else{float Zb=tan(Yb);M4=sign(E7-Yb)/max(abs(Zb),1./Ie);V5=M4>=.0?R6.y-(1.-R6.x)*Zb:R6.y+R6.x*Zb;}e T;T.x=max(R6.x,.0)+He;T.y=-R6.y+a8;T.z=M4;T.w=V5;return T;}
#endif
#ifdef IB
f d d9(e T g4){d M4=T.z;d V5=max(T.w,.0);d S6=M4>=.0?N5(V5):.0;if(abs(M4)<N9){d x=abs(T.x)-He;d y=-T.y+a8;d w3=(y-V5)*0.5984134206;i t=V5+w3*A0(0.20888568955,0.62665706865,1.04442844776,1.46219982687);i u=t*-M4+(y*M4+x);i Fi=A0(N5(u[0]),N5(u[1]),N5(u[2]),N5(u[3]));i Le=t*5.09593080173+-2.54796540086;i Gi=exp2(-Le*Le);S6+=dot(Fi,Gi)*w3;}return S6*sign(T.x);}f d f5(e T g4){float S6=1.;float Hi=(1.-a8)+T.x;S6-=N5(Hi);float Ii=1.-T.y;S6-=N5(Ii);return S6;}
#endif
#ifdef CB
f f0 G4(int Me){return f0(Me&((1<<ge)-1),Me>>ge);}f float K9(uint z){return float(z)*(D7/(65536.*65536.));}f float Ce(uint z){return float(z&0xffffu)*(1./65535.);}
#endif
#if defined(CB)&&defined(SD)
f float Ne(X Q0,c Ji){c F2=G0(Q0,Ji);return(abs(F2.x)+abs(F2.y))*(1./dot(F2,F2));}f bool xa(e d8,e ac,int r,p1(uint) z3,p1(c) Ki
#ifndef BB
,p1(e) h2
#else
,p1(O) e8
#endif
T6){int P9=int(d8.x);float K0=d8.y;float bc=d8.z;int Oe=floatBitsToInt(d8.w)>>2;int U6=floatBitsToInt(d8.w)&3;int S5=min(P9,Oe-1);int d4=r*Oe+S5;R e2=j1(WB,G4(d4));uint c0=e2.w;uint K6=max(c0&Gb,1u);R T5=p0(CD,K6-1u);c T7=uintBitsToFloat(T5.xy);z3=T5.z&0xffffu;uint I9=T5.w;X Q0=w1(uintBitsToFloat(p0(JB,z3*4u)));R x2=p0(JB,z3*4u+1u);c C2=uintBitsToFloat(x2.xy);float f2=.0;float P2=.0;float Pe=0.0;if((x2.z&Rh)!=0u){R V6=p0(JB,z3*4u+2u);f2=uintBitsToFloat(V6.x);P2=uintBitsToFloat(V6.y);Pe=uintBitsToFloat(V6.z);}uint M6=c0&c3;if(M6!=0u){P9=int(ac.x);K0=ac.y;bc=ac.z;}if(P9!=S5){int J9=d4+P9-S5;R V7=j1(WB,G4(J9));if((V7.w&(c3|0xffffu))!=(c0&(c3|0xffffu))){bool W7=f2==.0||T7.x!=.0;if(W7){d4=int(I9);e2=j1(WB,G4(d4));}}else{d4=J9;e2=V7;}c0=(e2.w&~c3)|M6;}bool cc=false;float k1;
#ifdef IB
float f8;float O1;if((c0&x3)==z9&&U6==B9){uint Qe=e2.z;float N4=float(Qe&0xffffu);float G2=float(Qe>>16);f0 Q9=f0(-N4-1.,G2-N4+1.);if((c0&c3)!=0u) Q9=-Q9;R Re=j1(WB,G4(d4+Q9.x));R dc=j1(WB,G4(d4+Q9.y));if((dc.w&(c3|0xffffu))!=(Re.w&(c3|0xffffu))){dc=j1(WB,G4(int(I9)));}f8=K9(Re.z);float Se=K9(dc.z);O1=Se-f8;if(abs(O1)>z4) O1-=D7*sign(O1);float ec=G2+1.-float(le);float Te=clamp(round(abs(O1)/z4*ec),1.,ec-1.);float g8=ec-Te;if(N4<=g8){O1=-(z4*sign(O1)-O1);G2=g8;if(N4==g8) K0=-K0;}else if(N4==g8+1.){N4=.0;G2=.0;K0=.0;}else{N4-=g8+2.;G2=Te;}if(N4==G2){k1=Se;}else{k1=f8+O1*(N4/G2);}}else
#endif
{k1=K9(e2.z);}c N1=c(sin(k1),-cos(k1));c N6=uintBitsToFloat(e2.xy);c h8=c(0,0);if(P2!=.0){P2=max(P2,(zb/3.)/length(G0(Q0,N1)));}if(f2!=.0){K0*=sign(determinant(Q0));if((c0&E6)!=0u) K0=min(K0,.0);if((c0&Fb)!=0u) K0=max(K0,.0);float w5=P2!=.0?P2:Ne(Q0,N1)*Z4;d Ue=1.;if(w5>f2&&P2==.0){Ue=a5(f2)/a5(w5);f2=w5;}c O3=N1*(f2+w5);
#ifndef BB
float x=K0*(f2+w5);h2.xy=(1./(w5*2.))*(c(x,-x)+f2)+.5;h2.zw=r7(.0);
#endif
uint p5=c0&x3;if(p5>O7){bool O6=(c0&Db)!=0u;bool Qb=(c0&E6)!=0u;float g2=Ce(e2.z);float H4=sqrt(max(1.-g2*g2,.0));if(O6==Qb) H4=-H4;X Rb=X(g2,H4,-H4,g2);c q5=G0(Rb,N1);float fc=Ne(Q0,q5);float gc;if((p5==Cb)||(p5==Kh&&g2>=.25)){float Li=(c0&A9)!=0u?1.:.25;gc=f2*(1./max(g2,Li));}else{gc=f2*g2+fc*.5;}float hc=gc+fc*Z4;if((c0&Eb)!=0u){float Ve=f2+w5;float Mi=w5*.125;if(Ve<=hc*g2+Mi){float Ni=Ve*(1./g2);O3=q5*Ni;}else{c ic=q5*hc;c Oi=c(dot(O3,O3),dot(ic,ic));O3=G0(Oi,inverse(X(O3,ic)));}}c Pi=abs(K0)*O3;float We=(hc-dot(Pi,q5))/(fc*(Z4*2.));
#ifndef BB
if((c0&E6)!=0u) h2.y=We;else h2.x=We;
#endif
}
#ifndef BB
h2.xy*=Ue;h2.y=max(h2.y,1e-4);if(P2!=.0){h2.x=a8-h2.x;}
#endif
h8=G0(Q0,K0*O3);if(U6!=B9) cc=true;}else{
#ifndef BB
h2=e(bc,-1.,.0,.0);
#ifdef IB
if(P2!=.0){h2.y=a8;h2.z=Ie;h2.w=bc;if((c0&x3)==z9&&U6==B9){if(O1<.0){f8+=O1;O1=-O1;}float O4=k1-f8;O4=mod(O4+E7,D7)-E7;O4=clamp(O4,.0,O1);if(O4>O1*.5){O4=O1-O4;}c O9=c(sin(O4),cos(O4));
#if 0
float i2=1.+.33*log2(E7/(z4-min(O1,z4-z4/16.)));e Qi=Ke(O1,O9,.5*(i2/3.));float Ri=d9(Qi r1);float Si=Td(Ri);float Ti=(.5-Si)*(zb*2.);float Ui=i2/max(Ti,i2);K0*=Ui;
#endif
h2=Ke(O1,O9,K0);}h8=G0(Q0,(K0*P2)*N1);}else
#endif
{h8=sign(G0(K0*N1,inverse(Q0)))*Z4;if(U6!=Hb){h8+=N1*Pe;}}if(bool(c0&c3)!=bool(c0&Nh)){h2*=e(-1.,+1.,+1.,+1.);}
#endif
if(U6==Hb) N6=T7;if((c0&me)!=0u&&U6!=ne){cc=true;}}Ki=G0(Q0,N6)+h8+C2;
#ifdef BB
e8=Z1(x2.z>>16u);
#else
h2.xy=mix(h2.xy,c(1.,-1.),hh(j.Vi!=0u));
#endif
return!cc;}
#endif
#if defined(CB)&&defined(DB)
f c od(M W6,p1(uint) z3
#ifdef BB
,p1(O) e8
#else
,p1(d) Wi
#endif
T6){z3=floatBitsToUint(W6.z)&0xffffu;R x2=p0(JB,z3*4u+1u);
#ifdef BB
e8=Z1(x2.z>>16u);
#else
Wi=mb(floatBitsToInt(W6.z)>>16);
#endif
c X6=W6.xy;X Q0=w1(uintBitsToFloat(p0(JB,z3*4u)));c C2=uintBitsToFloat(x2.xy);X6=G0(Q0,X6)+C2;return X6;}
#endif
#if defined(CB)&&defined(FB)
f c nd(M W6,p1(uint) z3,
#ifdef BB
p1(O) e8,
#endif
p1(c) Xi T6){z3=floatBitsToUint(W6.z)&0xffffu;
#ifdef BB
e8=Z1(p0(JB,z3*4u+1u).z>>16u);
#endif
c X6=W6.xy;M i8=uintBitsToFloat(p0(JB,z3*4u+3u).xyz);Xi=(X6*i8.x+i8.yz)*j.Yi;return X6;}
#endif
f d R9(d y2,d U1,d A3){return(U1-y2)/max(1.-y2*A3,wa);}
#if defined(TB)||defined(MD)
f uint S9(R0 y3,uint Zi){uint jc=(y3.y>>G6)*(Zi<<G6)+((y3.x>>G6)<<(G6<<1));jc+=((y3.x&0x1cu)<<G6)+((y3.y&0x1cu)<<2);jc+=((y3.y&0x3u)<<2)+(y3.x&0x3u);return jc;}
#endif
#ifdef TB
#ifdef Q
#define M5 K2
#define E4(W5) S1=W5;I3
#else
#define M5 E1
#define E4(W5) y0(q0,W5);L1;
#endif
f d kc(uint aj){return mb(int((aj&Mb)-P5))*Kb;}f uint j8(d n){return uint(n*ci+.5);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive