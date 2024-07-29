#ifdef VERTEX
void main(){gl_Position=vec4(mix(vec2(-1,1),vec2(1,-1),equal(gl_VertexID&ivec2(1,2),ivec2(0))),0,1);}
#endif
#ifdef FRAGMENT
#extension GL_EXT_shader_pixel_local_storage:enable
#extension GL_ARM_shader_framebuffer_fetch:enable
#extension GL_EXT_shader_framebuffer_fetch:enable
#ifdef CLEAR_COLOR
#if __VERSION__>=310
layout(binding=0,std140)uniform mb{uniform highp vec4 za;}Aa;
#else
uniform mediump vec4 XC;
#endif
#endif
#ifdef GL_EXT_shader_pixel_local_storage
#ifdef STORE_COLOR
__pixel_local_inEXT L0
#else
__pixel_local_outEXT L0
#endif
{layout(rgba8)mediump vec4 e0;layout(r32ui)highp uint Y0;layout(rgba8)mediump vec4 y2;
#ifdef ENABLE_CLIPPING
layout(r32ui)highp uint A0;
#endif
};
#ifndef GL_ARM_shader_framebuffer_fetch
#ifdef LOAD_COLOR
layout(location=0)inout mediump vec4 w6;
#endif
#endif
#ifdef STORE_COLOR
layout(location=0)out mediump vec4 w6;
#endif
void main(){
#ifdef CLEAR_COLOR
#if __VERSION__>=310
e0=Aa.za;
#else
e0=XC;
#endif
#endif
#ifdef LOAD_COLOR
#ifdef GL_ARM_shader_framebuffer_fetch
e0=gl_LastFragColorARM;
#else
e0=w6;
#endif
#endif
#ifdef CLEAR_COVERAGE
Y0=0u;
#endif
#ifdef CLEAR_CLIP
A0=0u;
#endif
#ifdef STORE_COLOR
w6=e0;
#endif
}
#else
layout(location=0)out mediump vec4 Ba;void main(){Ba=vec4(0,1,0,1);}
#endif
#endif
