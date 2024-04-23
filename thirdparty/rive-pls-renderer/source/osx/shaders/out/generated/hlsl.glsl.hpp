#pragma once

#include "hlsl.exports.h"

namespace rive {
namespace pls {
namespace glsl {
const char hlsl[] = R"===(#pragma warning(disable:3550)
#ifndef _ARE_TOKEN_NAMES_PRESERVED
#define h half
#define l0 half2
#define O half3
#define i half4
#define Z5 short
#define M ushort
#define d float2
#define C1 float3
#define g float4
#define Z7 bool3
#define y0 uint2
#define T uint4
#define m0 int2
#define F5 int4
#define M ushort
#define A float2x2
#define d6 half3x4
#endif
typedef C1 G3;
#define E0 float
#define Y1 d
#define j0 C1
#define M0 g
typedef min16int Z5;typedef min16uint M;
#define O0 min16uint
#define i5 half3x4
#define p inline
#define x2(g4) out g4
#define T0(a) struct a{
#define q0(c,G,a) G a:a
#define U0 };
#define w0(a6,r,a,G) G a=r.a
#define a8(c) register(b##c)
#define W3(c,a) cbuffer a:a8(c){struct{
#define F4(a) }a;}
#define z1 struct Y{
#define k0 noperspective
#define GB nointerpolation
#define c6 nointerpolation
#define I(c,G,a) G a:TEXCOORD##c
#define A1 g p1:SV_Position;};
#define Q(a,G) G a
#define S(a) K.a=a
#define N(a,G) G a=K.a
#ifdef W
#define N1
#define O1
#endif
#ifdef HB
#define E2
#define F2
#endif
#define P2(c,a) uniform Texture2D<T>a:register(t##c)
#define h4(c,a) uniform Texture2D<g>a:register(t##c)
#define w1(c,a) uniform Texture2D<unorm g>a:register(t##c)
#define c8(c2,a) SamplerState a:register(s##c2);
#define I3 c8
#define k3 c8
#define H1(a,F) a[F]
#define L2(a,p0,F) a.Sample(p0,F)
#define Q3(a,p0,F,W2) a.SampleLevel(p0,F,W2)
#define P3(a,p0,F,X2,Y2) a.SampleGrad(p0,F,X2,Y2)
#define m1
#define n1
#ifdef QD
#define A3 RasterizerOrderedTexture2D
#else
#define A3 RWTexture2D
#endif
#define D1
#ifdef MC
#define C0(c,a) uniform A3<unorm i>a:register(u##c)
#else
#define C0(c,a) uniform A3<uint>a:register(u##c)
#endif
#define D0(c,a) uniform A3<uint>a:register(u##c)
#define J3 D0
#define n3 L0
#define o3 Z0
#define E1
#ifdef MC
#define N0(e) e[H]
#else
#define N0(e) unpackUnorm4x8(e[H])
#endif
#define L0(e) e[H]
#ifdef MC
#define F0(e,l) e[H]=(l)
#else
#define F0(e,l) e[H]=packUnorm4x8(l)
#endif
#define Z0(e,l) e[H]=(l)
p uint e6(A3<uint>f6,m0 H,uint x){uint K1;InterlockedMax(f6[H],x,K1);return K1;}
#define A4(e,R0) e6(e,H,R0)
p uint g6(A3<uint>f6,m0 H,uint x){uint K1;InterlockedAdd(f6[H],x,K1);return K1;}
#define B4(e,R0) g6(e,H,R0)
#define d0(e)
#define C4(r0)
#define T2
#define i3
#define e1(a,P,r,j,L) cbuffer Va:a8(w7){uint ea;uint a##Wa;uint a##Xa;uint a##Ya;}Y a(P r,uint j:SV_VertexID,uint fa:SV_InstanceID){uint L=fa+ea;Y K;
#define j5(a,P,r,j,L) Y a(P r,uint j:SV_VertexID){Y K;g p1;
#define n4(a,W1,X1,k2,l2,j) Y a(W1 X1,k2 l2,uint j:SV_VertexID){Y K;g p1;
#define f1(h6) K.p1=h6;}return K;
#define q2(i4,a) i4 a(Y K):SV_Target{
#define r2(l) return l;}
#define v4 ,d n0
#define K2 ,n0
#define G2 ,m0 H
#define j1 ,H
#define Q1(a) [earlydepthstencil]void a(Y K){d n0=K.p1.xy;m0 H=m0(floor(n0));
#define T3(a) Q1(a)
#define a2 }
#define m3(a) [earlydepthstencil]i a(Y K):SV_Target{d n0=K.p1.xy;m0 H=m0(floor(n0));i P0;
#define y4(a) m3(a)
#define U3 }return P0;
#define uintBitsToFloat asfloat
#define floatBitsToInt asint
#define floatBitsToUint asuint
#define fract frac
#define mix lerp
#define inversesqrt rsqrt
#define notEqual(e0,g0) ((e0)!=(g0))
#define lessThanEqual(e0,g0) ((e0)<=(g0))
#define greaterThanEqual(e0,g0) ((e0)>=(g0))
#define h0(e0,g0) mul(g0,e0)
#define U1
#define V1
#define K3
#define N3
#define L3(c,a1,a) StructuredBuffer<y0>a:register(t##c)
#define Q2(c,a1,a) StructuredBuffer<T>a:register(t##c)
#define M3(c,a1,a) StructuredBuffer<g>a:register(t##c)
#define z0(a,A0) a[A0]
#define p2(a,A0) a[A0]
p l0 unpackHalf2x16(uint u){uint y=(u>>16);uint x=u&0xffffu;return Y1(f16tof32(x),f16tof32(y));}p uint packHalf2x16(d Q0){uint x=f32tof16(Q0.x);uint y=f32tof16(Q0.y);return(y<<16)|x;}p i unpackUnorm4x8(uint u){T d2=T(u&0xffu,(u>>8)&0xffu,(u>>16)&0xffu,u>>24);return g(d2)*(1./255.);}p uint packUnorm4x8(i f){T d2=(T(f*255.)&0xff)<<T(0,8,16,24);d2.xy|=d2.zw;d2.x|=d2.y;return d2.x;}p float atan(float y,float x){return atan2(y,x);}p A inverse(A c1){A ga=A(c1[1][1],-c1[0][1],-c1[1][0],c1[0][0]);return ga*(1./determinant(c1));}
)===";
} // namespace glsl
} // namespace pls
} // namespace rive