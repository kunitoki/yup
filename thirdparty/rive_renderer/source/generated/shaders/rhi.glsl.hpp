#pragma once

#include "rhi.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char rhi[] = R"===(#pragma warning(disable:3550)
#pragma warning(disable:4000)
#ifndef _ARE_TOKEN_NAMES_PRESERVED
#define d half
#define D half2
#define r half3
#define i half4
#define X ushort
#define c float2
#define V float3
#define g float4
#define E4 bool2
#define n6 bool3
#define w7 bool4
#define W0 uint2
#define Q uint4
#define U int2
#define Z5 int4
#define X ushort
#define Z float2x2
#define V6 half3x3
#define W6 half2x3
#define h5 half4x4
#endif
typedef V L3;
#ifdef HE
#if Jg
typedef min16uint X;
#endif
#else
#if Jg
typedef uint X;
#endif
#endif
#define Dd(A,F) A##F
#define e inline
#define e1(g2) out g2
#define T4(g2) inout g2
#define A1(a) struct a{
#define p0(f,W,a) W a:Dd(ni,f)
#define B1 };
#define q0(O8,G,a,W) W a=G.a
#define m6(f,a) cbuffer a{struct{
#define v7(a) }a;}
#define h2 struct f0{
#define J0 noperspective
#define OB nointerpolation
#define R4 nointerpolation
#define c0(f,W,a) W a:Dd(TEXCOORD,f)
#ifdef QE
#define a2 g L0:SV_Position;g Kg:SV_ClipDistance;};
#else
#define a2 g L0:SV_Position;};
#endif
#define Y(a,W) W a
#define k0(a) R.a=a
#define B(a,W) W a=R.a
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
#define D4(M,f,a) uniform Texture2D<Q>a
#define e5(M,f,a) uniform Texture2D<g>a
#ifdef GD
#define Te(M,f,a) uniform Texture2DMS<i>a
#endif
#define X2(M,f,a) uniform Texture2D<i>a
#define k5(M,f,a) uniform Texture2D<d>a
#define f6(M,f,a) uniform Texture2DArray<d>a
#define x5(f,a) SamplerState a;
#define X3 x5
#define o6(M,f,a) x5(f,a)
#define U3(a) x5(T3,a)
#ifdef qi
#define m8(a,Lg,l) a.ri(l,Lg)
#endif
#define v1(a,l) a[l]
#define r5(a,p,l) a.Sample(p,l)
#define m2(a,p,l,X0) a.SampleLevel(p,l,X0)
#define v5(a,p,l,P1) a.SampleBias(p,l,P1)
#define U6(a,p,q,p6,Q8,X0) a.SampleLevel(p,V(q,0.5,p6),X0)
#define g8(h0,p,l) r5(h0,p,l)
#define Q6(h0,p,l,X0) m2(h0,p,l,X0)
#define y7(h0,p,l,P1) v5(h0,p,l,P1)
#define v2
#define w2
#ifdef IE
#define K2 RasterizerOrderedTexture2D
#else
#define K2 RWTexture2D
#endif
#if defined(FB)&&defined(BB)
#ifdef MF
#define g7(a) [[ti::input_attachment_index(Q2)]]SubpassInputMS<i>a
#define S8(a) oc(h5(a.Oa(0),a.Oa(1),a.Oa(2),a.Oa(3)),Pa)
#else
#define g7(a) Texture2D a
#define S8(a) a[E]
#endif
#endif
#define J1
#define K1
#ifdef KC
#define r0(f,a) uniform K2<ui i>a
#else
#define r0(f,a) uniform K2<uint>a
#endif
#define n4 r0
#define k1(f,a) uniform K2<uint>a
#define T2 d1
#define U2 f1
#if COMPILER_METAL||FORCE_ATOMIC_BUFFER
#define E2(f,a) uniform RWBuffer<uint>a
#define T2(h) h[C0]
#define U2(h,C) h[C0]=C
#else
#define E2 k1
#define T2 d1
#define U2 f1
#endif
#ifdef KC
#define H0(h) h[E]
#else
#define H0(h) unpackUnorm4x8(h[E])
#endif
#define d1(h) h[E]
#ifdef KC
#define v0(h,C) h[E]=(C)
#else
#define v0(h,C) h[E]=packUnorm4x8(C)
#endif
#define f1(h,C) h[E]=(C)
#if COMPILER_METAL||FORCE_ATOMIC_BUFFER
e uint y5(RWBuffer<uint>p3,uint C0,uint x){uint a1;InterlockedMax(p3[C0],x,a1);return a1;}
#define W4(h,q) y5(h,C0,q)
e uint z5(RWBuffer<uint>p3,uint C0,uint x){uint a1;InterlockedAdd(p3[C0],x,a1);return a1;}
#define X4(h,q) z5(h,C0,q)
#else
e uint y5(K2<uint>p3,U E,uint x){uint a1;InterlockedMax(p3[E],x,a1);return a1;}
#define W4(h,q) y5(h,E,q)
e uint z5(K2<uint>p3,U E,uint x){uint a1;InterlockedAdd(p3[E],x,a1);return a1;}
#define X4(h,q) z5(h,E,q)
#endif
#define r2(h)
#define Y1(h)
#define i6
#define v3
#define F3
#define g1
#ifdef RE
#define C1(a,a0,G,v,T) uint baseInstance;g a(a0 G,uint v:SV_VertexID,uint D7:SV_InstanceID):SV_Position{uint T=D7+baseInstance;
#define D1(A5) return A5;}
#else
#define C1(a,a0,G,v,T) uint baseInstance;f0 a(a0 G,uint v:SV_VertexID,uint D7:SV_InstanceID){uint T=D7+baseInstance;f0 R;
#define S7(a,a0,G,v,T) f0 a(a0 G,uint v:SV_VertexID){f0 R;g L0;
#define E6(a,h3,i3,w3,x3,v) f0 a(h3 i3,w3 x3,uint v:SV_VertexID){f0 R;g L0;
#define D1(A5) R.L0=A5;}return R;
#endif
#ifdef RE
#define Y2(Q1,a) EARLYDEPTHSTENCIL Q1 a(g L0:SV_Position):SV_Target{c S=L0.xy;
#define q6(Q1,a) vi Q1 a(g L0:SV_Position,uint Pa:SV_Coverage,bool Qa:SV_IsFrontFace):SV_Target{c S=L0.xy;bool r6=!Qa;
#else
#define Y2(Q1,a) EARLYDEPTHSTENCIL Q1 a(f0 R,uint Pa:SV_Coverage):SV_Target{c S=R.L0.xy;U E=U(floor(S));uint C0=E.y*k.q5+E.x;
#define q6(Q1,a) Q1 a(f0 R,uint Pa:SV_Coverage,bool Qa:SV_IsFrontFace):SV_Target{c S=R.L0.xy;U E=U(floor(S));uint C0=E.y*k.q5+E.x;bool r6=!Qa;
#endif
#define G2(C) return C;}
#ifdef QE
#define a7 ,out g gl_ClipDistance
#define w5 ,R.Kg
#else
#define a7
#define w5
#endif
#define G6 ,c S
#define S2 ,S
#define P3 ,U E
#define M1 ,E
#define m1(a) EARLYDEPTHSTENCIL void a(f0 R){c S=R.L0.xy;U E=U(floor(S));uint C0=E.y*k.q5+E.x;
#define O5(a) m1(a)
#if defined(K)&&defined(LB)
#define U1 l3
#else
#define U1 }
#endif
#define o2(a) EARLYDEPTHSTENCIL i a(f0 R):SV_Target{c S=R.L0.xy;U E=U(floor(S));uint C0=E.y*k.q5+E.x;i l1;
#define r4(a) o2(a)
#define l3 }return l1;
#define uintBitsToFloat asfloat
#define floatBitsToInt asint
#define floatBitsToUint asuint
#define inversesqrt rsqrt
#define equal(A,F) ((A)==(F))
#define notEqual(A,F) ((A)!=(F))
#define lessThan(A,F) ((A)<(F))
#define greaterThan(A,F) ((A)>(F))
#define Z0(A,F) mul(F,A)
#define A4
#define B4
#define N3
#define O3
#define J5(f,y1,a) StructuredBuffer<W0>a
#define F4(f,y1,a) StructuredBuffer<Q>a
#define K5(f,y1,a) StructuredBuffer<g>a
#define P0(a,y0) a[y0]
#define L5(a,y0) a[y0]
e D unpackHalf2x16(uint u){uint y=(u>>16);uint x=u&0xffffu;return D(f16tof32(x),f16tof32(y));}e uint packHalf2x16(c e2){uint x=f32tof16(e2.x);uint y=f32tof16(e2.y);return(y<<16)|x;}e i unpackUnorm4x8(uint u){Q R1=Q(u&0xffu,(u>>8)&0xffu,(u>>16)&0xffu,u>>24);return i(R1)*(1./255.);}e uint packUnorm4x8(i j){Q R1=(Q(j*255.)&0xff)<<Q(0,8,16,24);R1.xy|=R1.zw;R1.x|=R1.y;return R1.x;}e Z inverse(Z o1){Z La=Z(o1[1][1],-o1[0][1],-o1[1][0],o1[0][0]);return La*(1./determinant(o1));}e float mix(float x,float y,float s){return lerp(x,y,s);}e c mix(c x,c y,c s){return lerp(x,y,s);}e V mix(V x,V y,V s){return lerp(x,y,s);}e g mix(g x,g y,g s){return lerp(x,y,s);}e float fract(float x){return frac(x);}e c fract(c x){return frac(x);}e V fract(V x){return frac(x);}e g fract(g x){return frac(x);}e float mod(float x,float y){return fmod(x,y);}e float L2(float x){return sign(x);}e c L2(c x){return sign(x);}e V L2(V x){return sign(x);}e g L2(g x){return sign(x);}
#define sign L2
e float M2(float x){return abs(x);}e c M2(c x){return abs(x);}e V M2(V x){return abs(x);}e g M2(g x){return abs(x);}
#define abs M2
e float N2(float x){return sqrt(x);}e c N2(c x){return sqrt(x);}e V N2(V x){return sqrt(x);}e g N2(g x){return sqrt(x);}
#define sqrt N2
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive