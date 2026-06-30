#pragma once

#include "draw_input_attachment.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_input_attachment_frag[] = R"===(#ifdef EB
layout(input_attachment_index=0,
#ifdef NE
binding=NE,
#else
binding=0,
#endif
set=B3)uniform lowp subpassInput sh;layout(location=0)out i eb;void main(){eb=subpassLoad(sh);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive