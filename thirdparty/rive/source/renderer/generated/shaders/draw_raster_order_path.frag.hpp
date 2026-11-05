#pragma once

#include "draw_raster_order_path.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_raster_order_path_frag[] = R"===(#ifdef EB
I1 z0(B2,q0);x1(o3,o0);z0(F6,T4);x1(o7,B8);J1 E1(HB){q(N0,e);
#ifdef GB
q(O0,M);
#endif
#ifdef DB
q(v1,d);
#else
q(S,U2);
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
#if!defined(DB)
R2;
#endif
F B5=unpackHalf2x16(o1(B8));d qa=B5.y;d x0=qa==J0?B5.x:B0(.0);
#ifdef DB
x0+=v1;l2(B8);
#else
x0=ok(x0,S r1);q1(B8,packHalf2x16(V2(x0,J0)));
#endif
d n;
#ifdef ME
if(ME){n=pb(x0,B0(.0),B0(1.));}else
#endif
{n=abs(x0);
#ifdef ZC
if(ZC&&J0<.0){n=1.-B0(abs(fract(n*.5)*2.+-1.));}
#endif
n=min(n,B0(1.));}
#ifdef N
if(N&&m2.x<.0){d D1=-m2.x;
#ifdef DD
if(DD){d k6=m2.y;if(k6!=.0){F Z0=unpackHalf2x16(o1(o0));d k7=Z0.y;d X4;if(k7!=D1){X4=k7==k6?Z0.x:.0;
#ifndef DB
y0(T4,A0(X4,.0,.0,.0));
#endif
}else{X4=L0(T4).x;
#ifndef DB
Q2(T4);
#endif
}n=min(n,X4);}}
#endif
q1(o0,packHalf2x16(V2(n,D1)));Q2(q0);}else
#endif
{
#ifdef N
if(N){d D1=m2.x;if(D1!=.0){F Z0=unpackHalf2x16(o1(o0));d k7=Z0.y;n=(k7==D1)?min(Z0.x,n):B0(.0);}}
#endif
#ifdef AB
if(AB){d G5=E3(y4(Y0));n=clamp(G5,B0(.0),n);}
#endif
i k=G8(
#ifdef GB
O0,
#endif
#ifdef H
a3(S0),
#endif
N0 q3);i X0;if(qa!=J0){X0=L0(q0);
#ifndef DB
y0(T4,X0);
#endif
}else{X0=L0(T4);
#ifndef DB
Q2(T4);
#endif
}bool dg=false;
#ifdef GB
dg=GB&&O0.z<.0;
#endif
if(dg){
#ifdef GB
uint Vb=uint(-O0.z-1.);d Wb=Fe(k,Vb);k=X0*mix(B0(1.),Wb,n);y0(q0,k);l2(o0);
#endif
}else{
#ifdef H
if(H&&S0!=z6(W3)){k.xyz=Y4(k.xyz,X0,a3(S0))*k.w;}
#endif
k*=n;d A3=k.w;k+=X0*(1.-A3);k.xyz=M2(k.xyz,A3,Y.xy,j.J3,j.K3);y0(q0,k);l2(o0);}}
#if!defined(DB)
S2;
#endif
L1;}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive