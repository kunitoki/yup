#pragma once

#include "color_ramp.exports.h"

namespace rive {
namespace pls {
namespace glsl {
const char color_ramp[] = R"===(#ifdef Y
U0(R)v0(0,W,RB);V0
#endif
D1 n0 K(0,j,c4);E1
#ifdef Y
Q1 R1 Y1 Z1 j A9(uint f){return j0((W(f,f,f,f)>>W(16,8,0,24))&0xffu)/255.;}g1(FD,R,v,k,O){z0(O,v,RB,W);S(c4,j);float x=float((k&1)==0?RB.x&0xffffu:RB.x>>16)/65536.;float M5=(k&2)==0?1.:.0;if(A.N5<.0){M5=1.-M5;}c4=A9((k&1)==0?RB.z:RB.w);g C;C.x=x*2.-1.;C.y=(float(RB.y)+M5)*A.N5-sign(A.N5);C.zw=d(0,1);U(c4);h1(C);}
#endif
#ifdef HB
w2(j,GD){P(c4,j);x2(c4);}
#endif
)===";
} // namespace glsl
} // namespace pls
} // namespace rive