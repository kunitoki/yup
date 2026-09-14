#pragma once

#include "color_ramp.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char color_ramp[] = R"===(#ifdef CB
A1(a0)
#ifdef O9
p0(0,uint,HD);p0(1,uint,ID);p0(2,uint,JD);p0(3,uint,KD);
#else
p0(0,Q,CC);
#endif
B1
#endif
h2 J0 c0(0,i,R6);a2
#ifdef CB
R3 S3 A4 B4 i Ve(uint j){return Tb((Q(j,j,j,j)>>Q(16,8,0,24))&0xffu)/255.;}C1(XE,a0,G,v,T){
#ifdef O9
q0(T,G,HD,uint);q0(T,G,ID,uint);q0(T,G,JD,uint);q0(T,G,KD,uint);Q CC=Q(HD,ID,JD,KD);
#else
q0(T,G,CC,Q);
#endif
Y(R6,i);int n8=v>>1;float x=float(n8<=1?CC.x&0xffffu:CC.x>>16)/65536.;float P9=(v&1)==0?.0:1.;if(k.Ub<.0){P9=1.-P9;}uint S6=CC.y;float y=float(S6&~We)+P9;if((S6&Vb)!=0u&&n8==0){if((S6&Q9)!=0u)x=.0;else x-=Wb;}if((S6&Xb)!=0u&&n8==3){if((S6&Q9)!=0u)x=1.;else x+=Wb;}R6=Ve(n8<=1?CC.z:CC.w);g N=o8(c(x,y),2.,k.Ub);
#ifdef JC
N.y=-N.y;
#endif
k0(R6);D1(N);}
#endif
#ifdef FB
B3 C3 Y2(i,YE){B(R6,i);G2(R6);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive