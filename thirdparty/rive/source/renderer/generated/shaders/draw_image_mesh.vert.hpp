#pragma once

#include "draw_image_mesh.vert.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_image_mesh_vert[] = R"===(#ifdef CB
A1(h3)p0(0,c,GC);B1 A1(w3)p0(1,c,HC);B1
#endif
h2 J0 c0(0,c,U0);
#ifdef O
OB c0(1,d,I3);
#endif
#if defined(AB)&&!defined(BB)
J0 c0(2,g,N0);
#endif
a2
#ifdef CB
R3 S3 E6(YB,h3,i3,w3,x3,v){q0(v,i3,GC,c);q0(v,x3,HC,c);Y(U0,c);
#ifdef O
Y(I3,d);
#endif
#if defined(AB)&&!defined(BB)
Y(N0,g);
#endif
c i0=Z0(j2(A0.r9),GC)+A0.c2;U0=HC;
#ifdef O
if(O){I3=r8(A0.V0,k.Y5);}
#endif
#ifdef AB
if(AB){
#ifndef BB
N0=T7(j2(A0.k2),A0.D2,i0 w5);
#else
nc(j2(A0.k2),A0.D2,i0 w5);
#endif
}
#endif
g N=K3(i0);
#ifdef JC
N.y=-N.y;
#endif
#ifdef BB
N.z=ca(A0.Z6);
#endif
k0(U0);
#ifdef O
k0(I3);
#endif
#if defined(AB)&&!defined(BB)
k0(N0);
#endif
D1(N);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive