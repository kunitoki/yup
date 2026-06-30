#pragma once

#include "blit_texture_as_draw.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char blit_texture_as_draw[] = R"===(h2
#ifdef WC
J0 d0(0,c,U0);
#endif
Z1
#ifdef BB
P3 Q3 B4 C4 A1(c0)B1 C1(TE,c0,G,v,S){c l2;l2.x=(v&1)==0?-1.:1.;l2.y=(v&2)==0?-1.:1.;
#ifdef WC
Y(U0,c);U0.x=l2.x*.5+.5;U0.y=l2.y*-.5+.5;l0(U0);
#endif
g O=g(l2,0,1);D1(O);}
#endif
#ifdef EB
y3
#ifdef ED
Ne(Y4,R3,ZB);
#else
U2(Y4,R3,ZB);
#endif
z3
#ifdef WC
Z4 S3(Oe)a5
#endif
V2(i,YD){i l8;
#ifdef WC
B(U0,c);l8=Q6(ZB,Oe,U0,.0);
#elif defined(ED)
l8=(m8(ZB,0,U(floor(T.xy)))+m8(ZB,1,U(floor(T.xy)))+m8(ZB,2,U(floor(T.xy)))+m8(ZB,3,U(floor(T.xy))))*0.25;
#else
l8=F1(ZB,U(floor(T.xy)));
#endif
F2(l8);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive