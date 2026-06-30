#pragma once

#include "color_ramp.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char color_ramp[] = R"===(#ifdef BB
A1(c0)
#ifdef O9
r0(0,uint,FD);r0(1,uint,GD);r0(2,uint,HD);r0(3,uint,ID);
#else
r0(0,Q,AC);
#endif
B1
#endif
h2 J0 d0(0,i,R6);Z1
#ifdef BB
P3 Q3 B4 C4 i Pe(uint j){return Ob((Q(j,j,j,j)>>Q(16,8,0,24))&0xffu)/255.;}C1(UE,c0,G,v,S){
#ifdef O9
v0(S,G,FD,uint);v0(S,G,GD,uint);v0(S,G,HD,uint);v0(S,G,ID,uint);Q AC=Q(FD,GD,HD,ID);
#else
v0(S,G,AC,Q);
#endif
Y(R6,i);int n8=v>>1;float x=float(n8<=1?AC.x&0xffffu:AC.x>>16)/65536.;float P9=(v&1)==0?.0:1.;if(k.Pb<.0){P9=1.-P9;}uint S6=AC.y;float y=float(S6&~Qe)+P9;if((S6&Qb)!=0u&&n8==0){if((S6&Q9)!=0u)x=.0;else x-=Rb;}if((S6&Sb)!=0u&&n8==3){if((S6&Q9)!=0u)x=1.;else x+=Rb;}R6=Pe(n8<=1?AC.z:AC.w);g O=o8(c(x,y),2.,k.Pb);
#ifdef JC
O.y=-O.y;
#endif
l0(R6);D1(O);}
#endif
#ifdef EB
y3 z3 V2(i,VE){B(R6,i);F2(R6);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive