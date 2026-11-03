#pragma once

#include "draw_input_attachment.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_input_attachment_frag[] = R"===(#ifdef EB
layout(input_attachment_index=0,
#ifdef NF
binding=NF,
#else
binding=0,
#endif
set=N3) uniform lowp subpassInput tk;layout(location=0) out i ed;void main(){ed=subpassLoad(tk);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive