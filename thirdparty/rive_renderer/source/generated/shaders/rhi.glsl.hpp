#pragma once

#include "rhi.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char rhi[] = R"===(#pragma warning(disable:3550)
#pragma warning(disable:4000)
#ifndef _ARE_TOKEN_NAMES_PRESERVED
#define g half
#define G half2
#define A half3
#define i half4
#define a0 ushort
#define c float2
#define Z float3
#define f float4
#define a5 bool2
#define G7 bool3
#define N0 uint2
#define M uint4
#define c0 int2
#define h7 int4
#define a0 ushort
#define S float2x2
#define U5 half3x3
#define V5 half2x3
#endif
typedef Z a4;
#ifdef PD
#if Ie
typedef min16uint a0;
#endif
#else
#if Ie
typedef uint a0;
#endif
#endif
#define Tb(o,r) o##r
#define d inline
#define k1(P1) out P1
#define i4(P1) inout P1
#define U0(a) struct a{
#define i0(e,W,a) W a:Tb(Sf,e)
#define V0 };
#define l0(H7,B,a,W) W a=B.a
#define e5(e,a) cbuffer a{struct{
#define W5(a) }a;}
#define o1 struct k0{
#define n0 noperspective
#define OB nointerpolation
#define L2 nointerpolation
#define H(e,W,a) W a:Tb(TEXCOORD,e)
#define p1 f g1:SV_Position;};
#define L(a,W) W a
#define P(a) X.a=a
#define N(a,W) W a=X.a
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
#define I3(g0,e,a) uniform Texture2D<M>a
#define r4(g0,e,a) uniform Texture2D<f>a
#define C2(g0,e,a) uniform Texture2D<Je i>a
#define y4(g0,e,a) uniform Texture2D<g>a
#define l5(g0,e,a) uniform Texture2DArray<g>a
#define F4(m2,a) SamplerState a;
#define P3 F4
#define G3 F4
#define d1(a,l) a[l]
#define D4(a,p,l) a.Sample(p,l)
#define T1(a,p,l,G0) a.SampleLevel(p,l,G0)
#define p5(a,p,l,X2) a.Gather(p,(l)*(X2))
#define T5(a,p,m,w5,K7,G0) a.SampleLevel(p,Z(m,0.5,w5),G0)
#define o4(A0,p,l) D4(A0,p,l)
#define C7(A0,p,l,G0) T1(A0,p,l,G0)
#define h2
#define j2
#ifdef QD
#define n2 RasterizerOrderedTexture2D
#else
#define n2 RWTexture2D
#endif
#define x2
#ifdef FC
#define M0(e,a) uniform n2<Je i>a
#else
#define M0(e,a) uniform n2<uint>a
#endif
#define O4 M0
#define Y0(e,a) uniform n2<uint>a
#define f4 Y0
#define m4 j1
#define n4 l1
#define y2
#ifdef FC
#define I0(h) h[I]
#else
#define I0(h) unpackUnorm4x8(h[I])
#endif
#define j1(h) h[I]
#ifdef FC
#define T0(h,F) h[I]=(F)
#else
#define T0(h,F) h[I]=packUnorm4x8(F)
#endif
#define l1(h,F) h[I]=(F)
d uint r6(n2<uint>G4,c0 I,uint x){uint B1;InterlockedMax(G4[I],x,B1);return B1;}
#define G5(h,m) r6(h,I,m)
d uint v6(n2<uint>G4,c0 I,uint x){uint B1;InterlockedAdd(G4[I],x,B1);return B1;}
#define H5(h,m) v6(h,I,m)
#define E2(h)
#define i2(h)
#define q5
#define Y1
#define n5
#define x1
#define q1(a,f0,B,n,K) uint baseInstance;k0 a(f0 B,uint n:SV_VertexID,uint A9:SV_InstanceID){uint K=A9+baseInstance;k0 X;
#define G6(a,f0,B,n,K) k0 a(f0 B,uint n:SV_VertexID){k0 X;f g1;
#define N4(a,a2,c2,v2,w2,n) k0 a(a2 c2,v2 w2,uint n:SV_VertexID){k0 X;f g1;
#define h1(w6) X.g1=w6;}return X;
#define e2(E4,a) E4 a(k0 X):SV_Target{
#define f2(F) return F;}
#define C5 ,c y0
#define Y2 ,y0
#define y3 ,c0 I
#define G1 ,I
#define z2(a) Ke void a(k0 X){c y0=X.g1.xy;c0 I=c0(floor(y0));
#define R4(a) z2(a)
#define M2 }
#define A3(a) Ke i a(k0 X):SV_Target{c y0=X.g1.xy;c0 I=c0(floor(y0));i K1;
#define F5(a) A3(a)
#define P4 }return K1;
#define uintBitsToFloat asfloat
#define floatBitsToInt asint
#define floatBitsToUint asuint
#define inversesqrt rsqrt
#define equal(o,r) ((o)==(r))
#define notEqual(o,r) ((o)!=(r))
#define lessThan(o,r) ((o)<(r))
#define C0(o,r) mul(r,o)
#define E3
#define F3
#define w3
#define x3
#define g4(e,f1,a) StructuredBuffer<N0>a
#define O3(e,f1,a) StructuredBuffer<M>a
#define h4(e,f1,a) StructuredBuffer<f>a
#define w0(a,v0) a[v0]
#define k4(a,v0) a[v0]
d G unpackHalf2x16(uint u){uint y=(u>>16);uint x=u&0xffffu;return G(f16tof32(x),f16tof32(y));}d uint packHalf2x16(c M1){uint x=f32tof16(M1.x);uint y=f32tof16(M1.y);return(y<<16)|x;}d i unpackUnorm4x8(uint u){M A1=M(u&0xffu,(u>>8)&0xffu,(u>>16)&0xffu,u>>24);return i(A1)*(1./255.);}d uint packUnorm4x8(i j){M A1=(M(j*255.)&0xff)<<M(0,8,16,24);A1.xy|=A1.zw;A1.x|=A1.y;return A1.x;}d S inverse(S W0){S B9=S(W0[1][1],-W0[0][1],-W0[1][0],W0[0][0]);return B9*(1./determinant(W0));}d float mix(float x,float y,float s){return lerp(x,y,s);}d c mix(c x,c y,c s){return lerp(x,y,s);}d Z mix(Z x,Z y,Z s){return lerp(x,y,s);}d f mix(f x,f y,f s){return lerp(x,y,s);}d float fract(float x){return frac(x);}d c fract(c x){return frac(x);}d Z fract(Z x){return frac(x);}d f fract(f x){return frac(x);}d float mod(float x,float y){return fmod(x,y);}d float o2(float x){return sign(x);}d c o2(c x){return sign(x);}d Z o2(Z x){return sign(x);}d f o2(f x){return sign(x);}
#define sign o2
d float p2(float x){return abs(x);}d c p2(c x){return abs(x);}d Z p2(Z x){return abs(x);}d f p2(f x){return abs(x);}
#define abs p2
d float q2(float x){return sqrt(x);}d c q2(c x){return sqrt(x);}d Z q2(Z x){return sqrt(x);}d f q2(f x){return sqrt(x);}
#define sqrt q2
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive