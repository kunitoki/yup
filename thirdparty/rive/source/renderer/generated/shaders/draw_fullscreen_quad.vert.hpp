#pragma once

#include "draw_fullscreen_quad.vert.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_fullscreen_quad_vert[] = R"===(#ifdef BB
c1(d0) d1 v1(RB,d0,D,p3,F6){e I;I.x=(p3&1)==0?-1.:1.;I.y=(p3&2)==0?-1.:1.;I.z=0.;I.w=1.;w1(I);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive