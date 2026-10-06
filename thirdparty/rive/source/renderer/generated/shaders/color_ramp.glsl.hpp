#pragma once

#include "color_ramp.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char color_ramp[] = R"===(#ifdef BB
c1(d0)
#ifdef ta
K(0,uint,TD);K(1,uint,UD);K(2,uint,VD);K(3,uint,WD);
#else
K(0,M,JC);
#endif
d1
#endif
l2 E0 V(0,i,g7);e2
#ifdef BB
j4 k4 P4 Q4 i Zf(uint p){return Pc((M(p,p,p,p)>>M(16,8,0,24))&0xffu)/255.;}v1(MF,d0,D,G,r){
#ifdef ta
L(r,D,TD,uint);L(r,D,UD,uint);L(r,D,VD,uint);L(r,D,WD,uint);M JC=M(TD,UD,VD,WD);
#else
L(r,D,JC,M);
#endif
T(g7,i);int E8=G>>1;float x=float(E8<=1?JC.x&0xffffu:JC.x>>16)/65536.;float ua=(G&1)==0?.0:1.;if(j.Qc<.0){ua=1.-ua;}uint h7=JC.y;float y=float(h7&~ag)+ua;if((h7&Rc)!=0u&&E8==0){if((h7&va)!=0u) x=.0;else x-=Sc;}if((h7&Tc)!=0u&&E8==3){if((h7&va)!=0u) x=1.;else x+=Sc;}g7=Zf(E8<=1?JC.z:JC.w);e I=F8(c(x,y),2.,j.Qc);
#ifdef MC
I.y=-I.y;
#endif
Z(g7);w1(I);}
#endif
#ifdef FB
N3 O3 j3(i,NF){q(g7,i);P2(g7);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive