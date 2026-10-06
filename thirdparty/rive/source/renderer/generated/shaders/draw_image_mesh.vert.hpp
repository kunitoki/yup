#pragma once

#include "draw_image_mesh.vert.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_image_mesh_vert[] = R"===(#ifdef BB
c1(w3) K(0,c,PC);d1 c1(J3) K(1,c,QC);d1 c1(A1) K(L9,e,YB);K(M9,e,SB);K(N9,e,PB);K(O9,uint,ZB);K(P9,uint,AC);K(Q9,uint,BC);K(R9,uint,LC);K(Z9,e,HC);d1
#endif
l2 E0 V(0,c,V5);
#ifdef A
KB V(1,d,Y3);
#endif
#if defined(AB)&&!defined(CB)
E0 V(2,e,R0);
#endif
KB V(3,i,Q1);
#ifdef N
Z2 V(4,Q,H1);
#endif
e2
#ifdef BB
j4 k4 T6(RB,w3,x3,J3,K3,A1,h0,G){L(G,x3,PC,c);L(G,K3,QC,c);L(r,h0,YB,e);L(r,h0,SB,e);L(r,h0,PB,e);L(r,h0,ZB,uint);L(r,h0,AC,uint);L(r,h0,BC,uint);L(r,h0,LC,uint);L(r,h0,HC,e);T(V5,c);
#ifdef A
T(Y3,d);
#endif
#if defined(AB)&&!defined(CB)
T(R0,e);
#endif
T(Q1,i);
#ifdef N
T(H1,Q);
#endif
c k0=M0(n1(YB),PC)+PB.xy;V5=QC*HC.zw+HC.xy;
#ifdef A
if(A){Y3=l6(AC,j.T4);}
#endif
#ifdef AB
if(AB){
#ifndef CB
R0=i8(n1(SB),PB.zw,k0 Y4);
#else
Ga(n1(SB),PB.zw,k0 Y4);
#endif
}
#endif
e I=H3(k0);
#ifdef MC
I.y=-I.y;
#endif
#ifdef CB
I.z=I8(LC,0xffu);
#endif
Q1=unpackUnorm4x8(ZB);
#ifdef N
H1=P1(BC);
#endif
Z(V5);
#ifdef A
Z(Y3);
#endif
#if defined(AB)&&!defined(CB)
Z(R0);
#endif
Z(Q1);
#ifdef N
Z(H1);
#endif
w1(I);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive