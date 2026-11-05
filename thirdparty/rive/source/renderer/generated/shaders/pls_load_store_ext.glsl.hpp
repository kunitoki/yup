#pragma once

#include "pls_load_store_ext.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char pls_load_store_ext[] = R"===(#ifdef CB
void main(){gl_Position=vec4(mix(vec2(-1,1),vec2(1,-1),equal(gl_VertexID&ivec2(1,2),ivec2(0))),0,1);
#ifdef OC
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
#ifdef EF
#if __VERSION__>=310
layout(binding=0,std140) uniform il{uniform highp vec4 uj;}vj;
#else
uniform mediump vec4 FF;
#endif
#endif
#ifdef GL_EXT_shader_pixel_local_storage
#ifdef KE
__pixel_local_inEXT V1
#else
__pixel_local_outEXT V1
#endif
{layout(rgba8) mediump vec4 q0;layout(r32ui) highp uint o0;layout(rgba8) mediump vec4 T4;layout(r32ui) highp uint B8;};
#ifndef GL_ARM_shader_framebuffer_fetch
#ifdef GF
layout(location=0) inout mediump vec4 Fc;
#endif
#endif
#ifdef KE
layout(location=0) out mediump vec4 Fc;
#endif
void main(){
#ifdef EF
#if __VERSION__>=310
q0=vj.uj;
#else
q0=FF;
#endif
#endif
#ifdef GF
#ifdef GL_ARM_shader_framebuffer_fetch
q0=gl_LastFragColorARM;
#else
q0=Fc;
#endif
#endif
#ifdef LE
B8=0u;
#endif
#ifdef EG
o0=0u;
#endif
#ifdef KE
Fc=q0;
#endif
}
#else
layout(location=0) out mediump vec4 wj;void main(){wj=vec4(0,1,0,1);}
#endif
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive