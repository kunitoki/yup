#pragma once

#include "draw_fullscreen_quad.vert.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_fullscreen_quad_vert[] = R"===(#ifdef BB
void main(){gl_Position.x=(gl_VertexID&1)==0?-1.:1.;gl_Position.y=(gl_VertexID&2)==0?-1.:1.;gl_Position.z=0.;gl_Position.w=1.;}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive