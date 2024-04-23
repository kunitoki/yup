#pragma once

#include "pls_load_store_ext.exports.h"

namespace rive {
namespace pls {
namespace glsl {
const char pls_load_store_ext[] = R"===(#ifdef W
void main(){gl_Position=vec4(mix(vec2(-1,1),vec2(1,-1),equal(gl_VertexID&ivec2(1,2),ivec2(0))),0,1);}
#endif
#ifdef HB
#extension GL_EXT_shader_pixel_local_storage:enable
#extension GL_ARM_shader_framebuffer_fetch:enable
#extension GL_EXT_shader_framebuffer_fetch:enable
#ifdef XC
#if __VERSION__>=310
layout(binding=0,std140)uniform bb{uniform highp vec4 oa;}pa;
#else
uniform mediump vec4 YC;
#endif
#endif
#ifdef GL_EXT_shader_pixel_local_storage
#ifdef NC
__pixel_local_inEXT H0
#else
__pixel_local_outEXT H0
#endif
{layout(rgba8)mediump vec4 X;layout(r32ui)highp uint x0;layout(rgba8)mediump vec4 v2;layout(r32ui)highp uint r0;};
#ifndef GL_ARM_shader_framebuffer_fetch
#ifdef ZC
layout(location=0)inout mediump vec4 k6;
#endif
#endif
#ifdef NC
layout(location=0)out mediump vec4 k6;
#endif
void main(){
#ifdef XC
#if __VERSION__>=310
X=pa.oa;
#else
X=YC;
#endif
#endif
#ifdef ZC
#ifdef GL_ARM_shader_framebuffer_fetch
X=gl_LastFragColorARM;
#else
X=k6;
#endif
#endif
#ifdef TD
x0=0u;
#endif
#ifdef UD
r0=0u;
#endif
#ifdef NC
k6=X;
#endif
}
#else
layout(location=0)out mediump vec4 qa;void main(){qa=vec4(0,1,0,1);}
#endif
#endif
)===";
} // namespace glsl
} // namespace pls
} // namespace rive