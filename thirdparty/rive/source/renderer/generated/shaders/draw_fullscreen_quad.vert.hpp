#pragma once

#include "draw_fullscreen_quad.vert.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_fullscreen_quad_vert[] = R"===(#ifdef CB
h1(d0) i1 n1(UB,d0,A,c4,H6){e I;I.x=(c4&1)==0?-1.:1.;I.y=(c4&2)==0?-1.:1.;I.z=0.;I.w=1.;C1(I);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive