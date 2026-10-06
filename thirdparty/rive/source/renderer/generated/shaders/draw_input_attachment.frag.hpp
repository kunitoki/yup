#pragma once

#include "draw_input_attachment.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_input_attachment_frag[] = R"===(#ifdef FB
layout(input_attachment_index=0,
#ifdef HF
binding=HF,
#else
binding=0,
#endif
set=C3) uniform lowp subpassInput cj;layout(location=0) out i bc;void main(){bc=subpassLoad(cj);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive