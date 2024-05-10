#ifdef VERTEX
void main(){gl_Position=vec4(mix(vec2(-1,1),vec2(1,-1),equal(gl_VertexID&ivec2(1,2),ivec2(0))),0,1);}
#endif
#ifdef FRAGMENT
#extension GL_EXT_shader_pixel_local_storage:enable
#extension GL_ARM_shader_framebuffer_fetch:enable
#extension GL_EXT_shader_framebuffer_fetch:enable
#ifdef CLEAR_COLOR
#if __VERSION__>=310
layout(binding=0,std140)uniform db{uniform highp vec4 qa;}ra;
#else
uniform mediump vec4 WC;
#endif
#endif
#ifdef GL_EXT_shader_pixel_local_storage
#ifdef STORE_COLOR
__pixel_local_inEXT H0
#else
__pixel_local_outEXT H0
#endif
{layout(rgba8)mediump vec4 c0;layout(r32ui)highp uint Q0;layout(rgba8)mediump vec4 w2;
#ifdef ENABLE_CLIPPING
layout(r32ui)highp uint w0;
#endif
};
#ifndef GL_ARM_shader_framebuffer_fetch
#ifdef LOAD_COLOR
layout(location=0)inout mediump vec4 l6;
#endif
#endif
#ifdef STORE_COLOR
layout(location=0)out mediump vec4 l6;
#endif
void main(){
#ifdef CLEAR_COLOR
#if __VERSION__>=310
c0=ra.qa;
#else
c0=WC;
#endif
#endif
#ifdef LOAD_COLOR
#ifdef GL_ARM_shader_framebuffer_fetch
c0=gl_LastFragColorARM;
#else
c0=l6;
#endif
#endif
#ifdef CLEAR_COVERAGE
Q0=0u;
#endif
#ifdef CLEAR_CLIP
w0=0u;
#endif
#ifdef STORE_COLOR
l6=c0;
#endif
}
#else
layout(location=0)out mediump vec4 sa;void main(){sa=vec4(0,1,0,1);}
#endif
#endif
