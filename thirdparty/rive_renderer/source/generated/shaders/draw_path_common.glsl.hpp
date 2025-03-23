#pragma once

#include "draw_path_common.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_path_common[] = R"===(#define v5 -2.
#define ja -1.5
#define ka .25
#define D6 1e3
#define la (D6*D6)
#ifdef AB
H2 F3(K2,ec,EC);
#if defined(DB)
G3(K2,x6,NC);
#endif
I2 H3(x6,Z7)r3 I3(S9,Dc,KB);W3(o7,c9,CC);X3(p7,d9,JB);I3(T9,Ec,OC);v3
#endif
#ifdef HB
J2 o2(K2,Q9,KC);
#if defined(DB)||defined(EB)
G3(K2,x6,NC);
#endif
#ifdef EB
G3(w6,R9,KD);
#endif
o2(w6,I4,UB);
#if defined(CB)&&defined(FB)
o2(K2,V9,MC);
#endif
L2 H3(Q9,x7)
#if(defined(DB)||defined(EB))&&!defined(AB)
H3(x6,Z7)
#endif
#ifdef EB
H3(R9,Fc)
#endif
h4(I4,q3)
#endif
#ifdef HB
d bool j6(e M){return M.y>=.0;}d bool j6(C M){return M.y>=.0;}
#endif
#if defined(DB)||defined(EB)
#define J3(N) S1(NC,Z7,c(N,.0),.0).x
#define w5(N) S1(NC,Z7,c(N,1.),.0).x
#endif
#if defined(HB)&&defined(DB)
d bool z7(e M){return M.x<ja;}d bool h6(e M){return M.y<ja;}
#endif
#ifdef AB
e ma(float J1,c E6,float w1){c N4=(1.-E6*abs(w1))*.5;float T2,j4;if(abs(J1-I1/2.)<1./D6){T2=.0;j4=.0;}else{float a8=tan(J1);T2=sign(I1/2.-J1)/max(abs(a8),1./la);j4=T2>=.0?N4.y-(1.-N4.x)*a8:N4.y+N4.x*a8;}e M;M.x=max(N4.x,.0)+ka;M.y=-N4.y+v5;M.z=T2;M.w=j4;return M;}
#endif
#ifdef DB
d float A4(e M x5){h T2=M.z;h j4=max(M.w,.0);h O4=T2>=.0?J3(j4):.0;if(abs(T2)<D6){h x=abs(M.x)-ka;h y=-M.y+v5;h na=(y-j4)*0.5984134206;i t=j4+na*f2(0.20888568955,0.62665706865,1.04442844776,1.46219982687);i u=t*-T2+(y*T2+x);i Gc=f2(J3(u[0]),J3(u[1]),J3(u[2]),J3(u[3]));i oa=t*5.09593080173+-2.54796540086;i Hc=exp2(-oa*oa);O4+=dot(Gc,Hc)*na;}return O4*sign(M.x);}d h g6(e M x5){float O4=1.;float Ic=(1.-v5)+M.x;O4-=J3(Ic);float Jc=1.-M.y;O4-=J3(Jc);return O4;}
#endif
#if defined(HB)&&defined(EB)
d h C7(c c8,c f5 x5){c d8=round(c8);i M=F6(KD,Fc,d8,f5);M=f2(w5(M.x),w5(M.y),w5(M.z),w5(M.w));M.xw=mix(M.xw,M.yz,d1(c8.x+.5-d8.x));M.x=mix(M.w,M.x,d1(c8.y+.5-d8.y));return J3(M.x);}
#endif
#if defined(AB)&&defined(JC)
d f0 y5(int pa){return f0(pa&((1<<H9)-1),pa>>H9);}d float qa(Y w0,c Kc){c B1=q0(w0,Kc);return(abs(B1.x)+abs(B1.y))*(1./dot(B1,B1));}d bool S5(e z5,e e8,int I,G1(uint)q2,G1(c)Lc
#ifndef CB
,G1(e)Y0
#else
,G1(O)A5
#endif
P4){int G6=int(z5.x);float w1=z5.y;float f8=z5.z;int ra=floatBitsToInt(z5.w)>>2;int H6=floatBitsToInt(z5.w)&3;int g8=min(G6,ra-1);int B5=I*ra+g8;D K3=f1(EC,y5(B5));uint T=K3.w;D h8=r0(OC,v9(T));c sa=uintBitsToFloat(h8.xy);q2=h8.z&0xffffu;uint Mc=h8.w;Y w0=r1(uintBitsToFloat(r0(KB,q2*4u)));D C5=r0(KB,q2*4u+1u);c G0=uintBitsToFloat(C5.xy);float y2=uintBitsToFloat(C5.z);float U2=uintBitsToFloat(C5.w);uint ta=T&y3;if(ta!=0u){G6=int(e8.x);w1=e8.y;f8=e8.z;}if(G6!=g8){B5+=G6-g8;D ua=f1(EC,y5(B5));if((ua.w&(y3|0xffffu))!=(T&(y3|0xffffu))){bool Nc=y2==.0||sa.x!=.0;if(Nc){K3=f1(EC,y5(int(Mc)));}}else{K3=ua;}T=(K3.w&~y3)|ta;}float g1=uintBitsToFloat(K3.z);c V2=c(sin(g1),-cos(g1));c i8=uintBitsToFloat(K3.xy);c I6;if(U2!=.0){U2=max(U2,(I9/3.)/length(q0(w0,V2)));}if(y2!=.0){w1*=sign(determinant(w0));if((T&v6)!=0u)w1=min(w1,.0);if((T&N9)!=0u)w1=max(w1,.0);float L3=U2!=.0?U2:qa(w0,V2)*g3;h va=1.;if(L3>y2&&U2==.0){va=T3(y2)/T3(L3);y2=L3;}c k4=q0(V2,y2+L3);
#ifndef CB
float x=w1*(y2+L3);Y0.xy=(1./(L3*2.))*(c(x,-x)+y2)+.5;Y0.zw=Mb(.0);
#endif
uint j8=T&x3;if(j8>q6){int D5=2;if((T&R7)==0u)D5=-D5;if((T&y3)!=0u)D5=-D5;f0 Oc=y5(B5+D5);D Pc=f1(EC,Oc);float Qc=uintBitsToFloat(Pc.z);float E5=abs(Qc-g1);if(E5>I1)E5=2.*I1-E5;bool J6=(T&R7)!=0u;bool Rc=(T&v6)!=0u;float wa=E5*(J6==Rc?-.5:.5)+g1;c K6=c(sin(wa),-cos(wa));float k8=qa(w0,K6);float F5=cos(E5*.5);float l8;if((j8==Zb)||(j8==ac&&F5>=.25)){float Sc=(T&r6)!=0u?1.:.25;l8=y2*(1./max(F5,Sc));}else{l8=y2*F5+k8*.5;}float m8=l8+k8*g3;if((T&M9)!=0u){float xa=y2+L3;float Tc=L3*.125;if(xa<=m8*F5+Tc){float Uc=xa*(1./F5);k4=K6*Uc;}else{c n8=K6*m8;c Vc=c(dot(k4,k4),dot(n8,n8));k4=q0(Vc,inverse(Y(k4,n8)));}}c Wc=abs(w1)*k4;float ya=(m8-dot(Wc,K6))/(k8*(g3*2.));
#ifndef CB
if((T&v6)!=0u)Y0.y=ya;else Y0.x=ya;
#endif
}
#ifndef CB
Y0.xy*=va;Y0.y=max(Y0.y,1e-4);if(U2!=.0){Y0.x=v5-Y0.x;}
#endif
I6=q0(w0,w1*k4);if(H6!=S7)return false;}else{
#ifndef CB
Y0=e(f8,-1.,.0,.0);
#ifdef DB
if(U2!=.0){Y0.y=v5;Y0.z=la;Y0.w=f8;if((T&x3)==P7){int o8=int(K3.x);if((T&y3)!=0u)o8=-o8;D za=f1(EC,y5(B5+o8));i8=uintBitsToFloat(za.xy);if(H6==S7){float Aa=uintBitsToFloat(za.z);float J1=uintBitsToFloat(K3.y);if(J1<.0){Aa+=J1;J1=-J1;}float W2=g1-Aa;W2=mod(W2+I1/2.,2.*I1)-I1/2.;W2=clamp(W2,.0,J1);if(W2>J1*.5){W2=J1-W2;}c E6=c(sin(W2),cos(W2));
#if 0
float i1=1.+.33*log2((I1/2.)/(I1-min(J1,I1-I1/16.)));e Xc=ma(J1,E6,.5*(i1/3.));float Yc=A4(Xc z1);float Zc=w5(Yc);float ad=(.5-Zc)*(I9*2.);float bd=i1/max(ad,i1);w1*=bd;
#endif
Y0=ma(J1,E6,w1);}if((T&Q7)!=0u){w1=.0;}}I6=q0(w0,(w1*U2)*V2);}else
#endif
{I6=sign(q0(w1*V2,inverse(w0)))*g3;}if(bool(T&y3)!=bool(T&bc)){Y0.x=-Y0.x;}
#endif
if(H6==P9)i8=sa;if((T&L9)!=0u&&H6!=O9){return false;}}Lc=q0(w0,i8)+I6+G0;
#ifdef CB
D z3=r0(KB,q2*4u+2u);A5=M1(z3.x);
#else
Y0.xy=mix(Y0.xy,c(1.,-1.),Nb(m.Ub!=0u));
#endif
return true;}
#endif
#if defined(AB)&&defined(GB)
d c g7(a0 Q4,G1(uint)q2
#ifdef CB
,G1(O)A5
#else
,G1(h)cd
#endif
P4){q2=floatBitsToUint(Q4.z)&0xffffu;
#ifdef CB
D z3=r0(KB,q2*4u+2u);A5=M1(z3.x);
#else
cd=J7(floatBitsToInt(Q4.z)>>16);
#endif
c R4=Q4.xy;Y w0=r1(uintBitsToFloat(r0(KB,q2*4u)));D C5=r0(KB,q2*4u+1u);c G0=uintBitsToFloat(C5.xy);R4=q0(w0,R4)+G0;return R4;}
#endif
#if defined(AB)&&defined(EB)
d c Y8(a0 Q4,G1(uint)q2,
#ifdef CB
G1(O)A5,
#endif
G1(c)dd P4){q2=floatBitsToUint(Q4.z)&0xffffu;D z3=r0(KB,q2*4u+2u);
#ifdef CB
A5=M1(z3.x);
#endif
c R4=Q4.xy;a0 p5=uintBitsToFloat(z3.yzw);dd=R4*p5.x+p5.yz;return R4;}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive