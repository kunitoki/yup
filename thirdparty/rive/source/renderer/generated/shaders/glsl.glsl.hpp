#pragma once

#include "glsl.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char glsl[] = R"===(#define cc
#ifndef EC
#define EC __VERSION__
#endif
#define c vec2
#define V vec3
#define L3 vec3
#define g vec4
#define d mediump float
#define D mediump vec2
#define r mediump vec3
#define i mediump vec4
#define V6 mediump mat3x3
#define W6 mediump mat2x3
#define h5 mediump mat4x4
#define U ivec2
#define Z5 ivec4
#define W0 uvec2
#define Q uvec4
#define X mediump uint
#define E4 bvec2
#define n6 bvec3
#define w7 bvec4
#define Z mat2
#define e
#define e1(g2) out g2
#define T4(g2) inout g2
#ifdef GL_ANGLE_base_vertex_base_instance_shader_builtin
#extension GL_ANGLE_base_vertex_base_instance_shader_builtin:require
#endif
#ifdef ZD
#extension GL_KHR_blend_equation_advanced:require
#endif
#ifdef MD
#extension GL_EXT_shader_framebuffer_fetch:require
#elif defined(ND)
#extension GL_EXT_shader_pixel_local_storage:require
#elif defined(EXPORTED_ATLAS_RENDER_TARGET_R32UI_PLS_ANGLE)
#extension GL_ANGLE_shader_pixel_local_storage:require
#elif defined(OD)
#ifdef GL_ARB_shader_image_load_store
#extension GL_ARB_shader_image_load_store:require
#endif
#ifdef GL_OES_shader_image_atomic
#extension GL_OES_shader_image_atomic:require
#endif
#endif
#if defined(BB)&&defined(AB)&&defined(GL_ES)&&!defined(EE)
#ifdef GL_EXT_clip_cull_distance
#extension GL_EXT_clip_cull_distance:require
#elif defined(GL_ANGLE_clip_cull_distance)
#extension GL_ANGLE_clip_cull_distance:require
#endif
#endif
#if EC>=310
#define m6(f,a) layout(binding=f,std140)uniform a{
#else
#define m6(f,a) layout(std140)uniform a{
#endif
#define v7(a) }a;
#define A1(a)
#define p0(f,W,a) layout(location=f)in W a
#define B1
#define q0(O8,G,a,W)
#ifdef CB
#if EC>=310
#define c0(f,W,a) layout(location=f)out W a
#else
#define c0(f,W,a) out W a
#endif
#else
#if EC>=310
#define c0(f,W,a) layout(location=f)in W a
#else
#define c0(f,W,a) in W a
#endif
#endif
#define R4 flat
#define h2
#define a2
#ifdef VB
#define J0
#else
#ifdef GL_NV_shader_noperspective_interpolation
#extension GL_NV_shader_noperspective_interpolation:require
#define J0 noperspective
#else
#define J0
#endif
#endif
#ifdef CB
#define R3
#define S3
#endif
#ifdef FB
#define B3
#define C3
#endif
#define a5
#define c5
#ifdef VB
#define D4(M,f,a) layout(set=M,binding=f)uniform highp utexture2D a
#define e5(M,f,a) layout(set=M,binding=f)uniform highp texture2D a
#define X2(M,f,a) layout(set=M,binding=f)uniform mediump texture2D a
#define k5(M,f,a) layout(binding=f)uniform mediump texture2D a
#if defined(FB)&&defined(BB)
#endif
#elif EC>=310
#define D4(M,f,a) layout(binding=f)uniform highp usampler2D a
#define e5(M,f,a) layout(binding=f)uniform highp sampler2D a
#define X2(M,f,a) layout(binding=f)uniform mediump sampler2D a
#define k5(M,f,a) layout(binding=f)uniform mediump sampler2D a
#else
#define D4(M,f,a) uniform highp usampler2D a
#define e5(M,f,a) uniform highp sampler2D a
#define X2(M,f,a) uniform mediump sampler2D a
#define k5(M,f,a) uniform mediump sampler2D a
#endif
#ifdef VB
#define o6(M,f,a) layout(set=M,binding=f)uniform mediump sampler a;
#ifdef BF
#define X3(x7,a) layout(set=Af,binding=x7)uniform mediump sampler a;
#define U3(a) o6(Z4,zf,a)
#else
#define X3(x7,a) layout(set=a3,binding=x7)uniform mediump sampler a;
#define U3(a) o6(Z4,T3,a)
#endif
#define r5(a,p,l) texture(sampler2D(a,p),l)
#define m2(a,p,l,X0) textureLod(sampler2D(a,p),l,X0)
#define v5(a,p,l,P1) texture(sampler2D(a,p),l,P1)
#if defined(FB)&&defined(BB)
#extension GL_OES_sample_variables:require
#endif
#else
#define X3(x7,a)
#define o6(M,f,a)
#define U3(a)
#define r5(a,p,l) texture(a,l)
#define m2(a,p,l,X0) textureLod(a,l,X0)
#define v5(a,p,l,P1) texture(a,l,P1)
#endif
#define g8(h0,p,l) r5(h0,p,l)
#define Q6(h0,p,l,X0) m2(h0,p,l,X0)
#define y7(h0,p,l,P1) v5(h0,p,l,P1)
#define f6(M,f,a) k5(M,f,a)
#define U6(a,p,q,p6,Q8,X0) m2(a,p,c(q,Q8),X0)
#define wg(M,f,a) D4(M,f,a)
#define F3
#define g1
#define v1(a,l) texelFetch(a,l,0)
#ifdef VB
#elif EC>=310
#else
#endif
#define A4
#define B4
#define N3
#define O3
#ifdef CF
#define J5(f,y1,a) D4(a3,f,a)
#define F4(f,y1,a) wg(a3,f,a)
#define K5(f,y1,a) e5(a3,f,a)
#define P0(a,y0) v1(a,U((y0)&sc,(y0)>>rc))
#define L5(a,y0) v1(a,U((y0)&sc,(y0)>>rc)).xy
#else
#ifdef GL_ARB_shader_storage_buffer_object
#extension GL_ARB_shader_storage_buffer_object:require
#endif
#define J5(f,y1,a) layout(std430,binding=f)readonly buffer y1{W0 X1[];}a
#define F4(f,y1,a) layout(std430,binding=f)readonly buffer y1{Q X1[];}a
#define K5(f,y1,a) layout(std430,binding=f)readonly buffer y1{g X1[];}a
#define Ea(f,y1,a) layout(std430,binding=f)buffer y1{uint X1[];}a
#define P0(a,y0) a.X1[y0]
#define L5(a,y0) a.X1[y0]
#define pd(a,y0) a.X1[y0]
#define A7(a,y0,q) atomicMax(a.X1[y0],q)
#define Fa(a,y0,q) atomicAdd(a.X1[y0],q)
#define xg(a,y0,q) atomicOr(a.X1[y0],q)
#endif
#ifdef PD
#define m1(a) void main(){U E=ivec2(floor(S));int C0=int(L8(uvec2(E),(k.q5+(la-1u))&~(la-1u)));
#define U1 }
#define P3 ,int C0
#define M1 ,C0
#ifdef FE
#define E2(f,a) layout(std430,set=E3,binding=f)buffer a##qd{uint X1[];}a
#elif defined(VB)
#define E2(f,a) layout(std430,set=E3,binding=f)coherent buffer a##qd{uint X1[];}a
#else
#define E2(f,a) layout(std430,binding=f)coherent buffer a##qd{uint X1[];}a
#endif
#define Ga E2
#define T2(h) h.X1[C0]
#define U2(h,C) h.X1[C0]=C
#define Ha(h) unpackUnorm4x8(T2(h))
#define Ia(h,C) U2(h,packUnorm4x8(C))
#define W4(h,q) atomicMax(h.X1[C0],q)
#define X4(h,q) atomicAdd(h.X1[C0],q)
#elif defined(QD)||defined(DF)
#ifdef GL_ARB_shader_image_load_store
#extension GL_ARB_shader_image_load_store:require
#endif
#define m1(a) void main(){U E=ivec2(floor(S));
#define U1 }
#define P3 ,U E
#define M1 ,E
#ifdef VB
#define Ga(f,a) layout(set=E3,binding=f,rgba8)uniform mediump coherent image2D a
#define E2(f,a) layout(set=E3,binding=f,r32ui)uniform highp coherent uimage2D a
#define Ja(f,a) layout(set=E3,binding=f,rgb10_a2)uniform mediump coherent image2D a
#else
#define Ga(f,a) layout(binding=f,rgba8)uniform mediump coherent image2D a
#define E2(f,a) layout(binding=f,r32ui)uniform highp coherent uimage2D a
#define Ja(f,a) layout(binding=f,rgb10_a2)uniform mediump coherent image2D a;
#endif
#define T2(h) imageLoad(h,E).x
#define U2(h,C) imageStore(h,E,uvec4(C))
#define Ha(h) imageLoad(h,E)
#define Ia(h,C) imageStore(h,E,C)
#define W4(h,q) imageAtomicMax(h,E,q)
#define X4(h,q) imageAtomicAdd(h,E,q)
#else
#define m1(a) void main()
#define U1
#define P3
#define M1
#endif
#ifdef EXPORTED_PLS_IMPL_ANGLE
#extension GL_ANGLE_shader_pixel_local_storage:require
#define J1
#define r0(f,a) layout(binding=f,rgba8)uniform mediump pixelLocalANGLE a
#define k1(f,a) layout(binding=f,r32ui)uniform highp upixelLocalANGLE a
#define K1
#define H0(h) pixelLocalLoadANGLE(h)
#define d1(h) pixelLocalLoadANGLE(h).x
#define v0(h,C) pixelLocalStoreANGLE(h,C)
#define f1(h,C) pixelLocalStoreANGLE(h,uvec4(C))
#define r2(h)
#define Y1(h)
#define v2
#define w2
#endif
#ifdef EF
#ifdef K
#extension GL_EXT_shader_pixel_local_storage2:require
#else
#extension GL_EXT_shader_pixel_local_storage:require
#endif
#define J1 __pixel_localEXT n1{
#define r0(f,a) layout(rgba8)mediump vec4 a
#define Ka(f,a) layout(rgb10_a2)mediump vec4 a
#define k1(f,a) layout(r32ui)highp uint a
#define K1 };
#define H0(h) h
#define d1(h) h
#define v0(h,C) h=(C)
#define f1(h,C) h=(C)
#define r2(h) h=h
#define Y1(h) h=h
#define v2
#define w2
#ifdef K
#define o2(a) layout(location=0,rgba8)out i l1;m1(a)
#define r4(a) layout(location=0,rgba8)out i l1;m1(a)
#endif
#endif
#if defined(QD)||defined(PD)
#define J1
#define K1
#define r0 Ga
#define k1 E2
#define Ka Ja
#define H0 Ha
#define v0 Ia
#define d1 T2
#define f1 U2
#define r2(h)
#define Y1(h)
#if defined(GL_ARB_fragment_shader_interlock)
#extension GL_ARB_fragment_shader_interlock:require
#define v2 beginInvocationInterlockARB()
#define w2 endInvocationInterlockARB()
#elif defined(GL_INTEL_fragment_shader_ordering)
#extension GL_INTEL_fragment_shader_ordering:require
#define v2 beginFragmentShaderOrderingINTEL()
#define w2
#else
#define v2
#define w2
#endif
#endif
#ifdef FF
#define J1
#define n4(f,a) layout(input_attachment_index=f,binding=f,set=E3)uniform mediump subpassInput B7##a
#define rd(f,a) layout(location=f)out mediump vec4 a
#define r0(f,a) n4(f,a);rd(f,a)
#define k1(f,a) layout(input_attachment_index=f,binding=f,set=E3)uniform highp usubpassInput B7##a;layout(location=f)out highp uvec4 a
#define K1
#define H0(h) subpassLoad(B7##h)
#define d1(h) subpassLoad(B7##h).x
#define v0(h,C) h=(C)
#define f1(h,C) h.x=(C)
#define r2(h) v0(h,subpassLoad(B7##h))
#define Y1(h) f1(h,subpassLoad(B7##h).x)
#define v2
#define w2
#endif
#ifdef GF
#define J1
#define r0(f,a) layout(location=f)out mediump vec4 a
#define k1(f,a) layout(location=f)out highp uvec4 a
#define K1
#define H0(h) vec4(0)
#define d1(h) 0u
#define v0(h,C) h=(C)
#define f1(h,C) h.x=(C)
#define r2(h) h=vec4(0)
#define Y1(h) h.x=0u
#define v2
#define w2
#endif
#ifndef n4
#define n4 r0
#endif
#ifdef VB
#define gl_VertexID gl_VertexIndex
#endif
#ifdef GE
#ifdef VB
#define R8 gl_InstanceIndex
#else
#ifdef RD
uniform highp int RD;
#define R8 (gl_InstanceID+RD)
#else
#define R8 (gl_InstanceID+gl_BaseInstance)
#endif
#endif
#else
#define R8 0
#endif
#define i6
#define v3
#define a7
#define w5
#define C1(a,a0,G,v,T) void main(){int v=gl_VertexID;int T=R8;
#define S7 C1
#define E6(a,h3,i3,w3,x3,v) C1(a,h3,i3,v,T)
#define Y(a,W)
#define k0(a)
#define B(a,W)
#define D1(L0) gl_Position=L0;}
#define Y2(Q1,a) layout(location=0)out Q1 yg;void main()
#define q6 Y2
#define r6 gl_FrontFacing
#define G2(C) yg=C
#define S gl_FragCoord.xy
#define G6
#define S2
#if defined(QD)||defined(PD)
#define sd(C7,h,C) if(!(C7)){v0(h,C);}
#define td(C7,h,C) if(!(C7)){f1(h,C);}
#else
#define sd(C7,h,C) v0(h,C);
#define td(C7,h,C) f1(h,C);
#endif
#define O5(a) m1(a)
#ifndef o2
#define o2(a) layout(location=0)out i l1;m1(a)
#endif
#ifndef r4
#define r4(a) layout(location=0)out i l1;m1(a)
#endif
#define l3 U1
#if defined(VB)&&!defined(FE)
#define g7(a) layout(input_attachment_index=0,binding=Q2,set=E3)uniform mediump subpassInputMS a
#define S8(a) oc(mat4(subpassLoad(a,0),subpassLoad(a,1),subpassLoad(a,2),subpassLoad(a,3)),gl_SampleMaskIn[0])
#else
#define g7(a) X2(a3,yf,a)
#define S8(a) texelFetch(a,ivec2(floor(S.xy)),0)
#endif
#define Z0(A,F) ((A)*(F))
precision highp float;precision highp int;
#if EC<310
e i zg(uint u){Q R1=Q(u&0xffu,(u>>8)&0xffu,(u>>16)&0xffu,u>>24);return g(R1)*(1./255.);}
#define unpackUnorm4x8 zg
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive