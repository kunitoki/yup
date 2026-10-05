#pragma once

#include "draw_path_common.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_path_common[] = R"===(#define x7 -2.
#define Cd -1.5
#define Dd .25
#define Q8 1e3
#define Ed (Q8*Q8)
#ifdef BB
j4 V4(l3,Og,TB);
#ifdef HB
q6(l3,v7,YC);
#endif
k4 P4 W4(xd,mh,LB);Z5(oc,Ef,WC);a6(pc,Ff,JB);W4(yd,nh,ZC);Q4
#endif
#if defined(HB)||defined(EB)
o4(v7,wa)
#endif
#ifdef FB
N3 i3(l3,zd,ED);
#if defined(HB)||defined(EB)
q6(l3,v7,YC);
#endif
#ifdef EB
D5(l3,Ad,FD);
#endif
i3(w5,l4,CC);
#if defined(CB)&&defined(N)&&!defined(W)
E5(XD);
#endif
O3 o4(zd,ha)
#ifdef EB
o4(Ad,ma)
#endif
x5 m4(r5) y5
#endif
#ifdef FB
f bool f6(e U){return U.y>=.0;}f bool f6(C U){return U.y>=.0;}
#endif
#if defined(FB)&&defined(HB)
f bool yc(e U){return U.x<Cd;}f bool zc(e U){return U.y<Cd;}
#endif
#ifdef BB
e Fd(float Ua,c R8,float L1){c r6=(1.-R8*abs(L1))*.5;float p4,F5;if(abs(Ua-i7)<1./Q8){p4=.0;F5=.0;}else{float Va=tan(Ua);p4=sign(i7-Ua)/max(abs(Va),1./Ed);F5=p4>=.0?r6.y-(1.-r6.x)*Va:r6.y+r6.x*Va;}e U;U.x=max(r6.x,.0)+Dd;U.y=-r6.y+x7;U.z=p4;U.w=F5;return U;}
#endif
#ifdef HB
f d q8(e U R3){d p4=U.z;d F5=max(U.w,.0);d v6=p4>=.0?A5(F5):.0;if(abs(p4)<Q8){d x=abs(U.x)-Dd;d y=-U.y+x7;d h3=(y-F5)*0.5984134206;i t=F5+h3*I0(0.20888568955,0.62665706865,1.04442844776,1.46219982687);i u=t*-p4+(y*p4+x);i oh=I0(A5(u[0]),A5(u[1]),A5(u[2]),A5(u[3]));i Gd=t*5.09593080173+-2.54796540086;i ph=exp2(-Gd*Gd);v6+=dot(oh,ph)*h3;}return v6*sign(U.x);}f d M4(e U R3){float v6=1.;float qh=(1.-x7)+U.x;v6-=A5(qh);float rh=1.-U.y;v6-=A5(rh);return v6;}
#endif
#ifdef BB
f e0 q4(int Hd){return e0(Hd&((1<<jd)-1),Hd>>jd);}f float Wa(uint z){return float(z)*(G8/(65536.*65536.));}f float sh(uint z){return float(z&0xffffu)*(1./65535.);}
#endif
#if defined(BB)&&defined(MD)
f float Id(Y S0,c th){c r2=M0(S0,th);return(abs(r2.x)+abs(r2.y))*(1./dot(r2,r2));}f bool K9(e y7,e Xa,int r,i1(uint) m3,i1(c) uh
#ifndef CB
,i1(e) W1
#else
,i1(Q) z7
#endif
w6){int S3=int(y7.x);float L1=y7.y;float Ya=y7.z;int Jd=floatBitsToInt(y7.w)>>2;int A7=floatBitsToInt(y7.w)&3;int G5=min(S3,Jd-1);int T3=r*Jd+G5;M C2=p1(TB,q4(T3));uint i0=C2.w;uint x6=max(i0&Ma,1u);M H5=p0(ZC,x6-1u);c S8=uintBitsToFloat(H5.xy);m3=H5.z&0xffffu;uint T8=H5.w;Y S0=n1(uintBitsToFloat(p0(LB,m3*4u)));M U3=p0(LB,m3*4u+1u);c m2=uintBitsToFloat(U3.xy);float R2=uintBitsToFloat(U3.z);float S2=uintBitsToFloat(U3.w);uint B7=i0&Q2;if(B7!=0u){S3=int(Xa.x);L1=Xa.y;Ya=Xa.z;}if(S3!=G5){int U8=T3+S3-G5;M C7=p1(TB,q4(U8));if((C7.w&(Q2|0xffffu))!=(i0&(Q2|0xffffu))){bool vh=R2==.0||S8.x!=.0;if(vh){T3=int(T8);C2=p1(TB,q4(T3));}}else{T3=U8;C2=C7;}i0=(C2.w&~Q2)|B7;}bool Za=false;float x1;
#ifdef HB
float D7;float D1;if((i0&Q3)==M8&&A7==P8){uint Kd=C2.z;float r4=float(Kd&0xffffu);float v2=float(Kd>>16);e0 V8=e0(-r4-1.,v2-r4+1.);if((i0&Q2)!=0u) V8=-V8;M Ld=p1(TB,q4(T3+V8.x));M ab=p1(TB,q4(T3+V8.y));if((ab.w&(Q2|0xffffu))!=(Ld.w&(Q2|0xffffu))){ab=p1(TB,q4(int(T8)));}D7=Wa(Ld.z);float Md=Wa(ab.z);D1=Md-D7;if(abs(D1)>i4) D1-=G8*sign(D1);float bb=v2+1.-float(md);float Nd=clamp(round(abs(D1)/i4*bb),1.,bb-1.);float E7=bb-Nd;if(r4<=E7){D1=-(i4*sign(D1)-D1);v2=E7;if(r4==E7) L1=-L1;}else if(r4==E7+1.){r4=.0;v2=.0;L1=.0;}else{r4-=E7+2.;v2=Nd;}if(r4==v2){x1=Md;}else{x1=D7+D1*(r4/v2);}}else
#endif
{x1=Wa(C2.z);}c O2=c(sin(x1),-cos(x1));c W8=uintBitsToFloat(C2.xy);c X8=c(0,0);if(S2!=.0){S2=max(S2,(La/3.)/length(M0(S0,O2)));}if(R2!=.0){L1*=sign(determinant(S0));if((i0&O8)!=0u) L1=min(L1,.0);if((i0&sd)!=0u) L1=max(L1,.0);float X4=S2!=.0?S2:Id(S0,O2)*H4;d Od=1.;if(X4>R2&&S2==.0){Od=d4(R2)/d4(X4);R2=X4;}c I5=O2*(R2+X4);
#ifndef CB
float x=L1*(R2+X4);W1.xy=(1./(X4*2.))*(c(x,-x)+R2)+.5;W1.zw=Y6(.0);
#endif
uint cb=i0&Q3;if(cb>r7){bool Y8=(i0&qd)!=0u;bool wh=(i0&O8)!=0u;float v4=sh(C2.z);float Z8=sqrt(max(1.-v4*v4,.0));if(Y8==wh) Z8=-Z8;Y xh=Y(v4,Z8,-Z8,v4);c a9=M0(xh,O2);float db=Id(S0,a9);float eb;if((cb==Eg)||(cb==Fg&&v4>=.25)){float yh=(i0&N8)!=0u?1.:.25;eb=R2*(1./max(v4,yh));}else{eb=R2*v4+db*.5;}float fb=eb+db*H4;if((i0&rd)!=0u){float Pd=R2+X4;float zh=X4*.125;if(Pd<=fb*v4+zh){float Ah=Pd*(1./v4);I5=a9*Ah;}else{c gb=a9*fb;c Bh=c(dot(I5,I5),dot(gb,gb));I5=M0(Bh,inverse(Y(I5,gb)));}}c Ch=abs(L1)*I5;float Qd=(fb-dot(Ch,a9))/(db*(H4*2.));
#ifndef CB
if((i0&O8)!=0u) W1.y=Qd;else W1.x=Qd;
#endif
}
#ifndef CB
W1.xy*=Od;W1.y=max(W1.y,1e-4);if(S2!=.0){W1.x=x7-W1.x;}
#endif
X8=M0(S0,L1*I5);if(A7!=P8) Za=true;}else{
#ifndef CB
W1=e(Ya,-1.,.0,.0);
#ifdef HB
if(S2!=.0){W1.y=x7;W1.z=Ed;W1.w=Ya;if((i0&Q3)==M8&&A7==P8){if(D1<.0){D7+=D1;D1=-D1;}float w4=x1-D7;w4=mod(w4+i7,G8)-i7;w4=clamp(w4,.0,D1);if(w4>D1*.5){w4=D1-w4;}c R8=c(sin(w4),cos(w4));
#if 0
float X1=1.+.33*log2(i7/(i4-min(D1,i4-i4/16.)));e Dh=Fd(D1,R8,.5*(X1/3.));float Eh=q8(Dh k1);float Fh=Wc(Eh);float Gh=(.5-Fh)*(La*2.);float Hh=X1/max(Gh,X1);L1*=Hh;
#endif
W1=Fd(D1,R8,L1);}X8=M0(S0,(L1*S2)*O2);}else
#endif
{X8=sign(M0(L1*O2,inverse(S0)))*H4;}if(bool(i0&Q2)!=bool(i0&Gg)){W1*=e(-1.,+1.,+1.,+1.);}
#endif
if(A7==ud) W8=S8;if((i0&pd)!=0u&&A7!=td){Za=true;}}uh=M0(S0,W8)+X8+m2;
#ifdef CB
M V3=p0(LB,m3*4u+2u);z7=P1(V3.x);
#else
W1.xy=mix(W1.xy,c(1.,-1.),fg(j.Ih!=0u));
#endif
return!Za;}
#endif
#if defined(BB)&&defined(DB)
f c lc(O y6,i1(uint) m3
#ifdef CB
,i1(Q) z7
#else
,i1(d) Jh
#endif
w6){m3=floatBitsToUint(y6.z)&0xffffu;
#ifdef CB
M V3=p0(LB,m3*4u+2u);z7=P1(V3.x);
#else
Jh=ya(floatBitsToInt(y6.z)>>16);
#endif
c z6=y6.xy;Y S0=n1(uintBitsToFloat(p0(LB,m3*4u)));M U3=p0(LB,m3*4u+1u);c m2=uintBitsToFloat(U3.xy);z6=M0(S0,z6)+m2;return z6;}
#endif
#if defined(BB)&&defined(EB)
f c kc(O y6,i1(uint) m3,
#ifdef CB
i1(Q) z7,
#endif
i1(c) Kh w6){m3=floatBitsToUint(y6.z)&0xffffu;M V3=p0(LB,m3*4u+2u);
#ifdef CB
z7=P1(V3.x);
#endif
c z6=y6.xy;O F7=uintBitsToFloat(V3.yzw);Kh=(z6*F7.x+F7.yz)*j.Lh;return z6;}
#endif
f d c9(d i2,d M1,d n3){return(M1-i2)/max(1.-i2*n3,J9);}
#if defined(QB)||defined(ID)
f uint d9(O0 o3,uint Mh){uint hb=(o3.y>>p6)*(Mh<<p6)+((o3.x>>p6)<<(p6<<1));hb+=((o3.x&0x1cu)<<p6)+((o3.y&0x1cu)<<2);hb+=((o3.y&0x3u)<<2)+(o3.x&0x3u);return hb;}
#endif
#ifdef QB
#ifdef W
#define z5 z2
#define n4(J5) K1=J5;z3
#else
#define z5 T1
#define n4(J5) y0(n0,J5);h2;
#endif
f d ib(uint Nh){return ya(int((Nh&Ra)-C5))*Pa;}f uint G7(d n){return uint(n*Vg+.5);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive