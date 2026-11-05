#pragma once

#include "draw_clockwise_atomic_path.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_clockwise_atomic_path_frag[] = R"===(#ifdef EB
I1
#ifndef Q
z0(B2,q0);
#endif
z0(o3,o0);
#ifndef Q
rc(F6,j7);
#endif
J1 n4 pc(Ib,Vf,d1);p4 f d qk(float A3,d F0,uint n2,p1(uint) G1,p1(d) k4){
#ifdef Q
if(min(A3,F0)>=1.){return 1.;}
#endif
d G;uint Yf=j8(abs(F0));G1=o8(d1,n2,j.z2|Yf);if(G1<j.z2){G=F0;
#ifndef Q
k4=F0;
#endif
}else{
#ifndef Q
if((G1&R7)!=0u){G1=o8(d1,n2,j.z2|R7|Yf);}
#endif
d y2=z6(G1&Mb)*Kb;d U1=max(y2,F0);G=R9(y2,U1,A3);
#ifndef Q
k4=U1;
#endif
}return G;}f d rk(float A3,d D5,uint n2,p1(uint) G1,p1(d) k4){d G=.0;uint ad=j8(abs(D5));G1=Q5(d1,n2);
#ifdef Q
if(min(A3,D5)>=1.&&(G1<j.z2||G1>=(j.z2|P5))){return 1.;}
#endif
if(G1<j.z2){uint Zf=j.z2|(P5+ad);uint D3=o8(d1,n2,Zf);
#ifndef Q
G1=D3;
#endif
if(D3<=j.z2){G=D5;
#ifdef DB
G=min(G,1.);
#endif
#ifndef Q
k4=G;
#endif
D5=.0;}else if(D3<Zf){uint ag=(D3&Mb)-P5;d y2=z6(ag)*Kb;d U1=D5;
#ifdef DB
U1=min(U1,1.);
#endif
#ifndef Q
k4=U1;
#endif
G=R9(y2,U1,A3);ad=ag;D5=y2;}}if(D5>.0){uint Zc=qc(d1,n2,ad);d y2=kc(Zc);d U1=y2+D5;y2=clamp(y2,.0,1.);U1=clamp(U1,.0,1.);
#ifndef Q
k4=U1;
#endif
G+=(1.-G*A3)*R9(y2,U1,A3);}return G;}M5(HB){q(N0,e);
#ifdef GB
q(O0,M);
#endif
#ifdef DB
V(v1,d);
#else
V(S,U2);
#endif
q(J0,d);
#ifdef N
q(m2,F);
#endif
#ifdef AB
q(Y0,e);
#endif
#ifdef H
q(S0,d);
#endif
q(C3,R0);q(W4,c);i r0=G8(
#ifdef GB
O0,
#endif
#ifdef H
a3(S0),
#endif
N0 q3);
#ifndef Q
i bd=L0(q0);
#endif
d F0=
#ifdef DB
v1;
#else
Xc(S);
#endif
c i7=W4;
#ifndef Q
i7+=(bd.xy+bd.zw)*j.cj;
#endif
i7=floor(i7);uint I8=C3.y;uint n2=C3.x+S9(R0(i7),I8);d X1=1.;
#ifdef AB
if(AB){d cd=E3(y4(Y0));X1=min(cd,X1);}
#endif
#ifdef N
if(N&&m2.x!=.0){d dd=L0(o0).x;X1=min(dd,X1);}
#endif
X1=max(X1,.0);F0=clamp(F0,.0,X1);uint G1;d K8;float k4;
#ifndef DB
if(w6(S)){K8=qk(r0.w,F0,n2,G1,k4);}else
#endif
{K8=rk(r0.w,F0,n2,G1,k4);}
#ifdef PB
d l6;if(PB){l6=sb(Y.xy,j.J3,j.K3);}
#endif
#ifdef Q
r0*=K8;
#else
if(H&&a3(S0)!=W3){r0.w*=K8;if(r0.w>.0){bool sk=G1>=j.z2&&(G1&R7)!=0u;if(!sk){r0.xyz=Y4(r0.xyz,bd,a3(S0));if(k4<1.){v L8=r0.xyz;
#ifdef PB
if(PB){L8+=l6*j.cf;}
#endif
r5(j7,A0(L8,.0));memoryBarrier();ej(d1,n2,R7);}}else{r0.xyz=e4(j7).xyz;}}r0.xyz*=r0.w;}else{r0*=K8;}
#endif
#ifdef PB
r0.xyz=M2(r0.xyz,r0.w,l6);
#endif
y0(o0,A0(.0));E4(r0);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive