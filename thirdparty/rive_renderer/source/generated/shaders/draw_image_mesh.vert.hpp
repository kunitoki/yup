#pragma once

#include "draw_image_mesh.vert.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_image_mesh_vert[] = R"===(#ifdef BB
A1(e3)r0(0,c,FC);B1 A1(q3)r0(1,c,GC);B1
#endif
h2 J0 d0(0,c,U0);
#ifdef M
NB d0(1,d,F3);
#endif
#if defined(Z)&&!defined(AB)
J0 d0(2,g,N0);
#endif
Z1
#ifdef BB
P3 Q3 E6(XB,e3,f3,q3,r3,v){v0(v,f3,FC,c);v0(v,r3,GC,c);Y(U0,c);
#ifdef M
Y(F3,d);
#endif
#if defined(Z)&&!defined(AB)
Y(N0,g);
#endif
c i0=Z0(j2(A0.r9),FC)+A0.a2;U0=GC;
#ifdef M
if(M){F3=r8(A0.V0,k.Y5);}
#endif
#ifdef Z
if(Z){
#ifndef AB
N0=T7(j2(A0.k2),A0.D2,i0 r5);
#else
ic(j2(A0.k2),A0.D2,i0 r5);
#endif
}
#endif
g O=H3(i0);
#ifdef JC
O.y=-O.y;
#endif
#ifdef AB
O.z=ca(A0.Z6);
#endif
l0(U0);
#ifdef M
l0(F3);
#endif
#if defined(Z)&&!defined(AB)
l0(N0);
#endif
D1(O);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive