#pragma once

#include "blit_texture_as_draw.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char blit_texture_as_draw[] = R"===(#ifdef AB
R1 S1 f2 g2 h1(QD,V,q,n,H){f r1;r1.x=(n&1)==0?-1.:1.;r1.y=(n&2)==0?-1.:1.;g T=g(r1,0,1);i1(T);}
#endif
#ifdef EB
H2 v1(p2,0,DD);I2 q2(i,ED){i O9=N1(DD,e0(floor(h0.xy)));r2(O9);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive