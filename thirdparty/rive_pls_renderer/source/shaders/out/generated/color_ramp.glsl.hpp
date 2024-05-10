#pragma once

#include "color_ramp.exports.h"

namespace rive {
namespace pls {
namespace glsl {
const char color_ramp[] = R"===(#ifdef V
U0(P)q0(0,T,QB);V0
#endif
A1 k0 I(0,i,W3);B1
#ifdef V
O1 P1 V1 W1 i i9(uint f){return M0((T(f,f,f,f)>>T(16,8,0,24))&0xffu)/255.;}g1(ED,P,r,j,L){v0(L,r,QB,T);Q(W3,i);float x=float((j&1)==0?QB.x&0xffffu:QB.x>>16)/65536.;float D5=(j&2)==0?1.:.0;if(v.E5<.0){D5=1.-D5;}W3=i9((j&1)==0?QB.z:QB.w);g B;B.x=x*2.-1.;B.y=(float(QB.y)+D5)*v.E5-sign(v.E5);B.zw=d(0,1);S(W3);h1(B);}
#endif
#ifdef GB
r2(i,FD){N(W3,i);v2(W3);}
#endif
)===";
} // namespace glsl
} // namespace pls
} // namespace rive