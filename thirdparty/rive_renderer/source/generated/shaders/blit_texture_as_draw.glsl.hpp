#pragma once

#include "blit_texture_as_draw.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char blit_texture_as_draw[] = R"===(#ifdef AB
H2 I2 r3 v3 Z0(WD,e0,o,l,I){c j2;j2.x=(l&1)==0?-1.:1.;j2.y=(l&2)==0?-1.:1.;e Q=e(j2,0,1);U0(Q);}
#endif
#ifdef HB
J2 o2(K2,0,ID);L2 T1(i,JD){i Ib=f1(ID,f0(floor(v0.xy)));U1(Ib);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive