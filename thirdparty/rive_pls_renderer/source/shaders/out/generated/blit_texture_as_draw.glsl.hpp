#pragma once

#include "blit_texture_as_draw.exports.h"

namespace rive {
namespace pls {
namespace glsl {
const char blit_texture_as_draw[] = R"===(#ifdef V
O1 P1 V1 W1 g1(DD,P,r,j,L){d j1;j1.x=(j&1)==0?-1.:1.;j1.y=(j&2)==0?-1.:1.;g B=g(j1,0,1);h1(B);}
#endif
#ifdef GB
F2 x1(0,RC);G2 r2(i,SC){i h9=I1(RC,m0(floor(n0.xy)));v2(h9);}
#endif
)===";
} // namespace glsl
} // namespace pls
} // namespace rive