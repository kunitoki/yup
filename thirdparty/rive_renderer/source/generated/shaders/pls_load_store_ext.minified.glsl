#ifdef VERTEX
void main(){gl_Position=vec4(mix(vec2(-1,1),vec2(1,-1),equal(gl_VertexID&ivec2(1,2),ivec2(0))),0,1);}
#endif
#ifdef FRAGMENT
#extension GL_EXT_shader_pixel_local_storage:require
#ifdef GL_ARM_shader_framebuffer_fetch
#extension GL_ARM_shader_framebuffer_fetch:require
#else
#extension GL_EXT_shader_framebuffer_fetch:require
#endif
#ifdef CLEAR_COLOR
#if __VERSION__>=310
layout(binding=0,std140)uniform Rf{uniform highp vec4 Fe;}Ge;
#else
uniform mediump vec4 SD;
#endif
#endif
#ifdef GL_EXT_shader_pixel_local_storage
#ifdef STORE_COLOR
__pixel_local_inEXT z1
#else
__pixel_local_outEXT z1
#endif
{layout(rgba8)mediump vec4 H0;layout(r32ui)highp uint r1;layout(rgba8)mediump vec4 N3;layout(r32ui)highp uint x4;};
#ifndef GL_ARM_shader_framebuffer_fetch
#ifdef LOAD_COLOR
layout(location=0)inout mediump vec4 D9;
#endif
#endif
#ifdef STORE_COLOR
layout(location=0)out mediump vec4 D9;
#endif
void main(){
#ifdef CLEAR_COLOR
#if __VERSION__>=310
H0=Ge.Fe;
#else
H0=SD;
#endif
#endif
#ifdef LOAD_COLOR
#ifdef GL_ARM_shader_framebuffer_fetch
H0=gl_LastFragColorARM;
#else
H0=D9;
#endif
#endif
#ifdef CLEAR_COVERAGE
x4=0u;
#endif
#ifdef CLEAR_CLIP
r1=0u;
#endif
#ifdef STORE_COLOR
D9=H0;
#endif
}
#else
layout(location=0)out mediump vec4 He;void main(){He=vec4(0,1,0,1);}
#endif
#endif
