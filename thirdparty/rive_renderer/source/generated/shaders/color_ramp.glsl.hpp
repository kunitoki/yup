#pragma once

#include "color_ramp.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char color_ramp[] = R"===(#ifdef AB
Y0(V)
#ifdef h6
a0(0,uint,OC);a0(1,uint,PC);a0(2,uint,QC);a0(3,uint,RC);
#else
a0(0,G,NB);
#endif
Z0
#endif
H1 w0 W(0,i,v4);I1
#ifdef AB
R1 S1 f2 g2 i P9(uint j){return E7((G(j,j,j,j)>>G(16,8,0,24))&0xffu)/255.;}h1(RD,V,q,n,H){
#ifdef h6
f0(H,q,OC,uint);f0(H,q,PC,uint);f0(H,q,QC,uint);f0(H,q,RC,uint);G NB=G(OC,PC,QC,RC);
#else
f0(H,q,NB,G);
#endif
c0(v4,i);float x=float((n&1)==0?NB.x&0xffffu:NB.x>>16)/65536.;float i6=(n&2)==0?1.:.0;if(P.j6<.0){i6=1.-i6;}v4=P9((n&1)==0?NB.z:NB.w);g T;T.x=x*2.-1.;T.y=(float(NB.y)+i6)*P.j6-sign(P.j6);T.zw=f(0,1);d0(v4);i1(T);}
#endif
#ifdef EB
q2(i,SD){Y(v4,i);r2(v4);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive