#pragma once

#include "draw_input_attachment.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_input_attachment_frag[] = R"===(#ifdef FB
layout(input_attachment_index=0,
#ifdef SE
binding=SE,
#else
binding=0,
#endif
set=E3)uniform lowp subpassInput Ah;layout(location=0)out i jb;void main(){jb=subpassLoad(Ah);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive