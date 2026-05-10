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
#define F4 bool2
#define n6 bool3
#define w7 bool4
#define W0 uint2
#define Q uint4
#define U int2
#define Z5 int4
#define X ushort
#define a0 float2x2
#define V6 half3x3
#define W6 half2x3
#define g5 half4x4
#endif
typedef V I3;
#ifdef BE
#if Dg
typedef min16uint X;
#endif
#else
#if Dg
typedef uint X;
#endif
#endif
#define yd(A,F) A##F
#define e inline
#define g1(g2) out g2
#define U4(g2) inout g2
#define A1(a) struct a{
#define r0(f,W,a) W a:yd(li,f)
#define B1 };
#define v0(N8,G,a,W) W a=G.a
#define l6(f,a) cbuffer a{struct{
#define v7(a) }a;}
#define h2 struct g0{
#define J0 noperspective
#define NB nointerpolation
#define S4 nointerpolation
#define d0(f,W,a) W a:yd(TEXCOORD,f)
#ifdef KE
#define Z1 g L0:SV_Position;g Eg:SV_ClipDistance;};
#else
#define Z1 g L0:SV_Position;};
#endif
#define Y(a,W) W a
#define l0(a) R.a=a
#define B(a,W) W a=R.a
#ifdef BB
#define P3
#define Q3
#endif
#ifdef EB
#define y3
#define z3
#endif
#define Z4
#define a5
#define E4(N,f,a) uniform Texture2D<Q>a
#define d5(N,f,a) uniform Texture2D<g>a
#ifdef ED
#define Ne(N,f,a) uniform Texture2DMS<i>a
#endif
#define U2(N,f,a) uniform Texture2D<i>a
#define j5(N,f,a) uniform Texture2D<d>a
#define e6(N,f,a) uniform Texture2DArray<d>a
#define v5(f,a) SamplerState a;
#define V3 v5
#define o6(N,f,a) v5(f,a)
#define S3(a) v5(R3,a)
#ifdef oi
#define m8(a,Fg,l) a.pi(l,Fg)
#endif
#define F1(a,l) a[l]
#define p5(a,p,l) a.Sample(p,l)
#define m2(a,p,l,X0) a.SampleLevel(p,l,X0)
#define q5(a,p,l,P1) a.SampleBias(p,l,P1)
#define U6(a,p,q,p6,P8,X0) a.SampleLevel(p,V(q,0.5,p6),X0)
#define g8(h0,p,l) p5(h0,p,l)
#define Q6(h0,p,l,X0) m2(h0,p,l,X0)
#define y7(h0,p,l,P1) q5(h0,p,l,P1)
#define v2
#define w2
#ifdef CE
#define J2 RasterizerOrderedTexture2D
#else
#define J2 RWTexture2D
#endif
#if defined(EB)&&defined(AB)
#ifdef KF
#define g7(a) [[ri::input_attachment_index(P2)]]SubpassInputMS<i>a
#define S8(a) jc(g5(a.Ja(0),a.Ja(1),a.Ja(2),a.Ja(3)),Ka)
#else
#define g7(a) Texture2D a
#define S8(a) a[E]
#endif
#endif
#define K1
#define L1
#ifdef KC
#define p0(f,a) uniform J2<si i>a
#else
#define p0(f,a) uniform J2<uint>a
#endif
#define m4 p0
#define f1(f,a) uniform J2<uint>a
#define N3 e1
#define O3 h1
#if COMPILER_METAL||FORCE_ATOMIC_BUFFER
#define o4(f,a) uniform RWBuffer<uint>a
#define N3(h) h[c1]
#define O3(h,C) h[c1]=C
#else
#define o4 f1
#define N3 e1
#define O3 h1
#endif
#ifdef KC
#define H0(h) h[E]
#else
#define H0(h) unpackUnorm4x8(h[E])
#endif
#define e1(h) h[E]
#ifdef KC
#define C0(h,C) h[E]=(C)
#else
#define C0(h,C) h[E]=packUnorm4x8(C)
#endif
#define h1(h,C) h[E]=(C)
#if COMPILER_METAL||FORCE_ATOMIC_BUFFER
e uint w5(RWBuffer<uint>m3,uint c1,uint x){uint a1;InterlockedMax(m3[c1],x,a1);return a1;}
#define O5(h,q) w5(h,c1,q)
e uint x5(RWBuffer<uint>m3,uint c1,uint x){uint a1;InterlockedAdd(m3[c1],x,a1);return a1;}
#define Q5(h,q) x5(h,c1,q)
#else
e uint w5(J2<uint>m3,U E,uint x){uint a1;InterlockedMax(m3[E],x,a1);return a1;}
#define O5(h,q) w5(h,E,q)
e uint x5(J2<uint>m3,U E,uint x){uint a1;InterlockedAdd(m3[E],x,a1);return a1;}
#define Q5(h,q) x5(h,E,q)
#endif
#define r2(h)
#define X1(h)
#define h6
#define p3
#define C3
#define i1
#ifdef LE
#define C1(a,c0,G,v,S) uint baseInstance;g a(c0 G,uint v:SV_VertexID,uint D7:SV_InstanceID):SV_Position{uint S=D7+baseInstance;
#define D1(y5) return y5;}
#else
#define C1(a,c0,G,v,S) uint baseInstance;g0 a(c0 G,uint v:SV_VertexID,uint D7:SV_InstanceID){uint S=D7+baseInstance;g0 R;
#define S7(a,c0,G,v,S) g0 a(c0 G,uint v:SV_VertexID){g0 R;g L0;
#define E6(a,e3,f3,q3,r3,v) g0 a(e3 f3,q3 r3,uint v:SV_VertexID){g0 R;g L0;
#define D1(y5) R.L0=y5;}return R;
#endif
#ifdef LE
#define V2(Q1,a) EARLYDEPTHSTENCIL Q1 a(g L0:SV_Position):SV_Target{c T=L0.xy;
#define q6(Q1,a) ti Q1 a(g L0:SV_Position,uint Ka:SV_Coverage,bool La:SV_IsFrontFace):SV_Target{c T=L0.xy;bool r6=!La;
#else
#define V2(Q1,a) EARLYDEPTHSTENCIL Q1 a(g0 R,uint Ka:SV_Coverage):SV_Target{c T=R.L0.xy;U E=U(floor(T));uint c1=E.y*k.m6+E.x;
#define q6(Q1,a) Q1 a(g0 R,uint Ka:SV_Coverage,bool La:SV_IsFrontFace):SV_Target{c T=R.L0.xy;U E=U(floor(T));uint c1=E.y*k.m6+E.x;bool r6=!La;
#endif
#define F2(C) return C;}
#ifdef KE
#define a7 ,out g gl_ClipDistance
#define r5 ,R.Eg
#else
#define a7
#define r5
#endif
#define G6 ,c T
#define R2 ,T
#define r4 ,U E
#define U1 ,E
#define v1(a) EARLYDEPTHSTENCIL void a(g0 R){c T=R.L0.xy;U E=U(floor(T));uint c1=E.y*k.m6+E.x;
#define M5(a) v1(a)
#if defined(K)&&defined(KB)
#define c2 i3
#else
#define c2 }
#endif
#define o2(a) EARLYDEPTHSTENCIL i a(g0 R):SV_Target{c T=R.L0.xy;U E=U(floor(T));uint c1=E.y*k.m6+E.x;i m1;
#define v4(a) o2(a)
#define i3 }return m1;
#define uintBitsToFloat asfloat
#define floatBitsToInt asint
#define floatBitsToUint asuint
#define inversesqrt rsqrt
#define equal(A,F) ((A)==(F))
#define notEqual(A,F) ((A)!=(F))
#define lessThan(A,F) ((A)<(F))
#define greaterThan(A,F) ((A)>(F))
#define Z0(A,F) mul(F,A)
#define B4
#define C4
#define K3
#define L3
#define H5(f,y1,a) StructuredBuffer<W0>a
#define G4(f,y1,a) StructuredBuffer<Q>a
#define I5(f,y1,a) StructuredBuffer<g>a
#define P0(a,y0) a[y0]
#define J5(a,y0) a[y0]
e D unpackHalf2x16(uint u){uint y=(u>>16);uint x=u&0xffffu;return D(f16tof32(x),f16tof32(y));}e uint packHalf2x16(c e2){uint x=f32tof16(e2.x);uint y=f32tof16(e2.y);return(y<<16)|x;}e i unpackUnorm4x8(uint u){Q R1=Q(u&0xffu,(u>>8)&0xffu,(u>>16)&0xffu,u>>24);return i(R1)*(1./255.);}e uint packUnorm4x8(i j){Q R1=(Q(j*255.)&0xff)<<Q(0,8,16,24);R1.xy|=R1.zw;R1.x|=R1.y;return R1.x;}e a0 inverse(a0 o1){a0 Ga=a0(o1[1][1],-o1[0][1],-o1[1][0],o1[0][0]);return Ga*(1./determinant(o1));}e float mix(float x,float y,float s){return lerp(x,y,s);}e c mix(c x,c y,c s){return lerp(x,y,s);}e V mix(V x,V y,V s){return lerp(x,y,s);}e g mix(g x,g y,g s){return lerp(x,y,s);}e float fract(float x){return frac(x);}e c fract(c x){return frac(x);}e V fract(V x){return frac(x);}e g fract(g x){return frac(x);}e float mod(float x,float y){return fmod(x,y);}e float K2(float x){return sign(x);}e c K2(c x){return sign(x);}e V K2(V x){return sign(x);}e g K2(g x){return sign(x);}
#define sign K2
e float L2(float x){return abs(x);}e c L2(c x){return abs(x);}e V L2(V x){return abs(x);}e g L2(g x){return abs(x);}
#define abs L2
e float M2(float x){return sqrt(x);}e c M2(c x){return sqrt(x);}e V M2(V x){return sqrt(x);}e g M2(g x){return sqrt(x);}
#define sqrt M2
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive