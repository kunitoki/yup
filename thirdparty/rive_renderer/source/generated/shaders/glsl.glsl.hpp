#pragma once

#include "glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char glsl[] = R"===(#define F7
#ifndef UB
#define UB __VERSION__
#endif
#define f vec2
#define Q vec3
#define I3 vec3
#define g vec4
#define h mediump float
#define r mediump vec2
#define k mediump vec3
#define i mediump vec4
#define w4 mediump mat3x3
#define x4 mediump mat2x3
#define e0 ivec2
#define g5 ivec4
#define J4 mediump int
#define n1 uvec2
#define G uvec4
#define M mediump uint
#define I mat2
#define d
#define j2(f1) out f1
#define l4(f1) inout f1
#ifdef GL_ANGLE_base_vertex_base_instance_shader_builtin
#extension GL_ANGLE_base_vertex_base_instance_shader_builtin:require
#endif
#ifdef ZC
#extension GL_KHR_blend_equation_advanced:require
#endif
#if defined(CB)&&defined(U)&&defined(GL_ES)
#ifdef GL_EXT_clip_cull_distance
#extension GL_EXT_clip_cull_distance:require
#elif defined(GL_ANGLE_clip_cull_distance)
#extension GL_ANGLE_clip_cull_distance:require
#endif
#endif
#if UB>=310
#define Y3(c,a) layout(binding=c,std140)uniform a{
#else
#define Y3(c,a) layout(std140)uniform a{
#endif
#define y4(a) }a;
#define Y0(a)
#define a0(c,D,a) layout(location=c)in D a
#define Z0
#define f0(w5,q,a,D)
#ifdef AB
#if UB>=310
#define W(c,D,a) layout(location=c)out D a
#else
#define W(c,D,a) out D a
#endif
#else
#if UB>=310
#define W(c,D,a) layout(location=c)in D a
#else
#define W(c,D,a) in D a
#endif
#endif
#define H3 flat
#define H1
#define I1
#ifdef OB
#define w0
#else
#ifdef GL_NV_shader_noperspective_interpolation
#extension GL_NV_shader_noperspective_interpolation:require
#define w0 noperspective
#else
#define w0
#endif
#endif
#ifdef AB
#define R1
#define S1
#endif
#ifdef EB
#define H2
#define I2
#endif
#ifdef OB
#define R2(v0,c,a) layout(set=v0,binding=c)uniform highp utexture2D a
#define e4(v0,c,a) layout(set=v0,binding=c)uniform highp texture2D a
#define v1(v0,c,a) layout(set=v0,binding=c)uniform mediump texture2D a
#elif UB>=310
#define R2(v0,c,a) layout(binding=c)uniform highp usampler2D a
#define e4(v0,c,a) layout(binding=c)uniform highp sampler2D a
#define v1(v0,c,a) layout(binding=c)uniform mediump sampler2D a
#else
#define R2(v0,c,a) uniform highp usampler2D a
#define e4(v0,c,a) uniform highp sampler2D a
#define v1(v0,c,a) uniform mediump sampler2D a
#endif
#define Ga(v0,c,a) R2(v0,c,a)
#ifdef OB
#define K3(a2,a) layout(set=g8,binding=a2)uniform mediump sampler a;
#define c3(a2,a) layout(set=g8,binding=a2)uniform mediump sampler a;
#define l3(a,i0,K) texture(sampler2D(a,i0),K)
#define h3(a,i0,K,v2) textureLod(sampler2D(a,i0),K,v2)
#else
#define K3(a2,a)
#define c3(a2,a)
#define l3(a,i0,K) texture(a,K)
#define h3(a,i0,K,v2) textureLod(a,K,v2)
#endif
#define N1(a,K) texelFetch(a,K,0)
#define f2
#define g2
#define M3
#define P3
#ifdef TD
#define N3(c,Q0,a) R2(p2,c,a)
#define S2(c,Q0,a) Ga(p2,c,a)
#define O3(c,Q0,a) e4(p2,c,a)
#define E0(a,o0) N1(a,e0((o0)&U7,(o0)>>T7))
#define Q3(a,o0) N1(a,e0((o0)&U7,(o0)>>T7)).xy
#else
#ifdef GL_ARB_shader_storage_buffer_object
#extension GL_ARB_shader_storage_buffer_object:require
#endif
#define N3(c,Q0,a) layout(std430,binding=c)readonly buffer Q0{n1 y5[];}a
#define S2(c,Q0,a) layout(std430,binding=c)readonly buffer Q0{G y5[];}a
#define O3(c,Q0,a) layout(std430,binding=c)readonly buffer Q0{g y5[];}a
#define E0(a,o0) a.y5[o0]
#define Q3(a,o0) a.y5[o0]
#endif
#ifdef _EXPORTED_PLS_IMPL_ANGLE
#extension GL_ANGLE_shader_pixel_local_storage:require
#define J1
#define A0(c,a) layout(binding=c,rgba8)uniform lowp pixelLocalANGLE a
#define C0(c,a) layout(binding=c,r32ui)uniform highp upixelLocalANGLE a
#define K1
#define r0(e) pixelLocalLoadANGLE(e)
#define I0(e) pixelLocalLoadANGLE(e).x
#define y0(e,o) pixelLocalStoreANGLE(e,o)
#define K0(e,o) pixelLocalStoreANGLE(e,uvec4(o))
#define O1(e)
#define X1(e)
#define w1
#define x1
#endif
#ifdef UD
#extension GL_EXT_shader_pixel_local_storage:enable
#extension GL_ARM_shader_framebuffer_fetch:enable
#extension GL_EXT_shader_framebuffer_fetch:enable
#define J1 __pixel_localEXT R0{
#define A0(c,a) layout(rgba8)lowp vec4 a
#define C0(c,a) layout(r32ui)highp uint a
#define K1 };
#define r0(e) e
#define I0(e) e
#define y0(e,o) e=(o)
#define K0(e,o) e=(o)
#define O1(e)
#define X1(e)
#define w1
#define x1
#endif
#ifdef VD
#extension GL_EXT_shader_framebuffer_fetch:require
#define J1
#define A0(c,a) layout(location=c)inout lowp vec4 a
#define C0(c,a) layout(location=c)inout highp uvec4 a
#define K1
#define r0(e) e
#define I0(e) e.x
#define y0(e,o) e=(o)
#define K0(e,o) e.x=(o)
#define O1(e) y0(e,r0(e))
#define X1(e) K0(e,I0(e))
#define w1
#define x1
#endif
#ifdef WD
#ifdef GL_ARB_shader_image_load_store
#extension GL_ARB_shader_image_load_store:require
#endif
#if defined(GL_ARB_fragment_shader_interlock)
#extension GL_ARB_fragment_shader_interlock:require
#define w1 beginInvocationInterlockARB()
#define x1 endInvocationInterlockARB()
#elif defined(GL_INTEL_fragment_shader_ordering)
#extension GL_INTEL_fragment_shader_ordering:require
#define w1 beginFragmentShaderOrderingINTEL()
#define x1
#else
#define w1
#define x1
#endif
#define J1
#ifdef OB
#define A0(c,a) layout(set=B4,binding=c,rgba8)uniform lowp coherent image2D a
#define C0(c,a) layout(set=B4,binding=c,r32ui)uniform highp coherent uimage2D a
#else
#define A0(c,a) layout(binding=c,rgba8)uniform lowp coherent image2D a
#define C0(c,a) layout(binding=c,r32ui)uniform highp coherent uimage2D a
#endif
#define K1
#define r0(e) imageLoad(e,v)
#define I0(e) imageLoad(e,v).x
#define y0(e,o) imageStore(e,v,o)
#define K0(e,o) imageStore(e,v,uvec4(o))
#define O1(e)
#define X1(e)
#ifndef TC
#define TC
#endif
#endif
#ifdef XD
#define J1
#define o7(c,a) layout(input_attachment_index=c,binding=c,set=B4)uniform lowp subpassInput K4##a;
#define A0(c,a) o7(c,a);layout(location=c)out lowp vec4 a
#define C0(c,a) layout(input_attachment_index=c,binding=c,set=B4)uniform highp usubpassInput K4##a;layout(location=c)out highp uvec4 a
#define K1
#define r0(e) subpassLoad(K4##e)
#define I0(e) subpassLoad(K4##e).x
#define y0(e,o) e=(o)
#define K0(e,o) e.x=(o)
#define O1(e) y0(e,subpassLoad(K4##e))
#define X1(e) K0(e,subpassLoad(K4##e).x)
#define w1
#define x1
#endif
#ifdef YD
#define J1
#define A0(c,a) layout(location=c)out lowp vec4 a
#define C0(c,a) layout(location=c)out highp uvec4 a
#define K1
#define r0(e) vec4(0)
#define I0(e) 0u
#define y0(e,o) e=(o)
#define K0(e,o) e.x=(o)
#define O1(e) e=vec4(1,0,1,1)
#define X1(e) e.x=0u
#define w1
#define x1
#endif
#ifdef OB
#define gl_VertexID gl_VertexIndex
#endif
#ifdef FD
#ifdef OB
#define z5 gl_InstanceIndex
#else
#ifdef ZD
uniform int SPIRV_Cross_BaseInstance;
#define z5 (gl_InstanceID+SPIRV_Cross_BaseInstance)
#else
#define z5 (gl_InstanceID+gl_BaseInstance)
#endif
#endif
#else
#define z5 0
#endif
#define T2
#define Y2
#define h1(a,V,q,n,H) void main(){int n=gl_VertexID;int H=z5;
#define T4 h1
#define j4(a,T1,U1,h2,i2,n) h1(a,T1,U1,n,H)
#define c0(a,D)
#define d0(a)
#define Y(a,D)
#define i1(F0) gl_Position=F0;}
#define q2(x3,a) layout(location=0)out x3 Ha;void main()
#define r2(o) Ha=o
#define h0 gl_FragCoord.xy
#define m4
#define O2
#ifdef TC
#ifdef OB
#define d3(c,a) layout(set=B4,binding=c,r32ui)uniform highp coherent uimage2D a
#else
#define d3(c,a) layout(binding=c,r32ui)uniform highp coherent uimage2D a
#endif
#define i3(e) imageLoad(e,v).x
#define j3(e,o) imageStore(e,v,uvec4(o))
#define p4(e,n0) imageAtomicMax(e,v,n0)
#define q4(e,n0) imageAtomicAdd(e,v,n0)
#define L2 ,e0 v
#define X0 ,v
#define M1(a) void main(){e0 v=ivec2(floor(h0));
#define k2 }
#else
#define L2
#define X0
#define M1(a) void main()
#define k2
#endif
#define U3(a) M1(a)
#define N2(a) layout(location=0)out i e1;M1(a)
#define o4(a) layout(location=0)out i e1;M1(a)
#define S3 k2
#define m0(L,O) ((L)*(O))
#ifndef OB
#define ma
#endif
precision highp float;precision highp int;
#if UB<310
d i unpackUnorm4x8(uint u){G S0=G(u&0xffu,(u>>8)&0xffu,(u>>16)&0xffu,u>>24);return g(S0)*(1./255.);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive