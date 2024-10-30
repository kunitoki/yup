#pragma once

#include "rhi.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char rhi[] = R"===(#pragma warning(disable:3550)
#pragma warning(disable:4000)
#ifndef _ARE_TOKEN_NAMES_PRESERVED
#define h half
#define r half2
#define k half3
#define i half4
#define J4 short
#define M ushort
#define f float2
#define Q float3
#define g float4
#define O6 bool3
#define n1 uint2
#define G uint4
#define e0 int2
#define g5 int4
#define M ushort
#define I float2x2
#define w4 half3x3
#define x4 half2x3
#endif
typedef Q I3;
#ifdef GD
typedef min16uint M;
#else
typedef uint M;
#endif
#define P1(L,O) L##O
#define d inline
#define j2(f1) out f1
#define l4(f1) inout f1
#define Y0(a) struct a{
#define a0(c,D,a) D a:P1(Jb,c)
#define Z0 };
#define f0(w5,q,a,D) D a=q.a
#define A5(c) register(P1(b,c))
#define Y3(c,a) cbuffer a:A5(c){struct{
#define y4(a) }a;}
#define H1 struct S{
#define w0 noperspective
#define FB nointerpolation
#define H3 nointerpolation
#define W(c,D,a) D a:P1(TEXCOORD,c)
#define I1 g F0:SV_Position;};
#define c0(a,D) D a
#define d0(a) E.a=a
#define Y(a,D) D a=E.a
#ifdef AB
#define R1
#define S1
#endif
#ifdef EB
#define H2
#define I2
#endif
#define R2(v0,c,a) uniform Texture2D<G>a:register(P1(t,c))
#define e4(v0,c,a) uniform Texture2D<g>a:register(P1(t,c))
#define v1(v0,c,a) uniform Texture2D<unorm g>a:register(P1(t,c))
#define L4(a2,a) SamplerState a:register(P1(s,a2));
#define K3 L4
#define c3 L4
#define N1(a,K) a[K]
#define l3(a,i0,K) a.Sample(i0,K)
#define h3(a,i0,K,v2) a.SampleLevel(i0,K,v2)
#define w1
#define x1
#ifdef HD
#define A1 RasterizerOrderedTexture2D
#else
#define A1 RWTexture2D
#endif
#define J1
#ifdef AC
#define A0(c,a) uniform A1<unorm i>a:register(SPLAT(u,c))
#else
#define A0(c,a) uniform A1<uint>a:register(P1(u,c))
#endif
#define C0(c,a) uniform A1<uint>a:register(P1(u,c))
#define d3 C0
#define i3 I0
#define j3 K0
#define K1
#ifdef AC
#define r0(e) e[v]
#else
#define r0(e) unpackUnorm4x8(e[v])
#endif
#define I0(e) e[v]
#ifdef AC
#define y0(e,o) e[v]=(o)
#else
#define y0(e,o) e[v]=packUnorm4x8(o)
#endif
#define K0(e,o) e[v]=(o)
d uint M4(A1<uint>y3,e0 v,uint x){uint T0;InterlockedMax(y3[v],x,T0);return T0;}
#define p4(e,n0) M4(e,v,n0)
d uint N4(A1<uint>y3,e0 v,uint x){uint T0;InterlockedAdd(y3[v],x,T0);return T0;}
#define q4(e,n0) N4(e,v,n0)
#define O1(e)
#define X1(e)
#define T2
#define Y2
#define h1(a,V,q,n,H) uint baseInstance;S a(V q,uint n:SV_VertexID,uint P6:SV_InstanceID){uint H=P6+baseInstance;S E;
#define T4(a,V,q,n,H) S a(V q,uint n:SV_VertexID){S E;g F0;
#define j4(a,T1,U1,h2,i2,n) S a(T1 U1,h2 i2,uint n:SV_VertexID){S E;g F0;
#define i1(O4) E.F0=O4;}return E;
#define q2(x3,a) x3 a(S E):SV_Target{
#define r2(o) return o;}
#define m4 ,f h0
#define O2 ,h0
#define L2 ,e0 v
#define X0 ,v
#define M1(a) [earlydepthstencil]void a(S E){f h0=E.F0.xy;e0 v=e0(floor(h0));
#define U3(a) M1(a)
#define k2 }
#define N2(a) [earlydepthstencil]i a(S E):SV_Target{f h0=E.F0.xy;e0 v=e0(floor(h0));i e1;
#define o4(a) N2(a)
#define S3 }return e1;
#define uintBitsToFloat asfloat
#define floatBitsToInt asint
#define floatBitsToUint asuint
#define inversesqrt rsqrt
#define notEqual(L,O) ((L)!=(O))
#define lessThanEqual(L,O) ((L)<=(O))
#define lessThan(L,O) ((L)<(O))
#define greaterThanEqual(L,O) ((L)>=(O))
#define m0(L,O) mul(O,L)
#define f2
#define g2
#define M3
#define P3
#define N3(c,Q0,a) StructuredBuffer<n1>a:register(P1(t,c))
#define S2(c,Q0,a) StructuredBuffer<G>a:register(P1(t,c))
#define O3(c,Q0,a) StructuredBuffer<g>a:register(P1(t,c))
#define E0(a,o0) a[o0]
#define Q3(a,o0) a[o0]
d r unpackHalf2x16(uint u){uint y=(u>>16);uint x=u&0xffffu;return r(f16tof32(x),f16tof32(y));}d uint packHalf2x16(f L0){uint x=f32tof16(L0.x);uint y=f32tof16(L0.y);return(y<<16)|x;}d i unpackUnorm4x8(uint u){G S0=G(u&0xffu,(u>>8)&0xffu,(u>>16)&0xffu,u>>24);return i(S0)*(1./255.);}d uint packUnorm4x8(i j){G S0=(G(j*255.)&0xff)<<G(0,8,16,24);S0.xy|=S0.zw;S0.x|=S0.y;return S0.x;}d float atan(float y,float x){return atan2(y,x);}d I inverse(I B0){I Q6=I(B0[1][1],-B0[0][1],-B0[1][0],B0[0][0]);return Q6*(1./determinant(B0));}d float mix(float x,float y,float s){return lerp(x,y,s);}d f mix(f x,f y,f s){return lerp(x,y,s);}d Q mix(Q x,Q y,Q s){return lerp(x,y,s);}d g mix(g x,g y,g s){return lerp(x,y,s);}d float fract(float x){return frac(x);}d f fract(f x){return frac(x);}d Q fract(Q x){return frac(x);}d g fract(g x){return frac(x);}d float B1(float x){return sign(x);}d f B1(f x){return sign(x);}d Q B1(Q x){return sign(x);}d g B1(g x){return sign(x);}
#define sign B1
d float C1(float x){return abs(x);}d f C1(f x){return abs(x);}d Q C1(Q x){return abs(x);}d g C1(g x){return abs(x);}
#define abs C1
d float D1(float x){return sqrt(x);}d f D1(f x){return sqrt(x);}d Q D1(Q x){return sqrt(x);}d g D1(g x){return sqrt(x);}
#define sqrt D1
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive