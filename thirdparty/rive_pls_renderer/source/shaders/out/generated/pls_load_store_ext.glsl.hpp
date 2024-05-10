#pragma once

#include "pls_load_store_ext.exports.h"

namespace rive {
namespace pls {
namespace glsl {
const char pls_load_store_ext[] = R"===(#ifdef V
void main(){gl_Position=vec4(mix(vec2(-1,1),vec2(1,-1),equal(gl_VertexID&ivec2(1,2),ivec2(0))),0,1);}
#endif
#ifdef GB
#extension GL_EXT_shader_pixel_local_storage:enable
#extension GL_ARM_shader_framebuffer_fetch:enable
#extension GL_EXT_shader_framebuffer_fetch:enable
#ifdef VC
#if __VERSION__>=310
layout(binding=0,std140)uniform db{uniform highp vec4 qa;}ra;
#else
uniform mediump vec4 WC;
#endif
#endif
#ifdef GL_EXT_shader_pixel_local_storage
#ifdef LC
__pixel_local_inEXT H0
#else
__pixel_local_outEXT H0
#endif
{layout(rgba8)mediump vec4 c0;layout(r32ui)highp uint Q0;layout(rgba8)mediump vec4 w2;
#ifdef BB
layout(r32ui)highp uint w0;
#endif
};
#ifndef GL_ARM_shader_framebuffer_fetch
#ifdef XC
layout(location=0)inout mediump vec4 l6;
#endif
#endif
#ifdef LC
layout(location=0)out mediump vec4 l6;
#endif
void main(){
#ifdef VC
#if __VERSION__>=310
c0=ra.qa;
#else
c0=WC;
#endif
#endif
#ifdef XC
#ifdef GL_ARM_shader_framebuffer_fetch
c0=gl_LastFragColorARM;
#else
c0=l6;
#endif
#endif
#ifdef RD
Q0=0u;
#endif
#ifdef SD
w0=0u;
#endif
#ifdef LC
l6=c0;
#endif
}
#else
layout(location=0)out mediump vec4 sa;void main(){sa=vec4(0,1,0,1);}
#endif
#endif
)===";
} // namespace glsl
} // namespace pls
} // namespace rive