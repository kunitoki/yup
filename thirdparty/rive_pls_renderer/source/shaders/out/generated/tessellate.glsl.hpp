#pragma once

#include "tessellate.exports.h"

namespace rive {
namespace pls {
namespace glsl {
const char tessellate[] = R"===(#define Ca 10
#ifdef Y
U0(R)v0(0,g,HC);v0(1,g,IC);v0(2,g,AC);v0(3,W,MB);V0
#endif
D1 n0 K(0,g,H3);n0 K(1,g,I3);n0 K(2,g,F2);n0 K(3,i0,j3);l6 K(4,uint,v4);E1 i B A8(d T0,d y0,d M0,d f1){B t;t[0]=(any(notEqual(T0,y0))?y0:any(notEqual(y0,M0))?M0:f1)-T0;t[1]=f1-(any(notEqual(f1,M0))?M0:any(notEqual(M0,y0))?y0:T0);return t;}
#ifdef Y
Q1 R1 Y1 W2(D7,Q9,TB);W2(E7,R9,GC);Z1 float B8(d n,d b){float Da=dot(n,b);float C8=dot(n,n)*dot(b,b);return(C8==.0)?1.:clamp(Da*inversesqrt(C8),-1.,1.);}g1(WD,R,v,k,O){z0(O,v,HC,g);z0(O,v,IC,g);z0(O,v,AC,g);z0(O,v,MB,W);S(H3,g);S(I3,g);S(F2,g);S(j3,i0);S(v4,uint);d T0=HC.xy;d y0=HC.zw;d M0=IC.xy;d f1=IC.zw;bool D8=k<4;float y=D8?AC.z:AC.w;int x6=int(D8?MB.x:MB.y);
#ifdef ha
int E8=x6<<16;if(MB.z==0xffffffffu){--E8;}float f5=float(E8>>16);
#else
float f5=float(x6<<16>>16);
#endif
float g5=float(x6>>16);d j1=d((k&1)==0?f5:g5,(k&2)==0?y+1.:y);uint x1=MB.z&0x3ffu;uint F8=(MB.z>>10)&0x3ffu;uint k3=MB.z>>20;uint E=MB.w;if(g5<f5){E|=P4;}if((g5-f5)*A.O5<.0){j1.y=2.*y+1.-j1.y;}if((E&I9)!=0u){uint Ea=C0(GC,i7(E)).z;B G8=W0(uintBitsToFloat(C0(TB,Ea*2u)));d H8=l0(G8,-2.*y0+M0+T0);d I8=l0(G8,-2.*M0+f1+y0);float e1=max(dot(H8,H8),dot(I8,I8));float J2=max(ceil(sqrt(.75*4.*sqrt(e1))),1.);x1=min(uint(J2),x1);}uint h5=x1+F8+k3-1u;B X1=A8(T0,y0,M0,f1);float r1=acos(B8(X1[0],X1[1]));float i2=r1/float(F8);float y6=determinant(B(M0-T0,f1-y0));if(y6==.0)y6=determinant(X1);if(y6<.0)i2=-i2;H3=g(T0,y0);I3=g(M0,f1);F2=g(float(h5)-abs(g5-j1.x),float(h5),(k3<<10)|x1,i2);if(k3>1u){B z6=B(X1[1],AC.xy);float Fa=acos(B8(z6[0],z6[1]));float J8=float(k3);if((E&(f4|O4))==O4){J8-=2.;}float A6=Fa/J8;if(determinant(z6)<.0)A6=-A6;j3.xy=AC.xy;j3.z=A6;}v4=E;g C;C.x=j1.x*(2./H9)-1.;C.y=j1.y*A.O5-sign(A.O5);C.zw=d(0,1);U(H3);U(I3);U(F2);U(j3);U(v4);h1(C);}
#endif
#ifdef HB
w2(W,XD){P(H3,g);P(I3,g);P(F2,g);P(j3,i0);P(v4,uint);d T0=H3.xy;d y0=H3.zw;d M0=I3.xy;d f1=I3.zw;B X1=A8(T0,y0,M0,f1);float Ga=max(floor(F2.x),.0);float h5=F2.y;uint K8=uint(F2.z);float x1=float(K8&0x3ffu);float k3=float(K8>>10);float i2=F2.w;uint E=v4;float l3=h5-k3;float y1=Ga;if(y1<=l3){E&=~f4;}else{T0=y0=M0=f1;X1=B(X1[1],j3.xy);x1=1.;y1-=l3;l3=k3;if((E&f4)!=0u){if(y1<2.5)E|=U5;if(y1>1.5&&y1<3.5)E|=y7;}else if((E&O4)!=0u){l3-=2.;y1--;}i2=j3.z;E|=i2<.0?Q4:z7;}d i5;float r1=.0;if(y1==.0||y1==l3||(E&f4)!=0u){bool X4=y1<l3*.5;i5=X4?T0:f1;r1=k7(X4?X1[0]:X1[1]);}else if((E&x7)!=0u){i5=y0;}else{float V1,m3;if(x1==l3){V1=y1/x1;m3=.0;}else{d f0,k0,a5=y0-T0;d q8=f1-T0;d L8=M0-y0;k0=L8-a5;f0=-3.*L8+q8;d Ha=k0*(x1*2.);d Ia=a5*(x1*x1);float j5=.0;float Ja=min(x1-1.,y1);d B6=normalize(X1[0]);float Ka=-abs(i2);float La=(1.+y1)*abs(i2);for(int L3=Ca-1;L3>=0;--L3){float w4=j5+exp2(float(L3));if(w4<=Ja){d C6=w4*f0+Ha;C6=w4*C6+Ia;float Ma=dot(normalize(C6),B6);float D6=w4*Ka+La;D6=min(D6,L4);if(Ma>=cos(D6))j5=w4;}}float Na=j5/x1;float M8=y1-j5;float k5=acos(clamp(B6.x,-1.,1.));k5=B6.y>=.0?k5:-k5;r1=M8*i2+k5;d a3=d(sin(r1),-cos(r1));float n=dot(a3,f0),l5=dot(a3,k0),g0=dot(a3,a5);float Oa=max(l5*l5-n*g0,.0);float G2=sqrt(Oa);if(l5>.0)G2=-G2;G2-=l5;float N8=-.5*G2*n;d E6=(abs(G2*G2+N8)<abs(n*g0+N8))?d(G2,n):d(g0,G2);m3=(E6.y!=.0)?E6.x/E6.y:.0;m3=clamp(m3,.0,1.);if(M8==.0)m3=.0;V1=max(Na,m3);}d Pa=z3(T0,y0,V1);d O8=z3(y0,M0,V1);d Qa=z3(M0,f1,V1);d P8=z3(Pa,O8,V1);d Q8=z3(O8,Qa,V1);i5=z3(P8,Q8,V1);if(V1!=m3)r1=k7(Q8-P8);}x2(W(floatBitsToUint(i0(i5,r1)),E));}
#endif
)===";
} // namespace glsl
} // namespace pls
} // namespace rive