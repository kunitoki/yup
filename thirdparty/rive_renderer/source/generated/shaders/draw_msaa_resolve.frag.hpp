#pragma once

#include "draw_msaa_resolve.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_msaa_resolve_frag[] = R"===(#ifdef EB
layout(input_attachment_index=0,binding=P2,set=B3)uniform lowp subpassInputMS i9;layout(location=0)out i eb;void main(){eb=(subpassLoad(i9,0)+subpassLoad(i9,1)+subpassLoad(i9,2)+subpassLoad(i9,3))*.25;}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive