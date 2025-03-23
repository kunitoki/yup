#pragma once

#include "pls_load_store_ext.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char pls_load_store_ext[] = R"===(#ifdef AB
void main(){gl_Position=vec4(mix(vec2(-1,1),vec2(1,-1),equal(gl_VertexID&ivec2(1,2),ivec2(0))),0,1);}
#endif
#ifdef HB
#extension GL_EXT_shader_pixel_local_storage:require
#ifdef GL_ARM_shader_framebuffer_fetch
#extension GL_ARM_shader_framebuffer_fetch:require
#else
#extension GL_EXT_shader_framebuffer_fetch:require
#endif
#ifdef OD
#if __VERSION__>=310
layout(binding=0,std140)uniform le{uniform highp vec4 md;}nd;
#else
uniform mediump vec4 PD;
#endif
#endif
#ifdef GL_EXT_shader_pixel_local_storage
#ifdef AD
__pixel_local_inEXT j1
#else
__pixel_local_outEXT j1
#endif
{layout(rgba8)mediump vec4 B0;layout(r32ui)highp uint c1;layout(rgba8)mediump vec4 D3;layout(r32ui)highp uint i4;};
#ifndef GL_ARM_shader_framebuffer_fetch
#ifdef QD
layout(location=0)inout mediump vec4 F8;
#endif
#endif
#ifdef AD
layout(location=0)out mediump vec4 F8;
#endif
void main(){
#ifdef OD
#if __VERSION__>=310
B0=nd.md;
#else
B0=PD;
#endif
#endif
#ifdef QD
#ifdef GL_ARM_shader_framebuffer_fetch
B0=gl_LastFragColorARM;
#else
B0=F8;
#endif
#endif
#ifdef PE
i4=0u;
#endif
#ifdef QE
c1=0u;
#endif
#ifdef AD
F8=B0;
#endif
}
#else
layout(location=0)out mediump vec4 od;void main(){od=vec4(0,1,0,1);}
#endif
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive