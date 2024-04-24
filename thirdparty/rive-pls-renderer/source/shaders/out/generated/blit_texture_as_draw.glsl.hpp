#pragma once

#include "blit_texture_as_draw.exports.h"

namespace rive {
namespace pls {
namespace glsl {
const char blit_texture_as_draw[] = R"===(#ifdef W
N1 O1 U1 V1 e1(FD,P,r,j,L){d h1;h1.x=(j&1)==0?-1.:1.;h1.y=(j&2)==0?-1.:1.;g B=g(h1,0,1);f1(B);}
#endif
#ifdef HB
E2 w1(0,TC);F2 q2(i,UC){i f9=H1(TC,m0(floor(n0.xy)));r2(f9);}
#endif
)===";
} // namespace glsl
} // namespace pls
} // namespace rive