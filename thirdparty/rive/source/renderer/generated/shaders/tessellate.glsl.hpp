#pragma once

#include "tessellate.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char tessellate[] = R"===(#define Ai 10
#ifdef BB
c1(d0) K(0,e,KD);K(1,e,LD);K(2,e,TC);
#ifdef ta
K(3,uint,HE);K(4,uint,IE);K(5,uint,JE);K(6,uint,KE);
#else
K(3,M,VB);
#endif
d1
#endif
l2 E0 V(0,e,L6);E0 V(1,e,M6);E0 V(2,e,Z4);E0 V(3,O,a5);Z2 V(4,uint,S7);e2
#ifdef BB
j4 q6(l3,v7,YC);k4 o4(v7,wa) P4 W4(xd,mh,LB);W4(yd,nh,ZC);Q4 v1(GG,d0,D,G,r){L(r,D,KD,e);L(r,D,LD,e);L(r,D,TC,e);
#ifdef ta
L(r,D,HE,uint);L(r,D,IE,uint);L(r,D,JE,uint);L(r,D,KE,uint);M VB=M(HE,IE,JE,KE);
#else
L(r,D,VB,M);
#endif
T(L6,e);T(M6,e);T(Z4,e);T(a5,O);T(S7,uint);c z0=KD.xy;c C0=KD.zw;c J0=LD.xy;c P0=LD.zw;bool ue=G<4;float y=ue?TC.z:TC.w;int Fb=int(ue?VB.x:VB.y);
#ifdef xa
int ve=Fb<<16;if(VB.z==0xffffffffu){--ve;}float r9=float(ve>>16);
#else
float r9=float(Fb<<16>>16);
#endif
float v9=float(Fb>>16);c y2=c((G&1)==0?r9:v9,(G&2)==0?y+1.:y);if((v9-r9)*j.Td<.0){y2.y=2.*y+1.-y2.y;}uint Y2=VB.z&0x3ffu;uint we=(VB.z>>10)&0x3ffu;uint v2=VB.z>>20;uint i0=VB.w;uint x6=i0&Ma;uint a0=x6>0u?p0(ZC,max(x6,1u)-1u).z:0u;M U3=a0!=0u?p0(LB,a0*4u+1u):M(0u,0u,0u,0u);float R2=uintBitsToFloat(U3.z);float S2=uintBitsToFloat(U3.w);if(S2!=.0&&R2==.0){float xe;float Bi=Wf(z0,C0,J0,P0,xe);float Gb=S2*(1./La);float Ci=Rf(z0,C0,J0,P0,xe,Gb);float T7=1.-Ci*(1./i4);float Di=dot(P0-z0,P0-z0)/(Gb*Gb);float Ei=(Di-1.)*.5;T7=min(T7,Ei);T7=min(T7,.99);float Fi=.5*T7;float x=Wc(Fi)*-2.+1.;float ye=B8(x*S2,Bi);e ze=mix(z0.xyxy,P0.xyxy,e(1./3.,1./3.,2./3.,2./3.));C0=mix(C0,ze.xy,ye);J0=mix(J0,ze.zw,ye);}if((i0&Dg)!=0u){Y k9=n1(uintBitsToFloat(p0(LB,a0*4u)));c Ae=M0(k9,-2.*C0+J0+z0);c Be=M0(k9,-2.*J0+P0+C0);float y1=max(dot(Ae,Ae),dot(Be,Be));float e4=max(ceil(sqrt(.75*4.*sqrt(y1))),1.);Y2=min(uint(e4),Y2);}uint w9=Y2+we+v2-1u;Y q2=qa(z0,C0,J0,P0);float x1=acos(x8(q2[0],q2[1]));float D4=x1/float(we);float Hb=determinant(Y(J0-z0,P0-C0));if(Hb==.0) Hb=determinant(q2);if(Hb<.0) D4=-D4;L6=e(z0,C0);M6=e(J0,P0);Z4=e(float(w9)-abs(v9-y2.x),float(w9),(v2<<10)|Y2,D4);a5.xy=TC.xy;if(v2>1u){Y Ib=Y(q2[1],TC.xy);float Gi=acos(x8(Ib[0],Ib[1]));float Ce=float(v2);if((i0&(Q3|N8))==(r7|N8)){Ce-=2.;}float Jb=Gi/Ce;if(determinant(Ib)<.0) Jb=-Jb;a5.z=Jb;}if(v9<r9){i0|=Q2;}S7=i0;e I=F8(y2,2./ug,j.Td);
#ifdef MC
I.y=-I.y;
#endif
Z(L6);Z(M6);Z(Z4);Z(a5);Z(S7);w1(I);}
#endif
#ifdef FB
N3 O3 j3(M,HG){q(L6,e);q(M6,e);q(Z4,e);q(a5,O);q(S7,uint);c z0=L6.xy;c C0=L6.zw;c J0=M6.xy;c P0=M6.zw;Y q2=qa(z0,C0,J0,P0);float Hi=max(floor(Z4.x),.0);float w9=Z4.y;uint De=uint(Z4.z);float Y2=float(De&0x3ffu);float v2=float(De>>10);float D4=Z4.w;uint i0=S7;float c5=w9-v2;float c2=Hi;if(c2<=c5){i0&=~Q3;}else{z0=C0=J0=P0;q2=Y(q2[1],a5.xy);Y2=1.;c2-=c5;c5=v2;D4=a5.z;if((i0&Q3)>r7){if(c2<2.5) i0|=qd;if(c2>1.5&&c2<3.5) i0|=rd;}else if((i0&N8)!=0u||(i0&Q3)==M8){c5-=2.;--c2;}i0|=D4<.0?O8:sd;}c T5;float x1=.0;if(c2==.0||c2==c5||(i0&Q3)>r7){bool Y8=c2<c5*.5;T5=Y8?z0:P0;x1=Yc(Y8?q2[0]:q2[1]);}else if((i0&pd)!=0u){T5=z0;if(c2>=float(L8/2u)) T5=C0;if(c2>=float(L8*3u/4u)) T5=J0;if(c2>=float(L8*7u/8u)) T5=a5.xy;}else{float C1,U5;if(Y2==c5){C1=c2/Y2;U5=.0;}else{c B,J,p2=C0-z0;c Z6=P0-z0;c y8=J0-C0;J=y8-p2;B=-3.*y8+Z6;c Ii=J*(Y2*2.);c c7=p2*(Y2*Y2);float x9=.0;float Ji=min(Y2-1.,c2);c Kb=normalize(q2[0]);float Ki=-abs(D4);float Li=(1.+c2)*abs(D4);for(int Lb=Ai-1;Lb>=0;--Lb){float U7=x9+exp2(float(Lb));if(U7<=Ji){c Mb=U7*B+Ii;Mb=U7*Mb+c7;float Mi=dot(normalize(Mb),Kb);float Nb=U7*Ki+Li;Nb=min(Nb,i4);if(Mi>=cos(Nb)) x9=U7;}}float Ni=x9/Y2;float Ee=c2-x9;float y9=acos(clamp(Kb.x,-1.,1.));y9=Kb.y>=.0?y9:-y9;x1=Ee*D4+y9;c O2=c(sin(x1),-cos(x1));float k=dot(O2,B),z9=dot(O2,J),O1=dot(O2,p2);float Oi=max(z9*z9-k*O1,.0);float B2=sqrt(Oi);if(z9>.0) B2=-B2;B2-=z9;float Fe=-.5*B2*k;c Ob=(abs(B2*B2+Fe)<abs(k*O1+Fe))?c(B2,k):c(O1,B2);U5=(Ob.y!=.0)?Ob.x/Ob.y:.0;U5=clamp(U5,.0,1.);if(Ee==.0) U5=.0;C1=max(Ni,U5);}c Pi=k6(z0,C0,C1);c Ge=k6(C0,J0,C1);c Qi=k6(J0,P0,C1);c He=k6(Pi,Ge,C1);c Ie=k6(Ge,Qi,C1);T5=k6(He,Ie,C1);if(C1!=U5) x1=Yc(Ie-He);}M V7;V7.xy=floatBitsToUint(T5);if((i0&Q3)==M8){V7.z=(uint(c5)<<16)|uint(c2);}else{uint Ri=uint(int(round(x1*(65536./G8))))&0xffffu;uint Je=0u;if((i0&Q3)>r7){float Si=clamp(x8(q2[0],q2[1]),-1.,1.);Je=uint(round(sqrt((1.+Si)*.5)*65535.));}V7.z=(Ri<<16)|Je;}V7.w=i0;P2(V7);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive