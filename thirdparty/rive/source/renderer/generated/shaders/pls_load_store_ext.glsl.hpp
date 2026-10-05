#pragma once

#include "pls_load_store_ext.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char pls_load_store_ext[] = R"===(#ifdef BB
void main(){gl_Position=vec4(mix(vec2(-1,1),vec2(1,-1),equal(gl_VertexID&ivec2(1,2),ivec2(0))),0,1);
#ifdef MC
gl_Position.y=-gl_Position.y;
#endif
}
#endif
#ifdef FB
#extension GL_EXT_shader_pixel_local_storage:require
#ifdef GL_ARM_shader_framebuffer_fetch
#extension GL_ARM_shader_framebuffer_fetch:require
#else
#extension GL_EXT_shader_framebuffer_fetch:require
#endif
#ifdef XE
#if __VERSION__>=310
layout(binding=0,std140) uniform Vj{uniform highp vec4 di;}ei;
#else
uniform mediump vec4 YE;
#endif
#endif
#ifdef GL_EXT_shader_pixel_local_storage
#ifdef EE
__pixel_local_inEXT a2
#else
__pixel_local_outEXT a2
#endif
{layout(rgba8) mediump vec4 n0;layout(r32ui) highp uint m0;layout(rgba8) mediump vec4 B4;layout(r32ui) highp uint R7;};
#ifndef GL_ARM_shader_framebuffer_fetch
#ifdef ZE
layout(location=0) inout mediump vec4 xb;
#endif
#endif
#ifdef EE
layout(location=0) out mediump vec4 xb;
#endif
void main(){
#ifdef XE
#if __VERSION__>=310
n0=ei.di;
#else
n0=YE;
#endif
#endif
#ifdef ZE
#ifdef GL_ARM_shader_framebuffer_fetch
n0=gl_LastFragColorARM;
#else
n0=xb;
#endif
#endif
#ifdef FE
R7=0u;
#endif
#ifdef XF
m0=0u;
#endif
#ifdef EE
xb=n0;
#endif
}
#else
layout(location=0) out mediump vec4 fi;void main(){fi=vec4(0,1,0,1);}
#endif
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive