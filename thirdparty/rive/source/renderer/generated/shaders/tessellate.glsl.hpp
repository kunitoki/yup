#pragma once

#include "tessellate.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char tessellate[] = R"===(#define Uj 10
#ifdef CB
h1(d0) K(0,e,QD);K(1,e,RD);K(2,e,WC);
#ifdef hb
K(3,uint,NE);K(4,uint,OE);K(5,uint,PE);K(6,uint,QE);
#else
K(3,R,XB);
#endif
i1
#endif
o2 I0 W(0,e,g7);I0 W(1,e,h7);I0 W(2,e,z5);I0 W(3,M,A5);l3 W(4,uint,C8);P1
#ifdef CB
A4 P6(O2,Q7,BD);B4 f4(Q7,kb) i5 o4(Pa,rd,JB);o4(se,Ei,CD);j5 n1(OG,d0,A,C,r){L(r,A,QD,e);L(r,A,RD,e);L(r,A,WC,e);
#ifdef hb
L(r,A,NE,uint);L(r,A,OE,uint);L(r,A,PE,uint);L(r,A,QE,uint);R XB=R(NE,OE,PE,QE);
#else
L(r,A,XB,R);
#endif
V(g7,e);V(h7,e);V(z5,e);V(A5,M);V(C8,uint);c D0=QD.xy;c H0=QD.zw;c M0=RD.xy;c V0=RD.zw;bool Bf=C<4;float y=Bf?WC.z:WC.w;int Nc=int(Bf?XB.x:XB.y);
#ifdef lb
int Cf=Nc<<16;if(XB.z==0xffffffffu){--Cf;}float ha=float(Cf>>16);
#else
float ha=float(Nc<<16>>16);
#endif
float ia=float(Nc>>16);c I2=c((C&1)==0?ha:ia,(C&2)==0?y+1.:y);if((ia-ha)*j.Ze<.0){I2.y=2.*y+1.-I2.y;}uint k3=XB.z&0x3ffu;uint Df=(XB.z>>10)&0x3ffu;uint G2=XB.z>>20;uint c0=XB.w;uint K6=c0&Gb;uint g0=K6>0u?p0(CD,max(K6,1u)-1u).z:0u;R V6=g0!=0u?p0(JB,g0*4u+2u):R(0u,0u,0u,0u);float f2=uintBitsToFloat(V6.x);float P2=uintBitsToFloat(V6.y);if(P2!=.0&&f2==.0){float Ef;float Vj=Yg(D0,H0,M0,V0,Ef);float Oc=P2*(1./zb);float Wj=Tg(D0,H0,M0,V0,Ef,Oc);float D8=1.-Wj*(1./z4);float Xj=dot(V0-D0,V0-D0)/(Oc*Oc);float Yj=(Xj-1.)*.5;D8=min(D8,Yj);D8=min(D8,.99);float Zj=.5*D8;float x=Td(Zj)*-2.+1.;float Ff=l9(x*P2,Vj);e Gf=mix(D0.xyxy,V0.xyxy,e(1./3.,1./3.,2./3.,2./3.));H0=mix(H0,Gf.xy,Ff);M0=mix(M0,Gf.zw,Ff);}if((c0&Jh)!=0u){X ba=w1(uintBitsToFloat(p0(JB,g0*4u)));c Hf=G0(ba,-2.*H0+M0+D0);c If=G0(ba,-2.*M0+V0+H0);float F1=max(dot(Hf,Hf),dot(If,If));float m4=max(ceil(sqrt(.75*4.*sqrt(F1))),1.);k3=min(uint(m4),k3);}uint ja=k3+Df+G2-1u;X E2=eb(D0,H0,M0,V0);float k1=acos(h9(E2[0],E2[1]));float V4=k1/float(Df);float Pc=determinant(X(M0-D0,V0-H0));if(Pc==.0) Pc=determinant(E2);if(Pc<.0) V4=-V4;g7=e(D0,H0);h7=e(M0,V0);z5=e(float(ja)-abs(ia-I2.x),float(ja),(G2<<10)|k3,V4);A5.xy=WC.xy;if(G2>1u){X Qc=X(E2[1],WC.xy);float ak=acos(h9(Qc[0],Qc[1]));float Jf=float(G2);if((c0&(x3|A9))==(O7|A9)){Jf-=2.;}float Rc=ak/Jf;if(determinant(Qc)<.0) Rc=-Rc;A5.z=Rc;}if(ia<ha){c0|=c3;}C8=c0;e I=p9(I2,2./wh,j.Ze);
#ifdef OC
I.y=-I.y;
#endif
e0(g7);e0(h7);e0(z5);e0(A5);e0(C8);C1(I);}
#endif
#ifdef EB
X3 Y3 v2(R,PG){q(g7,e);q(h7,e);q(z5,e);q(A5,M);q(C8,uint);c D0=g7.xy;c H0=g7.zw;c M0=h7.xy;c V0=h7.zw;X E2=eb(D0,H0,M0,V0);float F4=max(floor(z5.x),.0);float ja=z5.y;uint Kf=uint(z5.z);float k3=float(Kf&0x3ffu);float G2=float(Kf>>10);float V4=z5.w;uint c0=C8;float B3=ja-G2;float A1=F4;if(A1<=B3){c0&=~x3;}else{D0=H0=M0=V0;E2=X(E2[1],A5.xy);k3=1.;A1-=B3;B3=G2;V4=A5.z;bool Lf=(c0&A9)!=0u;if(Lf||(c0&x3)==z9){B3-=2.;--A1;}bool bk=Lf&&(A1==0.||A1==B3);if(bk){c0&=~x3;}else if((c0&x3)!=Mh){c0|=V4<.0?E6:Fb;}if((c0&x3)>O7){float ka=B3*.5;if(A1<ka) c0|=Db;if(B3>3.&&A1>ka-1.&&A1<ka+1.) c0|=Eb;A1=A1<ka?.0:B3;}}c h6;float k1=.0;if(A1==.0||A1==B3){bool O6=A1<B3*.5;h6=O6?D0:V0;k1=nb(O6?E2[0]:E2[1]);}else if((c0&me)!=0u){h6=D0;if(A1>=float(y9/2u)) h6=H0;if(A1>=float(y9*3u/4u)) h6=M0;if(A1>=float(y9*7u/8u)) h6=A5.xy;}else{float M1,i6;if(k3==B3){M1=A1/k3;i6=.0;}else{c E,J,D2=H0-D0;c v7=V0-D0;c i9=M0-H0;J=i9-D2;E=-3.*i9+v7;c ck=J*(k3*2.);c x7=D2*(k3*k3);float la=.0;float dk=min(k3-1.,A1);c Sc=normalize(E2[0]);float ek=-abs(V4);float fk=(1.+A1)*abs(V4);for(int Tc=Uj-1;Tc>=0;--Tc){float E8=la+exp2(float(Tc));if(E8<=dk){c Uc=E8*E+ck;Uc=E8*Uc+x7;float gk=dot(normalize(Uc),Sc);float Vc=E8*ek+fk;Vc=min(Vc,z4);if(gk>=cos(Vc)) la=E8;}}float hk=la/k3;float Mf=A1-la;float ma=acos(clamp(Sc.x,-1.,1.));ma=Sc.y>=.0?ma:-ma;k1=Mf*V4+ma;c N1=c(sin(k1),-cos(k1));float l=dot(N1,E),na=dot(N1,J),Y1=dot(N1,D2);float ik=max(na*na-l*Y1,.0);float N2=sqrt(ik);if(na>.0) N2=-N2;N2-=na;float Nf=-.5*N2*l;c Wc=(abs(N2*N2+Nf)<abs(l*Y1+Nf))?c(N2,l):c(Y1,N2);i6=(Wc.y!=.0)?Wc.x/Wc.y:.0;i6=clamp(i6,.0,1.);if(Mf==.0) i6=.0;M1=max(hk,i6);}c jk=A6(D0,H0,M1);c Of=A6(H0,M0,M1);c kk=A6(M0,V0,M1);c Pf=A6(jk,Of,M1);c Qf=A6(Of,kk,M1);h6=A6(Pf,Qf,M1);if(M1!=i6) k1=nb(Qf-Pf);}R F8;F8.xy=floatBitsToUint(h6);if((c0&x3)==z9){F8.z=(uint(B3)<<16)|uint(A1);}else{uint lk=uint(int(round(k1*(65536./D7))))&0xffffu;uint Rf=0u;if((c0&x3)>O7){float mk=clamp(h9(E2[0],E2[1]),-1.,1.);Rf=uint(round(sqrt((1.+mk)*.5)*65535.));}F8.z=(lk<<16)|Rf;}F8.w=c0;w2(F8);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive