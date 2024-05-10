#pragma once

#include "glsl.exports.h"

namespace rive {
namespace pls {
namespace glsl {
const char glsl[] = R"===(#ifndef TB
#define TB __VERSION__
#endif
#define d vec2
#define D1 vec3
#define H3 vec3
#define g vec4
#define h mediump float
#define l0 mediump vec2
#define O mediump vec3
#define i mediump vec4
#define E0 float
#define Z1 vec2
#define j0 vec3
#define M0 vec4
#define m0 ivec2
#define G5 ivec4
#define a6 mediump int
#define x0 uvec2
#define T uvec4
#define M mediump uint
#define O0 uint
#define A mat2
#define j5 mat3x4
#define p
#define y2(h4) out h4
#ifdef GL_ANGLE_base_vertex_base_instance_shader_builtin
#extension GL_ANGLE_base_vertex_base_instance_shader_builtin:require
#endif
#ifdef CC
#extension GL_ARB_bindless_texture:require
#endif
#ifdef MC
#extension GL_KHR_blend_equation_advanced:require
#endif
#ifdef CB
#ifdef Z
#ifdef GL_EXT_clip_cull_distance
#extension GL_EXT_clip_cull_distance:require
#elif defined(GL_ANGLE_clip_cull_distance)
#extension GL_ANGLE_clip_cull_distance:require
#endif
#endif
#endif
#if TB>=310
#define X3(c,a) layout(binding=c,std140)uniform a{
#else
#define X3(c,a) layout(std140)uniform a{
#endif
#define G4(a) }a;
#define U0(a)
#define q0(c,G,a) layout(location=c)in G a
#define V0
#define v0(c6,r,a,G)
#ifdef V
#if TB>=310
#define I(c,G,a) layout(location=c)out G a
#else
#define I(c,G,a) out G a
#endif
#else
#if TB>=310
#define I(c,G,a) layout(location=c)in G a
#else
#define I(c,G,a) in G a
#endif
#endif
#define d6 flat
#define A1
#define B1
#ifdef EB
#define k0
#else
#ifdef GL_NV_shader_noperspective_interpolation
#extension GL_NV_shader_noperspective_interpolation:require
#define k0 noperspective
#else
#define k0
#endif
#endif
#ifdef V
#define O1
#define P1
#endif
#ifdef GB
#define F2
#define G2
#endif
#ifdef EB
#define Q2(c,a) layout(binding=c)uniform highp utexture2D a
#define i4(c,a) layout(binding=c)uniform highp texture2D a
#define x1(c,a) layout(binding=c)uniform mediump texture2D a
#elif TB>=310
#define Q2(c,a) layout(binding=c)uniform highp usampler2D a
#define i4(c,a) layout(binding=c)uniform highp sampler2D a
#define x1(c,a) layout(binding=c)uniform mediump sampler2D a
#else
#define Q2(c,a) uniform highp usampler2D a
#define i4(c,a) uniform highp sampler2D a
#define x1(c,a) uniform mediump sampler2D a
#endif
#define aa(c,a) Q2(c,a)
#ifdef EB
#define J3(d2,a) layout(binding=d2,set=A7)uniform mediump sampler a;
#define l3(d2,a) layout(binding=d2,set=A7)uniform mediump sampler a;
#define M2(a,p0,F) texture(sampler2D(a,p0),F)
#define R3(a,p0,F,X2) textureLod(sampler2D(a,p0),F,X2)
#define Q3(a,p0,F,Y2,Z2) textureGrad(sampler2D(a,p0),F,Y2,Z2)
#else
#define J3(d2,a)
#define l3(d2,a)
#define M2(a,p0,F) texture(a,F)
#define R3(a,p0,F,X2) textureLod(a,F,X2)
#define Q3(a,p0,F,Y2,Z2) textureGrad(a,F,Y2,Z2)
#endif
#define I1(a,F) texelFetch(a,F,0)
#define V1
#define W1
#define L3
#define O3
#ifdef HD
#define M3(c,d1,a) Q2(c,a)
#define R2(c,d1,a) aa(c,a)
#define N3(c,d1,a) i4(c,a)
#define z0(a,A0) I1(a,m0((A0)&l7,(A0)>>k7))
#define q2(a,A0) I1(a,m0((A0)&l7,(A0)>>k7)).xy
#else
#ifdef GL_ARB_shader_storage_buffer_object
#extension GL_ARB_shader_storage_buffer_object:require
#endif
#define M3(c,d1,a) layout(std430,binding=c)readonly buffer d1{x0 U4[];}a
#define R2(c,d1,a) layout(std430,binding=c)readonly buffer d1{T U4[];}a
#define N3(c,d1,a) layout(std430,binding=c)readonly buffer d1{g U4[];}a
#define z0(a,A0) a.U4[A0]
#define q2(a,A0) a.U4[A0]
#endif
#ifdef _EXPORTED_PLS_IMPL_ANGLE
#extension GL_ANGLE_shader_pixel_local_storage:require
#define E1
#define C0(c,a) layout(binding=c,rgba8)uniform lowp pixelLocalANGLE a
#define D0(c,a) layout(binding=c,r32ui)uniform highp upixelLocalANGLE a
#define F1
#define N0(e) pixelLocalLoadANGLE(e)
#define L0(e) pixelLocalLoadANGLE(e).x
#define F0(e,l) pixelLocalStoreANGLE(e,l)
#define a1(e,l) pixelLocalStoreANGLE(e,uvec4(l))
#define y0(e)
#define n1
#define o1
#endif
#ifdef ID
#extension GL_EXT_shader_pixel_local_storage:enable
#extension GL_ARM_shader_framebuffer_fetch:enable
#extension GL_EXT_shader_framebuffer_fetch:enable
#define E1 __pixel_localEXT H0{
#define C0(c,a) layout(rgba8)lowp vec4 a
#define D0(c,a) layout(r32ui)highp uint a
#define F1 };
#define N0(e) e
#define L0(e) e
#define F0(e,l) e=(l)
#define a1(e,l) e=(l)
#define y0(e)
#define n1
#define o1
#endif
#ifdef JD
#extension GL_EXT_shader_framebuffer_fetch:require
#define E1
#define C0(c,a) layout(location=c)inout lowp vec4 a
#define D0(c,a) layout(location=c)inout highp uvec4 a
#define F1
#define N0(e) e
#define L0(e) e.x
#define F0(e,l) e=(l)
#define a1(e,l) e.x=(l)
#define y0(e) e=e
#define n1
#define o1
#endif
#ifdef KD
#ifdef GL_ARB_shader_image_load_store
#extension GL_ARB_shader_image_load_store:require
#endif
#if defined(GL_ARB_fragment_shader_interlock)
#extension GL_ARB_fragment_shader_interlock:require
#define n1 beginInvocationInterlockARB()
#define o1 endInvocationInterlockARB()
#elif defined(GL_INTEL_fragment_shader_ordering)
#extension GL_INTEL_fragment_shader_ordering:require
#define n1 beginFragmentShaderOrderingINTEL()
#define o1
#else
#define n1
#define o1
#endif
#define E1
#ifdef EB
#define C0(c,a) layout(set=K4,binding=c,rgba8)uniform lowp coherent image2D a
#define D0(c,a) layout(set=K4,binding=c,r32ui)uniform highp coherent uimage2D a
#else
#define C0(c,a) layout(binding=c,rgba8)uniform lowp coherent image2D a
#define D0(c,a) layout(binding=c,r32ui)uniform highp coherent uimage2D a
#endif
#define F1
#define N0(e) imageLoad(e,H)
#define L0(e) imageLoad(e,H).x
#define F0(e,l) imageStore(e,H,l)
#define a1(e,l) imageStore(e,H,uvec4(l))
#define y0(e)
#endif
#ifdef LD
#define E1
#define C0(c,a) layout(input_attachment_index=c,binding=c,set=K4)uniform lowp subpassInput V4##a;layout(location=c)out lowp vec4 a
#define D0(c,a) layout(input_attachment_index=c,binding=c,set=K4)uniform lowp usubpassInput V4##a;layout(location=c)out highp uvec4 a
#define F1
#define N0(e) subpassLoad(V4##e)
#define L0(e) subpassLoad(V4##e).x
#define F0(e,l) e=(l)
#define a1(e,l) e.x=(l)
#define y0(e) e=subpassLoad(V4##e)
#define n1
#define o1
#endif
#ifdef TC
#define E1
#define C0(c,a) layout(location=c)out lowp vec4 a
#define D0(c,a) layout(location=c)out highp uvec4 a
#define F1
#define N0(e) vec4(0)
#define L0(e) 0u
#define F0(e,l) e=(l)
#define a1(e,l) e.x=(l)
#define y0(e)
#define n1
#define o1
#endif
#define D4(e)
#ifdef EB
#define gl_VertexID gl_VertexIndex
#endif
#ifdef UC
#ifdef EB
#define W4 gl_InstanceIndex
#else
#ifdef MD
uniform int SPIRV_Cross_BaseInstance;
#define W4 (gl_InstanceID+SPIRV_Cross_BaseInstance)
#else
#define W4 (gl_InstanceID+gl_BaseInstance)
#endif
#endif
#else
#define W4 0
#endif
#define U2
#define j3
#define g1(a,P,r,j,L) void main(){int j=gl_VertexID;int L=W4;
#define k5 g1
#define o4(a,X1,Y1,l2,m2,j) g1(a,X1,Y1,j,L)
#define Q(a,G)
#define S(a)
#define N(a,G)
#define h1(q1) gl_Position=q1;}
#define r2(j4,a) layout(location=0)out j4 ba;void main()
#define v2(l) ba=l
#define n0 gl_FragCoord.xy
#define w4
#define L2
#ifdef ND
#define K3(c,a) layout(binding=c,r32ui)uniform highp coherent uimage2D a
#define o3(e) imageLoad(e,H).x
#define p3(e,l) imageStore(e,H,uvec4(l))
#define B4(e,S0) imageAtomicMax(e,H,S0)
#define C4(e,S0) imageAtomicAdd(e,H,S0)
#define H2 ,m0 H
#define l1 ,H
#define R1(a) void main(){m0 H=ivec2(floor(n0));
#define c2 }
#else
#define H2
#define l1
#define R1(a) void main()
#define c2
#endif
#define U3(a) R1(a)
#define n3(a) layout(location=0)out i P0;R1(a)
#define z4(a) layout(location=0)out i P0;R1(a)
#define V3 c2
#define h0(d0,g0) ((d0)*(g0))
#if TB<310
p i unpackUnorm4x8(uint u){T e2=T(u&0xffu,(u>>8)&0xffu,(u>>16)&0xffu,u>>24);return g(e2)*(1./255.);}
#endif
#ifndef EB
#define y9
#endif
precision highp float;precision highp int;
)===";
} // namespace glsl
} // namespace pls
} // namespace rive