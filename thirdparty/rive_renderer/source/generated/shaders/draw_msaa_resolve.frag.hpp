#pragma once

#include "draw_msaa_resolve.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_msaa_resolve_frag[] = R"===(#ifdef FB
layout(input_attachment_index=0,binding=Q2,set=E3)uniform lowp subpassInputMS i9;layout(location=0)out i jb;void main(){jb=(subpassLoad(i9,0)+subpassLoad(i9,1)+subpassLoad(i9,2)+subpassLoad(i9,3))*.25;}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive