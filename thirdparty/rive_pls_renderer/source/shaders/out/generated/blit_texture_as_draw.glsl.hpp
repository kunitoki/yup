#pragma once

#include "blit_texture_as_draw.exports.h"

namespace rive {
namespace pls {
namespace glsl {
const char blit_texture_as_draw[] = R"===(#ifdef Y
Q1 R1 Y1 Z1 g1(ED,R,v,k,O){d j1;j1.x=(k&1)==0?-1.:1.;j1.y=(k&2)==0?-1.:1.;g C=g(j1,0,1);h1(C);}
#endif
#ifdef HB
K2 z1(d2,0,TC);L2 w2(j,UC){j z9=K1(TC,o0(floor(p0.xy)));x2(z9);}
#endif
)===";
} // namespace glsl
} // namespace pls
} // namespace rive