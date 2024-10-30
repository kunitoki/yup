#ifdef VERTEX
void main(){gl_Position=vec4(mix(vec2(-1,1),vec2(1,-1),equal(gl_VertexID&ivec2(1,2),ivec2(0))),0,1);}
#endif
#ifdef FRAGMENT
#extension GL_EXT_shader_pixel_local_storage:enable
#extension GL_ARM_shader_framebuffer_fetch:enable
#extension GL_EXT_shader_framebuffer_fetch:enable
#ifdef CLEAR_COLOR
#if __VERSION__>=310
layout(binding=0,std140)uniform Ib{uniform highp vec4 Pa;}Qa;
#else
uniform mediump vec4 JD;
#endif
#endif
#ifdef GL_EXT_shader_pixel_local_storage
#ifdef STORE_COLOR
__pixel_local_inEXT R0
#else
__pixel_local_outEXT R0
#endif
{layout(rgba8)mediump vec4 q0;
#ifdef ENABLE_CLIPPING
layout(r32ui)highp uint O0;
#endif
layout(rgba8)mediump vec4 P2;layout(r32ui)highp uint C4;};
#ifndef GL_ARM_shader_framebuffer_fetch
#ifdef LOAD_COLOR
layout(location=0)inout mediump vec4 S6;
#endif
#endif
#ifdef STORE_COLOR
layout(location=0)out mediump vec4 S6;
#endif
void main(){
#ifdef CLEAR_COLOR
#if __VERSION__>=310
q0=Qa.Pa;
#else
q0=JD;
#endif
#endif
#ifdef LOAD_COLOR
#ifdef GL_ARM_shader_framebuffer_fetch
q0=gl_LastFragColorARM;
#else
q0=S6;
#endif
#endif
#ifdef CLEAR_COVERAGE
C4=0u;
#endif
#ifdef CLEAR_CLIP
O0=0u;
#endif
#ifdef STORE_COLOR
S6=q0;
#endif
}
#else
layout(location=0)out mediump vec4 Ra;void main(){Ra=vec4(0,1,0,1);}
#endif
#endif
