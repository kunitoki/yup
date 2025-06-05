#pragma once

#include "glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char glsl[] = R"===(#define za
#ifndef WB
#define WB __VERSION__
#endif
#define c vec2
#define Z vec3
#define a4 vec3
#define f vec4
#define g mediump float
#define G mediump vec2
#define A mediump vec3
#define i mediump vec4
#define U5 mediump mat3x3
#define V5 mediump mat2x3
#define c0 ivec2
#define h7 ivec4
#define N0 uvec2
#define M uvec4
#define a0 mediump uint
#define a5 bvec2
#define G7 bvec3
#define S mat2
#define d
#define k1(P1) out P1
#define i4(P1) inout P1
#ifdef GL_ANGLE_base_vertex_base_instance_shader_builtin
#extension GL_ANGLE_base_vertex_base_instance_shader_builtin:require
#endif
#ifdef KD
#extension GL_KHR_blend_equation_advanced:require
#endif
#if defined(DB)&&defined(BB)&&defined(GL_ES)
#ifdef GL_EXT_clip_cull_distance
#extension GL_EXT_clip_cull_distance:require
#elif defined(GL_ANGLE_clip_cull_distance)
#extension GL_ANGLE_clip_cull_distance:require
#endif
#endif
#if WB>=310
#define e5(e,a) layout(binding=e,std140)uniform a{
#else
#define e5(e,a) layout(std140)uniform a{
#endif
#define W5(a) }a;
#define U0(a)
#define i0(e,W,a) layout(location=e)in W a
#define V0
#define l0(H7,B,a,W)
#ifdef AB
#if WB>=310
#define H(e,W,a) layout(location=e)out W a
#else
#define H(e,W,a) out W a
#endif
#else
#if WB>=310
#define H(e,W,a) layout(location=e)in W a
#else
#define H(e,W,a) in W a
#endif
#endif
#define L2 flat
#define o1
#define p1
#ifdef CC
#define n0
#else
#ifdef GL_NV_shader_noperspective_interpolation
#extension GL_NV_shader_noperspective_interpolation:require
#define n0 noperspective
#else
#define n0
#endif
#endif
#ifdef AB
#define P2
#define Q2
#endif
#ifdef HB
#define R2
#define S2
#endif
#define p4
#define q4
#ifdef CC
#define I3(g0,e,a) layout(set=g0,binding=e)uniform highp utexture2D a
#define r4(g0,e,a) layout(set=g0,binding=e)uniform highp texture2D a
#define C2(g0,e,a) layout(set=g0,binding=e)uniform mediump texture2D a
#define y4(g0,e,a) layout(binding=e)uniform mediump texture2D a
#elif WB>=310
#define I3(g0,e,a) layout(binding=e)uniform highp usampler2D a
#define r4(g0,e,a) layout(binding=e)uniform highp sampler2D a
#define C2(g0,e,a) layout(binding=e)uniform mediump sampler2D a
#define y4(g0,e,a) layout(binding=e)uniform mediump sampler2D a
#else
#define I3(g0,e,a) uniform highp usampler2D a
#define r4(g0,e,a) uniform highp sampler2D a
#define C2(g0,e,a) uniform mediump sampler2D a
#define y4(g0,e,a) uniform mediump sampler2D a
#endif
#ifdef CC
#define P3(m2,a) layout(set=ab,binding=m2)uniform mediump sampler a;
#define G3(m2,a) layout(set=Y5,binding=m2)uniform mediump sampler a;
#define D4(a,p,l) texture(sampler2D(a,p),l)
#define T1(a,p,l,G0) textureLod(sampler2D(a,p),l,G0)
#else
#define P3(m2,a)
#define G3(m2,a)
#define D4(a,p,l) texture(a,l)
#define T1(a,p,l,G0) textureLod(a,l,G0)
#endif
#define o4(A0,p,l) D4(A0,p,l)
#define C7(A0,p,l,G0) T1(A0,p,l,G0)
#define l5(g0,e,a) y4(g0,e,a)
#define T5(a,p,m,w5,K7,G0) T1(a,p,c(m,K7),G0)
#define xe(g0,e,a) I3(g0,e,a)
#define n5
#define x1
#define d1(a,l) texelFetch(a,l,0)
#ifdef CC
#define p5(a,p,l,X2) textureGather(sampler2D(a,p),(l)*(X2))
#elif WB>=310
#define p5(a,p,l,X2) textureGather(a,(l)*(X2))
#else
#define p5(a,p,l,X2) E1(d1(a,c0(l)+c0(-1,0)).x,d1(a,c0(l)+c0(0,0)).x,d1(a,c0(l)+c0(0,-1)).x,d1(a,c0(l)+c0(-1,-1)).x)
#endif
#define E3
#define F3
#define w3
#define x3
#ifdef FE
#define g4(e,f1,a) I3(U2,e,a)
#define O3(e,f1,a) xe(U2,e,a)
#define h4(e,f1,a) r4(U2,e,a)
#define w0(a,v0) d1(a,c0((v0)&Oa,(v0)>>Na))
#define k4(a,v0) d1(a,c0((v0)&Oa,(v0)>>Na)).xy
#else
#ifdef GL_ARB_shader_storage_buffer_object
#extension GL_ARB_shader_storage_buffer_object:require
#endif
#define g4(e,f1,a) layout(std430,binding=e)readonly buffer f1{N0 V3[];}a
#define O3(e,f1,a) layout(std430,binding=e)readonly buffer f1{M V3[];}a
#define h4(e,f1,a) layout(std430,binding=e)readonly buffer f1{f V3[];}a
#define Jd(e,f1,a) layout(std430,binding=e)buffer f1{uint V3[];}a
#define w0(a,v0) a.V3[v0]
#define k4(a,v0) a.V3[v0]
#define Pd(a,v0) a.V3[v0]
#define c9(a,v0,m) atomicMax(a.V3[v0],m)
#define eb(a,v0,m) atomicAdd(a.V3[v0],m)
#endif
#ifdef _EXPORTED_PLS_IMPL_ANGLE
#extension GL_ANGLE_shader_pixel_local_storage:require
#define x2
#define M0(e,a) layout(binding=e,rgba8)uniform lowp pixelLocalANGLE a
#define Y0(e,a) layout(binding=e,r32ui)uniform highp upixelLocalANGLE a
#define y2
#define I0(h) pixelLocalLoadANGLE(h)
#define j1(h) pixelLocalLoadANGLE(h).x
#define T0(h,F) pixelLocalStoreANGLE(h,F)
#define l1(h,F) pixelLocalStoreANGLE(h,uvec4(F))
#define E2(h)
#define i2(h)
#define h2
#define j2
#endif
#ifdef GE
#extension GL_EXT_shader_pixel_local_storage:enable
#define x2 __pixel_localEXT z1{
#define M0(e,a) layout(rgba8)lowp vec4 a
#define Y0(e,a) layout(r32ui)highp uint a
#define y2 };
#define I0(h) h
#define j1(h) h
#define T0(h,F) h=(F)
#define l1(h,F) h=(F)
#define E2(h) h=h
#define i2(h) h=h
#define h2
#define j2
#endif
#ifdef HE
#extension GL_EXT_shader_framebuffer_fetch:require
#define x2
#define M0(e,a) layout(location=e)inout lowp vec4 a
#define Y0(e,a) layout(location=e)inout highp uvec4 a
#define y2
#define I0(h) h
#define j1(h) h.x
#define T0(h,F) h=(F)
#define l1(h,F) h.x=(F)
#define E2(h) T0(h,I0(h))
#define i2(h) l1(h,j1(h))
#define h2
#define j2
#endif
#ifdef IE
#ifdef GL_ARB_shader_image_load_store
#extension GL_ARB_shader_image_load_store:require
#endif
#if defined(GL_ARB_fragment_shader_interlock)
#extension GL_ARB_fragment_shader_interlock:require
#define h2 beginInvocationInterlockARB()
#define j2 endInvocationInterlockARB()
#elif defined(GL_INTEL_fragment_shader_ordering)
#extension GL_INTEL_fragment_shader_ordering:require
#define h2 beginFragmentShaderOrderingINTEL()
#define j2
#else
#define h2
#define j2
#endif
#define x2
#ifdef CC
#define M0(e,a) layout(set=a6,binding=e,rgba8)uniform lowp coherent image2D a
#define Y0(e,a) layout(set=a6,binding=e,r32ui)uniform highp coherent uimage2D a
#else
#define M0(e,a) layout(binding=e,rgba8)uniform lowp coherent image2D a
#define Y0(e,a) layout(binding=e,r32ui)uniform highp coherent uimage2D a
#endif
#define y2
#define I0(h) imageLoad(h,I)
#define j1(h) imageLoad(h,I).x
#define T0(h,F) imageStore(h,I,F)
#define l1(h,F) imageStore(h,I,uvec4(F))
#define E2(h)
#define i2(h)
#ifndef DD
#define DD
#endif
#endif
#ifdef JE
#define x2
#define O4(e,a) layout(input_attachment_index=e,binding=e,set=a6)uniform lowp subpassInput q6##a;
#define M0(e,a) O4(e,a);layout(location=e)out lowp vec4 a
#define Y0(e,a) layout(input_attachment_index=e,binding=e,set=a6)uniform highp usubpassInput q6##a;layout(location=e)out highp uvec4 a
#define y2
#define I0(h) subpassLoad(q6##h)
#define j1(h) subpassLoad(q6##h).x
#define T0(h,F) h=(F)
#define l1(h,F) h.x=(F)
#define E2(h) T0(h,subpassLoad(q6##h))
#define i2(h) l1(h,subpassLoad(q6##h).x)
#define h2
#define j2
#endif
#ifdef KE
#define x2
#define M0(e,a) layout(location=e)out lowp vec4 a
#define Y0(e,a) layout(location=e)out highp uvec4 a
#define y2
#define I0(h) vec4(0)
#define j1(h) 0u
#define T0(h,F) h=(F)
#define l1(h,F) h.x=(F)
#define E2(h) h=vec4(0)
#define i2(h) h.x=0u
#define h2
#define j2
#endif
#ifdef CC
#define gl_VertexID gl_VertexIndex
#endif
#ifdef OD
#ifdef CC
#define L7 gl_InstanceIndex
#else
#ifdef ED
uniform highp int ED;
#define L7 (gl_InstanceID+ED)
#else
#define L7 (gl_InstanceID+gl_BaseInstance)
#endif
#endif
#else
#define L7 0
#endif
#define q5
#define Y1
#define q1(a,f0,B,n,K) void main(){int n=gl_VertexID;int K=L7;
#define G6 q1
#define N4(a,a2,c2,v2,w2,n) q1(a,a2,c2,n,K)
#define L(a,W)
#define P(a)
#define N(a,W)
#define h1(g1) gl_Position=g1;}
#define e2(E4,a) layout(location=0)out E4 ye;void main()
#define f2(F) ye=F
#define y0 gl_FragCoord.xy
#define C5
#define Y2
#ifdef DD
#ifdef CC
#define f4(e,a) layout(set=a6,binding=e,r32ui)uniform highp coherent uimage2D a
#else
#define f4(e,a) layout(binding=e,r32ui)uniform highp coherent uimage2D a
#endif
#define m4(h) imageLoad(h,I).x
#define n4(h,F) imageStore(h,I,uvec4(F))
#define G5(h,m) imageAtomicMax(h,I,m)
#define H5(h,m) imageAtomicAdd(h,I,m)
#define y3 ,c0 I
#define G1 ,I
#define z2(a) void main(){c0 I=ivec2(floor(y0));
#define M2 }
#else
#define y3
#define G1
#define z2(a) void main()
#define M2
#endif
#define R4(a) z2(a)
#define A3(a) layout(location=0)out i K1;z2(a)
#define F5(a) layout(location=0)out i K1;z2(a)
#define P4 M2
#define C0(o,r) ((o)*(r))
precision highp float;precision highp int;
#if WB<310
d i unpackUnorm4x8(uint u){M A1=M(u&0xffu,(u>>8)&0xffu,(u>>16)&0xffu,u>>24);return f(A1)*(1./255.);}
#endif
#if WB>=310&&defined(AB)&&defined(DB)&&defined(BB)
out gl_PerVertex{float gl_ClipDistance[4];f gl_Position;};
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive