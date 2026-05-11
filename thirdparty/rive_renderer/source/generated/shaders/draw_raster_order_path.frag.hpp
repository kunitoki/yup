#pragma once

#include "draw_raster_order_path.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_raster_order_path_frag[] = R"===(#ifdef EB
K1 p0(P2,j0);f1(Q2,e0);p0(d6,f4);f1(F6,H7);L1 v1(HB){B(k1,g);
#ifdef CB
B(l1,d);
#else
B(I,z2);
#endif
B(z0,d);
#ifdef M
B(S1,D);
#endif
#ifdef Z
B(N0,g);
#endif
#ifdef FB
B(Y1,d);
#endif
#if!defined(CB)
v2;
#endif
D O4=unpackHalf2x16(e1(H7));d h9=O4.y;d n0=h9==z0?O4.x:G0(.0);
#ifdef CB
n0+=l1;X1(H7);
#else
n0=nh(n0,I i1);h1(H7,packHalf2x16(A2(n0,z0)));
#endif
d n;
#ifdef YC
if(YC){n=W9(n0,G0(.0),G0(1.));}else
#endif
{n=abs(n0);
#ifdef IC
if(IC&&z0<.0){n=1.-G0(abs(fract(n*.5)*2.+-1.));}
#endif
n=min(n,G0(1.));}
#ifdef M
if(M&&S1.x<.0){d V0=-S1.x;
#ifdef NC
if(NC){d D5=S1.y;if(D5!=.0){D O0=unpackHalf2x16(e1(e0));d A6=O0.y;d j4;if(A6!=V0){j4=A6==D5?O0.x:.0;
#ifndef CB
C0(f4,B0(j4,.0,.0,.0));
#endif
}else{j4=H0(f4).x;
#ifndef CB
r2(f4);
#endif
}n=min(n,j4);}}
#endif
h1(e0,packHalf2x16(A2(n,V0)));r2(j0);}else
#endif
{
#ifdef M
if(M){d V0=S1.x;if(V0!=.0){D O0=unpackHalf2x16(e1(e0));d A6=O0.y;n=(A6==V0)?min(O0.x,n):G0(.0);}}
#endif
#ifdef Z
if(Z){d V4=d3(X4(N0));n=clamp(V4,G0(.0),n);}
#endif
i j=M7(k1,n R2);i M1;if(h9!=z0){M1=H0(j0);
#ifndef CB
C0(f4,M1);
#endif
}else{M1=H0(f4);
#ifndef CB
r2(f4);
#endif
}
#ifdef FB
if(FB){if(Y1!=V5(K5)){j.xyz=R4(j.xyz,M1,W5(Y1));}j.xyz*=j.w;}
#endif
#ifdef VB
if(VB){j=h3(j);}
#endif
j+=M1*(1.-j.w);j.xyz=M3(j.xyz,T.xy,k.v3,k.w3);C0(j0,j);X1(e0);}
#if!defined(CB)
w2;
#endif
c2;}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive