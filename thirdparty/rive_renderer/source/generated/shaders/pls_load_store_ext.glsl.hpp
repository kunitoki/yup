#pragma once

#include "pls_load_store_ext.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char pls_load_store_ext[] = R"===(#ifdef BB
void main(){gl_Position=vec4(mix(vec2(-1,1),vec2(1,-1),equal(gl_VertexID&ivec2(1,2),ivec2(0))),0,1);
#ifdef JC
gl_Position.y=-gl_Position.y;
#endif
}
#endif
#ifdef EB
#extension GL_EXT_shader_pixel_local_storage:require
#ifdef GL_ARM_shader_framebuffer_fetch
#extension GL_ARM_shader_framebuffer_fetch:require
#else
#extension GL_EXT_shader_framebuffer_fetch:require
#endif
#ifdef DE
#if __VERSION__>=310
layout(binding=0,std140)uniform ki{uniform highp vec4 zg;}Ag;
#else
uniform mediump vec4 EE;
#endif
#endif
#ifdef GL_EXT_shader_pixel_local_storage
#ifdef PD
__pixel_local_inEXT n1
#else
__pixel_local_outEXT n1
#endif
{layout(rgba8)mediump vec4 j0;layout(r32ui)highp uint e0;layout(rgba8)mediump vec4 f4;layout(r32ui)highp uint H7;};
#ifndef GL_ARM_shader_framebuffer_fetch
#ifdef FE
layout(location=0)inout mediump vec4 Ia;
#endif
#endif
#ifdef PD
layout(location=0)out mediump vec4 Ia;
#endif
void main(){
#ifdef DE
#if __VERSION__>=310
j0=Ag.zg;
#else
j0=EE;
#endif
#endif
#ifdef FE
#ifdef GL_ARM_shader_framebuffer_fetch
j0=gl_LastFragColorARM;
#else
j0=Ia;
#endif
#endif
#ifdef QD
H7=0u;
#endif
#ifdef HF
e0=0u;
#endif
#ifdef PD
Ia=j0;
#endif
}
#else
layout(location=0)out mediump vec4 Bg;void main(){Bg=vec4(0,1,0,1);}
#endif
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive