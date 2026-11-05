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
layout(binding=0,std140) uniform il{uniform highp vec4 uj;}vj;
#else
uniform mediump vec4 FF;
#endif
#endif
#ifdef GL_EXT_shader_pixel_local_storage
#ifdef STORE_COLOR
__pixel_local_inEXT V1
#else
__pixel_local_outEXT V1
#endif
{layout(rgba8) mediump vec4 q0;layout(r32ui) highp uint o0;layout(rgba8) mediump vec4 T4;layout(r32ui) highp uint B8;};
#ifndef GL_ARM_shader_framebuffer_fetch
#ifdef LOAD_COLOR
layout(location=0) inout mediump vec4 Fc;
#endif
#endif
#ifdef STORE_COLOR
layout(location=0) out mediump vec4 Fc;
#endif
void main(){
#ifdef CLEAR_COLOR
#if __VERSION__>=310
q0=vj.uj;
#else
q0=FF;
#endif
#endif
#ifdef LOAD_COLOR
#ifdef GL_ARM_shader_framebuffer_fetch
q0=gl_LastFragColorARM;
#else
q0=Fc;
#endif
#endif
#ifdef CLEAR_COVERAGE
B8=0u;
#endif
#ifdef CLEAR_CLIP
o0=0u;
#endif
#ifdef STORE_COLOR
Fc=q0;
#endif
}
#else
layout(location=0) out mediump vec4 wj;void main(){wj=vec4(0,1,0,1);}
#endif
#endif
