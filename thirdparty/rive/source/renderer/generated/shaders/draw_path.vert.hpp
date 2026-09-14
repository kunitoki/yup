#pragma once

#include "draw_path.vert.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_path_vert[] = R"===(#undef E5
#ifdef RF
#define E5 true
#elif defined(GB)
#define E5 GB
#else
#define E5 false
#endif
#undef z2
#ifdef HB
#define z2 g
#else
#define z2 D
#endif
#ifdef CB
A1(a0)
#if defined(DB)||defined(EB)
p0(0,L3,KB);
#else
p0(0,g,SB);p0(1,g,TB);
#endif
B1
#endif
h2 J0 c0(0,g,i1);
#ifdef EB
J0 c0(1,c,C2);
#elif!defined(BB)
#ifdef DB
OB c0(1,d,j1);
#else
J0 c0(2,z2,I);
#endif
OB c0(3,d,z0);
#endif
#ifdef O
#ifdef EB
OB c0(4,d,I3);
#else
OB c0(4,D,S1);
#endif
#endif
#if defined(AB)&&!defined(BB)
J0 c0(5,g,N0);
#endif
#ifdef GB
OB c0(6,d,Z1);
#endif
#ifdef QB
R4 c0(7,W0,e3);c0(8,c,j4);
#endif
a2
#ifdef CB
C1(YB,a0,G,v,T){
#if defined(DB)||defined(EB)
q0(v,G,KB,V);
#else
q0(v,G,SB,g);q0(v,G,TB,g);
#endif
Y(i1,g);
#ifdef EB
Y(C2,c);
#elif!defined(BB)
#ifdef DB
Y(j1,d);
#else
Y(I,z2);
#endif
Y(z0,d);
#endif
#ifdef O
#ifdef EB
Y(I3,d);
#else
Y(S1,D);
#endif
#endif
#if defined(AB)&&!defined(BB)
Y(N0,g);
#endif
#ifdef GB
Y(Z1,d);
#endif
#ifdef QB
Y(e3,W0);Y(j4,c);
#endif
bool Ud=false;uint l0;c i0;
#ifdef BB
X e9;
#endif
#ifdef EB
i0=sb(KB,l0,
#ifdef BB
e9,
#endif
C2 v3);
#elif defined(DB)
i0=tb(KB,l0
#ifdef BB
,e9
#else
,j1
#endif
v3);
#else
g J;Ud=!p9(SB,TB,T,l0,i0
#ifndef BB
,J
#else
,e9
#endif
v3);
#ifndef BB
#ifdef HB
I=J;
#else
I.xy=R7(J.xy);
#endif
#endif
#endif
W0 r1=L5(TC,l0);
#if!defined(EB)&&!defined(BB)
z0=r8(l0,k.Y5);if((r1.x&C9)!=0u)z0=-z0;
#endif
uint j3=r1.x&0xfu;
#ifdef O
if(O){uint rh=(j3==Z7?r1.y:r1.x)>>16;d V0=r8(rh,k.Y5);if(j3==Z7)V0=-V0;
#ifdef EB
I3=V0;
#else
S1.x=V0;
#endif
}
#endif
#ifdef GB
if(GB){Z1=float((r1.x>>4)&0xfu);}
#endif
c K0=i0;
#ifdef SF
K0.y=float(k.ug)-K0.y;
#endif
#ifdef AB
if(AB){Z k2=j2(P0(PB,l0*4u+2u));g D2=P0(PB,l0*4u+3u);
#ifndef BB
N0=T7(k2,D2.xy,K0);
#else
nc(k2,D2.xy,K0 w5);
#endif
}
#endif
if(j3==Ab){i j=unpackUnorm4x8(r1.y);if(E5){}else{j.xyz*=j.w;}i1=g(j);}
#if defined(O)&&!defined(EB)
else if(O&&j3==Z7){d F5=r8(r1.x>>16,k.Y5);S1.y=F5;}
#endif
else{Z sh=j2(P0(PB,l0*4u));g f9=P0(PB,l0*4u+1u);c V4=Z0(sh,K0)+f9.xy;if(j3==E9||j3==wf){i1.w=-uintBitsToFloat(r1.y);float th=f9.z;if(th>.9){i1.z=2.;}else{i1.z=f9.w;}if(j3==E9){i1.y=.0;i1.x=V4.x;}else{i1.z=-i1.z;i1.xy=V4.xy;}}else{float x4=uintBitsToFloat(r1.y);float bb=f9.z;i1=g(V4.x,V4.y,x4,-2.-bb);}}g N;if(!Ud){N=K3(i0);
#ifdef JC
N.y=-N.y;
#endif
#ifdef BB
N.z=ca(e9);
#elif defined(QB)
Q N4=P0(MB,l0*4u+3u);e3=N4.xy;j4=i0+uintBitsToFloat(N4.zw);
#endif
}else{N=g(k.P2,k.P2,k.P2,k.P2);}k0(i1);
#ifdef EB
k0(C2);
#elif!defined(BB)
#ifdef DB
k0(j1);
#else
k0(I);
#endif
k0(z0);
#endif
#ifdef O
#ifdef EB
k0(I3);
#else
k0(S1);
#endif
#endif
#if defined(AB)&&!defined(BB)
k0(N0);
#endif
#ifdef GB
k0(Z1);
#endif
#ifdef QB
k0(e3);k0(j4);
#endif
D1(N);}
#endif
#ifdef FB
N3 O3 e i M7(g q3,float n G6){i j;if(q3.w>=.0){j=Y4(q3);if(E5)j.w*=n;else j*=n;}else if(q3.w>-1.){float t=q3.z>.0?q3.x:length(q3.xy);t=clamp(t,.0,1.);float Vd=abs(q3.z);float x=Vd>1.?(1.-1./da)*t+(.5/da):(1./da)*t+Vd;float uh=-q3.w;j=m2(DD,Bb,c(x,uh),.0);j.w*=n;if(E5){}else{j.xyz*=j.w;}}else{d bb=-q3.w-2.;j=Q6(AC,R5,q3.xy,bb);d x4=q3.z*n;if(E5)j=B0(B6(j),j.w*x4);else j*=x4;}return j;}
#if!defined(DB)&&!defined(EB)
e d Wd(z2 J F3){
#ifdef HB
if(HB&&Db(J))return v4(J g1);else
#endif
return min(J.x,J.y);}e d Xd(z2 J F3){
#if defined(HB)
if(HB&&Eb(J))return d8(J g1);else
#endif
return J.x;}e d cb(z2 J F3){if(Q5(J))return Wd(J g1);else return Xd(J g1);}e d vh(d O4,z2 J F3){if(Q5(J)){d o0=Wd(J g1);return max(o0,O4);}else{d o0=Xd(J g1);return O4+o0;}}
#endif
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive