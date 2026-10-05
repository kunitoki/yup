#pragma once

#include "draw_msaa_resolve.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_msaa_resolve_frag[] = R"===(#ifdef FB
layout(input_attachment_index=0,binding=K2,set=C3) uniform lowp subpassInputMS E9;layout(location=0) out i bc;void main(){bc=(subpassLoad(E9,0)+subpassLoad(E9,1)+subpassLoad(E9,2)+subpassLoad(E9,3))*.25;}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive