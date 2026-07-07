#ifdef VERTEX
void main(){gl_Position=vec4(mix(vec2(-1,1),vec2(1,-1),equal(gl_VertexID&ivec2(1,2),ivec2(0))),0,1);
#ifdef POST_INVERT_Y
gl_Position.y=-gl_Position.y;
#endif
}
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
layout(binding=0,std140)uniform mi{uniform highp vec4 Fg;}Gg;
#else
uniform mediump vec4 KE;
#endif
#endif
#ifdef GL_EXT_shader_pixel_local_storage
#ifdef STORE_COLOR
__pixel_local_inEXT n1
#else
__pixel_local_outEXT n1
#endif
{layout(rgba8)mediump vec4 g0;layout(r32ui)highp uint d0;layout(rgba8)mediump vec4 g4;layout(r32ui)highp uint H7;};
#ifndef GL_ARM_shader_framebuffer_fetch
#ifdef LOAD_COLOR
layout(location=0)inout mediump vec4 Na;
#endif
#endif
#ifdef STORE_COLOR
layout(location=0)out mediump vec4 Na;
#endif
void main(){
#ifdef CLEAR_COLOR
#if __VERSION__>=310
g0=Gg.Fg;
#else
g0=KE;
#endif
#endif
#ifdef LOAD_COLOR
#ifdef GL_ARM_shader_framebuffer_fetch
g0=gl_LastFragColorARM;
#else
g0=Na;
#endif
#endif
#ifdef CLEAR_COVERAGE
H7=0u;
#endif
#ifdef CLEAR_CLIP
d0=0u;
#endif
#ifdef STORE_COLOR
Na=g0;
#endif
}
#else
layout(location=0)out mediump vec4 Hg;void main(){Hg=vec4(0,1,0,1);}
#endif
#endif
