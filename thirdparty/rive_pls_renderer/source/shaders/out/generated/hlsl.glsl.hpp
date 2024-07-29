#pragma once

#include "hlsl.exports.h"

namespace rive {
namespace pls {
namespace glsl {
const char hlsl[] = R"===(#pragma warning(disable:3550)
#ifndef _ARE_TOKEN_NAMES_PRESERVED
#define h half
#define F half2
#define q half3
#define j half4
#define Z4 short
#define N ushort
#define d float2
#define i0 float3
#define g float4
#define r8 bool3
#define B0 uint2
#define W uint4
#define o0 int2
#define P5 int4
#define N ushort
#define B float2x2
#define m6 half3x4
#endif
typedef i0 M3;
#define V half
#define l1 half2
#define c0 half3
#define j0 half4
#ifdef PD
typedef min16int Z4;typedef min16uint N;
#define J0 min16uint
#else
typedef int Z4;typedef uint N;
#define J0 uint
#endif
#define r5 half3x4
#define i inline
#define A2(n4) out n4
#define U0(a) struct a{
#define v0(e,I,a) I a:a
#define V0 };
#define z0(k6,v,a,I) I a=v.a
#define v8(e) register(b##e)
#define d4(e,a) cbuffer a:v8(e){struct{
#define N4(a) }a;}
#define D1 struct a0{
#define n0 noperspective
#define GB nointerpolation
#define l6 nointerpolation
#define K(e,I,a) I a:TEXCOORD##e
#define E1 g v1:SV_Position;};
#define S(a,I) I a
#define U(a) M.a=a
#define P(a,I) I a=M.a
#ifdef Y
#define Q1
#define R1
#endif
#ifdef HB
#define K2
#define L2
#endif
#define V2(x0,e,a) uniform Texture2D<W>a:register(t##e)
#define o4(x0,e,a) uniform Texture2D<g>a:register(t##e)
#define z1(x0,e,a) uniform Texture2D<unorm g>a:register(t##e)
#define w8(g2,a) SamplerState a:register(s##g2);
#define O3 w8
#define q3 w8
#define K1(a,H) a[H]
#define R2(a,r0,H) a.Sample(r0,H)
#define W3(a,r0,H,d3) a.SampleLevel(r0,H,d3)
#define V3(a,r0,H,e3,f3) a.SampleGrad(r0,H,e3,f3)
#define p1
#define q1
#ifdef QD
#define G3 RasterizerOrderedTexture2D
#else
#define G3 RWTexture2D
#endif
#define G1
#ifdef MC
#define F0(e,a) uniform G3<unorm j>a:register(u##e)
#else
#define F0(e,a) uniform G3<uint>a:register(u##e)
#endif
#define G0(e,a) uniform G3<uint>a:register(u##e)
#define P3 G0
#define w3 H0
#define x3 P0
#define H1
#ifdef MC
#define I0(c) c[J]
#else
#define I0(c) unpackUnorm4x8(c[J])
#endif
#define H0(c) c[J]
#ifdef MC
#define w0(c,m) c[J]=(m)
#else
#define w0(c,m) c[J]=packUnorm4x8(m)
#endif
#define P0(c,m) c[J]=(m)
i uint n6(G3<uint>o6,o0 J,uint x){uint N1;InterlockedMax(o6[J],x,N1);return N1;}
#define I4(c,S0) n6(c,J,S0)
i uint p6(G3<uint>o6,o0 J,uint x){uint N1;InterlockedAdd(o6[J],x,N1);return N1;}
#define J4(c,S0) p6(c,J,S0)
#define n1(c)
#define A1(c)
#define K4(A0)
#define Z2
#define o3
#define g1(a,R,v,k,O) cbuffer gb:v8(F7){uint pa;uint a##hb;uint a##ib;uint a##jb;}a0 a(R v,uint k:SV_VertexID,uint qa:SV_InstanceID){uint O=qa+pa;a0 M;
#define v5(a,R,v,k,O) a0 a(R v,uint k:SV_VertexID){a0 M;g v1;
#define y4(a,a2,c2,n2,o2,k) a0 a(a2 c2,n2 o2,uint k:SV_VertexID){a0 M;g v1;
#define h1(q6) M.v1=q6;}return M;
#define w2(q4,a) q4 a(a0 M):SV_Target{
#define x2(m) return m;}
#define D4 ,d p0
#define Q2 ,p0
#define M2 ,o0 J
#define m1 ,J
#define T1(a) [earlydepthstencil]void a(a0 M){d p0=M.v1.xy;o0 J=o0(floor(p0));
#define Z3(a) T1(a)
#define f2 }
#define v3(a) [earlydepthstencil]j a(a0 M):SV_Target{d p0=M.v1.xy;o0 J=o0(floor(p0));j Q0;
#define G4(a) v3(a)
#define a4 }return Q0;
#define uintBitsToFloat asfloat
#define floatBitsToInt asint
#define floatBitsToUint asuint
#define inversesqrt rsqrt
#define notEqual(f0,k0) ((f0)!=(k0))
#define lessThanEqual(f0,k0) ((f0)<=(k0))
#define greaterThanEqual(f0,k0) ((f0)>=(k0))
#define l0(f0,k0) mul(k0,f0)
#define Y1
#define Z1
#define Q3
#define T3
#define R3(e,d1,a) StructuredBuffer<B0>a:register(t##e)
#define W2(e,d1,a) StructuredBuffer<W>a:register(t##e)
#define S3(e,d1,a) StructuredBuffer<g>a:register(t##e)
#define C0(a,D0) a[D0]
#define v2(a,D0) a[D0]
i F unpackHalf2x16(uint u){uint y=(u>>16);uint x=u&0xffffu;return l1(f16tof32(x),f16tof32(y));}i uint packHalf2x16(d R0){uint x=f32tof16(R0.x);uint y=f32tof16(R0.y);return(y<<16)|x;}i j unpackUnorm4x8(uint u){W h2=W(u&0xffu,(u>>8)&0xffu,(u>>16)&0xffu,u>>24);return j0(h2)*(1./255.);}i uint packUnorm4x8(j f){W h2=(W(f*255.)&0xff)<<W(0,8,16,24);h2.xy|=h2.zw;h2.x|=h2.y;return h2.x;}i float atan(float y,float x){return atan2(y,x);}i B inverse(B e1){B ra=B(e1[1][1],-e1[0][1],-e1[1][0],e1[0][0]);return ra*(1./determinant(e1));}i float mix(float x,float y,float s){return lerp(x,y,s);}i d mix(d x,d y,d s){return lerp(x,y,s);}i i0 mix(i0 x,i0 y,i0 s){return lerp(x,y,s);}i g mix(g x,g y,g s){return lerp(x,y,s);}i h mix(h x,h y,h s){return x+s*(y-x);}i F mix(F x,F y,F s){return x+s*(y-x);}i q mix(q x,q y,q s){return x+s*(y-x);}i j mix(j x,j y,j s){return x+s*(y-x);}i float fract(float x){return frac(x);}i d fract(d x){return frac(x);}i i0 fract(i0 x){return frac(x);}i g fract(g x){return frac(x);}i h fract(h x){return V(frac(x));}i F fract(F x){return l1(frac(x));}i q fract(q x){return c0(frac(x));}i j fract(j x){return j0(frac(x));}i h C2(h x){return V(sign(x));}i F C2(F x){return l1(sign(x));}i q C2(q x){return c0(sign(x));}i j C2(j x){return j0(sign(x));}i float C2(float x){return sign(x);}i d C2(d x){return sign(x);}i i0 C2(i0 x){return sign(x);}i g C2(g x){return sign(x);}
#define sign C2
i h D2(h x){return V(abs(x));}i F D2(F x){return l1(abs(x));}i q D2(q x){return c0(abs(x));}i j D2(j x){return j0(abs(x));}i float D2(float x){return abs(x);}i d D2(d x){return abs(x);}i i0 D2(i0 x){return abs(x);}i g D2(g x){return abs(x);}
#define abs D2
i h E2(h x){return V(sqrt(x));}i F E2(F x){return l1(sqrt(x));}i q E2(q x){return c0(sqrt(x));}i j E2(j x){return j0(sqrt(x));}i float E2(float x){return sqrt(x);}i d E2(d x){return sqrt(x);}i i0 E2(i0 x){return sqrt(x);}i g E2(g x){return sqrt(x);}
#define sqrt E2
)===";
} // namespace glsl
} // namespace pls
} // namespace rive