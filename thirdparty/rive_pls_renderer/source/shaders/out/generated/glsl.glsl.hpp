#pragma once

#include "glsl.exports.h"

namespace rive {
namespace pls {
namespace glsl {
const char glsl[] = R"===(#define ha
#ifndef UB
#define UB __VERSION__
#endif
#define d vec2
#define i0 vec3
#define M3 vec3
#define g vec4
#define h mediump float
#define F mediump vec2
#define q mediump vec3
#define j mediump vec4
#define V float
#define l1 vec2
#define c0 vec3
#define j0 vec4
#define o0 ivec2
#define P5 ivec4
#define Z4 mediump int
#define B0 uvec2
#define W uvec4
#define N mediump uint
#define J0 uint
#define B mat2
#define r5 mat3x4
#define i
#define A2(n4) out n4
#ifdef GL_ANGLE_base_vertex_base_instance_shader_builtin
#extension GL_ANGLE_base_vertex_base_instance_shader_builtin:require
#endif
#ifdef DC
#extension GL_ARB_bindless_texture:require
#endif
#ifdef OC
#extension GL_KHR_blend_equation_advanced:require
#endif
#ifdef DB
#ifdef AB
#ifdef GL_EXT_clip_cull_distance
#extension GL_EXT_clip_cull_distance:require
#elif defined(GL_ANGLE_clip_cull_distance)
#extension GL_ANGLE_clip_cull_distance:require
#endif
#endif
#endif
#if UB>=310
#define d4(e,a) layout(binding=e,std140)uniform a{
#else
#define d4(e,a) layout(std140)uniform a{
#endif
#define N4(a) }a;
#define U0(a)
#define v0(e,I,a) layout(location=e)in I a
#define V0
#define z0(k6,v,a,I)
#ifdef Y
#if UB>=310
#define K(e,I,a) layout(location=e)out I a
#else
#define K(e,I,a) out I a
#endif
#else
#if UB>=310
#define K(e,I,a) layout(location=e)in I a
#else
#define K(e,I,a) in I a
#endif
#endif
#define l6 flat
#define D1
#define E1
#ifdef FB
#define n0
#else
#ifdef GL_NV_shader_noperspective_interpolation
#extension GL_NV_shader_noperspective_interpolation:require
#define n0 noperspective
#else
#define n0
#endif
#endif
#ifdef Y
#define Q1
#define R1
#endif
#ifdef HB
#define K2
#define L2
#endif
#ifdef FB
#define V2(x0,e,a) layout(set=x0,binding=e)uniform highp utexture2D a
#define o4(x0,e,a) layout(set=x0,binding=e)uniform highp texture2D a
#define z1(x0,e,a) layout(set=x0,binding=e)uniform mediump texture2D a
#elif UB>=310
#define V2(x0,e,a) layout(binding=e)uniform highp usampler2D a
#define o4(x0,e,a) layout(binding=e)uniform highp sampler2D a
#define z1(x0,e,a) layout(binding=e)uniform mediump sampler2D a
#else
#define V2(x0,e,a) uniform highp usampler2D a
#define o4(x0,e,a) uniform highp sampler2D a
#define z1(x0,e,a) uniform mediump sampler2D a
#endif
#define ja(x0,e,a) V2(x0,e,a)
#ifdef FB
#define O3(g2,a) layout(set=H7,binding=g2)uniform mediump sampler a;
#define q3(g2,a) layout(set=H7,binding=g2)uniform mediump sampler a;
#define R2(a,r0,H) texture(sampler2D(a,r0),H)
#define W3(a,r0,H,d3) textureLod(sampler2D(a,r0),H,d3)
#define V3(a,r0,H,e3,f3) textureGrad(sampler2D(a,r0),H,e3,f3)
#else
#define O3(g2,a)
#define q3(g2,a)
#define R2(a,r0,H) texture(a,H)
#define W3(a,r0,H,d3) textureLod(a,H,d3)
#define V3(a,r0,H,e3,f3) textureGrad(a,H,e3,f3)
#endif
#define K1(a,H) texelFetch(a,H,0)
#define Y1
#define Z1
#define Q3
#define T3
#ifdef ID
#define R3(e,d1,a) V2(d2,e,a)
#define W2(e,d1,a) ja(d2,e,a)
#define S3(e,d1,a) o4(d2,e,a)
#define C0(a,D0) K1(a,o0((D0)&w7,(D0)>>v7))
#define v2(a,D0) K1(a,o0((D0)&w7,(D0)>>v7)).xy
#else
#ifdef GL_ARB_shader_storage_buffer_object
#extension GL_ARB_shader_storage_buffer_object:require
#endif
#define R3(e,d1,a) layout(std430,binding=e)readonly buffer d1{B0 c5[];}a
#define W2(e,d1,a) layout(std430,binding=e)readonly buffer d1{W c5[];}a
#define S3(e,d1,a) layout(std430,binding=e)readonly buffer d1{g c5[];}a
#define C0(a,D0) a.c5[D0]
#define v2(a,D0) a.c5[D0]
#endif
#ifdef _EXPORTED_PLS_IMPL_ANGLE
#extension GL_ANGLE_shader_pixel_local_storage:require
#define G1
#define F0(e,a) layout(binding=e,rgba8)uniform lowp pixelLocalANGLE a
#define G0(e,a) layout(binding=e,r32ui)uniform highp upixelLocalANGLE a
#define H1
#define I0(c) pixelLocalLoadANGLE(c)
#define H0(c) pixelLocalLoadANGLE(c).x
#define w0(c,m) pixelLocalStoreANGLE(c,m)
#define P0(c,m) pixelLocalStoreANGLE(c,uvec4(m))
#define n1(c)
#define A1(c)
#define p1
#define q1
#endif
#ifdef JD
#extension GL_EXT_shader_pixel_local_storage:enable
#extension GL_ARM_shader_framebuffer_fetch:enable
#extension GL_EXT_shader_framebuffer_fetch:enable
#define G1 __pixel_localEXT L0{
#define F0(e,a) layout(rgba8)lowp vec4 a
#define G0(e,a) layout(r32ui)highp uint a
#define H1 };
#define I0(c) c
#define H0(c) c
#define w0(c,m) c=(m)
#define P0(c,m) c=(m)
#define n1(c)
#define A1(c)
#define p1
#define q1
#endif
#ifdef KD
#extension GL_EXT_shader_framebuffer_fetch:require
#define G1
#define F0(e,a) layout(location=e)inout lowp vec4 a
#define G0(e,a) layout(location=e)inout highp uvec4 a
#define H1
#define I0(c) c
#define H0(c) c.x
#define w0(c,m) c=(m)
#define P0(c,m) c.x=(m)
#define n1(c) w0(c,I0(c))
#define A1(c) P0(c,H0(c))
#define p1
#define q1
#endif
#ifdef LD
#ifdef GL_ARB_shader_image_load_store
#extension GL_ARB_shader_image_load_store:require
#endif
#if defined(GL_ARB_fragment_shader_interlock)
#extension GL_ARB_fragment_shader_interlock:require
#define p1 beginInvocationInterlockARB()
#define q1 endInvocationInterlockARB()
#elif defined(GL_INTEL_fragment_shader_ordering)
#extension GL_INTEL_fragment_shader_ordering:require
#define p1 beginFragmentShaderOrderingINTEL()
#define q1
#else
#define p1
#define q1
#endif
#define G1
#ifdef FB
#define F0(e,a) layout(set=g4,binding=e,rgba8)uniform lowp coherent image2D a
#define G0(e,a) layout(set=g4,binding=e,r32ui)uniform highp coherent uimage2D a
#else
#define F0(e,a) layout(binding=e,rgba8)uniform lowp coherent image2D a
#define G0(e,a) layout(binding=e,r32ui)uniform highp coherent uimage2D a
#endif
#define H1
#define I0(c) imageLoad(c,J)
#define H0(c) imageLoad(c,J).x
#define w0(c,m) imageStore(c,J,m)
#define P0(c,m) imageStore(c,J,uvec4(m))
#define n1(c)
#define A1(c)
#ifndef LC
#define LC
#endif
#endif
#ifdef MD
layout(constant_id=0)const bool d5=false;
#define G1
#define F0(e,a) layout(input_attachment_index=e,binding=e,set=g4)uniform lowp subpassInput p4##a;layout(location=e)out lowp vec4 a
#define G0(e,a) layout(input_attachment_index=e,binding=e,set=g4)uniform lowp usubpassInput p4##a;layout(location=e)out highp uvec4 a
#define H1
#define I0(c) (d5?subpassLoad(p4##c):vec4(0))
#define H0(c) (d5?subpassLoad(p4##c).x:0u)
#define w0(c,m) c=(m)
#define P0(c,m) c.x=(m)
#define n1(c) w0(c,d5?subpassLoad(p4##c):vec4(1,0,1,1))
#define A1(c) P0(c,d5?subpassLoad(p4##c).x:0u)
#define p1
#define q1
#endif
#ifdef ND
#define G1
#define F0(e,a) layout(location=e)out lowp vec4 a
#define G0(e,a) layout(location=e)out highp uvec4 a
#define H1
#define I0(c) vec4(0)
#define H0(c) 0u
#define w0(c,m) c=(m)
#define P0(c,m) c.x=(m)
#define n1(c) c=vec4(1,0,1,1)
#define A1(c) c.x=0u
#define p1
#define q1
#endif
#define K4(c)
#ifdef FB
#define gl_VertexID gl_VertexIndex
#endif
#ifdef VC
#ifdef FB
#define e5 gl_InstanceIndex
#else
#ifdef OD
uniform int SPIRV_Cross_BaseInstance;
#define e5 (gl_InstanceID+SPIRV_Cross_BaseInstance)
#else
#define e5 (gl_InstanceID+gl_BaseInstance)
#endif
#endif
#else
#define e5 0
#endif
#define Z2
#define o3
#define g1(a,R,v,k,O) void main(){int k=gl_VertexID;int O=e5;
#define v5 g1
#define y4(a,a2,c2,n2,o2,k) g1(a,a2,c2,k,O)
#define S(a,I)
#define U(a)
#define P(a,I)
#define h1(v1) gl_Position=v1;}
#define w2(q4,a) layout(location=0)out q4 ka;void main()
#define x2(m) ka=m
#define p0 gl_FragCoord.xy
#define D4
#define Q2
#ifdef LC
#define P3(e,a) layout(set=g4,binding=e,r32ui)uniform highp coherent uimage2D a
#define w3(c) imageLoad(c,J).x
#define x3(c,m) imageStore(c,J,uvec4(m))
#define I4(c,S0) imageAtomicMax(c,J,S0)
#define J4(c,S0) imageAtomicAdd(c,J,S0)
#define M2 ,o0 J
#define m1 ,J
#define T1(a) void main(){o0 J=ivec2(floor(p0));
#define f2 }
#else
#define M2
#define m1
#define T1(a) void main()
#define f2
#endif
#define Z3(a) T1(a)
#define v3(a) layout(location=0)out j Q0;T1(a)
#define G4(a) layout(location=0)out j Q0;T1(a)
#define a4 f2
#define l0(f0,k0) ((f0)*(k0))
#if UB<310
i j unpackUnorm4x8(uint u){W h2=W(u&0xffu,(u>>8)&0xffu,(u>>16)&0xffu,u>>24);return g(h2)*(1./255.);}
#endif
#ifndef FB
#define N9
#endif
precision highp float;precision highp int;
)===";
} // namespace glsl
} // namespace pls
} // namespace rive